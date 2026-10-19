#!/usr/bin/env python3
"""Summarize escape-room playtest CSVs written by the SessionAnalytics plugin.

Usage:
    python analyze_playtest.py <csv or folder> [...] [-o report.md]

Each CSV is one play session (columns: session_id, seq, timestamp_utc,
elapsed_sec, event, params). Params are "key=value;key=value" with '\\', ';'
and '=' backslash-escaped. Prints a Markdown report: one row per session, then
per-puzzle and per-object summaries checked against the design doc's targets.
"""
import argparse
import csv
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

PUZZLES = (1, 2, 3)
TARGET_SEC = {1: 40, 2: 60, 3: 80}   # design doc: per-puzzle target times
TARGET_TOTAL_SEC = 180               # design doc: 3 minutes
FAIL_TOTAL_SEC = 300                 # design doc: over 5 minutes counts as a failure
QUICK_AFTER_HINT_SEC = 15            # solved this soon after hint 1 = hint 1 supplied the missing link
# Events the game logs on its own; everything else is a player action (used for the idle gap).
SYSTEM_EVENTS = {"session_start", "build_info", "puzzle_started", "hint_shown", "escaped", "session_end"}


def split_unescaped(text, sep):
    """Splits on sep characters that are not backslash-escaped; keeps escapes intact."""
    parts, cur, i = [], [], 0
    while i < len(text):
        ch = text[i]
        if ch == "\\" and i + 1 < len(text):
            cur.append(text[i:i + 2])
            i += 2
            continue
        if ch == sep:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
        i += 1
    parts.append("".join(cur))
    return parts


def unescape(text):
    out, i = [], 0
    while i < len(text):
        if text[i] == "\\" and i + 1 < len(text):
            out.append(text[i + 1])
            i += 2
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def parse_params(raw):
    params = {}
    if not raw:
        return params
    for pair in split_unescaped(raw, ";"):
        kv = split_unescaped(pair, "=")
        if len(kv) == 2:
            params[unescape(kv[0])] = unescape(kv[1])
    return params


def load_session(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    events = []
    for row in rows:
        events.append({
            "t": float(row["elapsed_sec"]),
            "event": row["event"],
            "p": parse_params(row.get("params", "")),
        })
    events.sort(key=lambda e: e["t"])
    return {"file": Path(path).name, "id": rows[0]["session_id"] if rows else "", "events": events}


def paused_between(pauses, a, b):
    """Seconds of Esc-menu pause inside [a, b]."""
    return sum(max(0.0, min(b, end) - max(a, start)) for start, end in pauses)


def summarize(session):
    started, solved = {}, {}
    version = None
    pauses, menu_open_at = [], None
    hint_sources = Counter()
    wrong = defaultdict(list)
    hints = defaultdict(int)
    hint_times = defaultdict(dict)
    hint_events = 0
    inspected = Counter()
    first_inspect = None
    escaped = None
    end_reason = None
    repeats = None
    for e in session["events"]:
        ev, p = e["event"], e["p"]
        puzzle = int(p["puzzle"]) if p.get("puzzle", "").isdigit() else None
        if ev == "puzzle_started" and puzzle:
            started.setdefault(puzzle, e["t"])
        elif ev == "puzzle_solved" and puzzle:
            solved.setdefault(puzzle, e["t"])
        elif ev == "wrong_attempt" and puzzle:
            wrong[puzzle].append(p.get("input", ""))
        elif ev == "hint_shown" and puzzle:
            level = int(p.get("level", "0") or 0)
            hints[puzzle] = max(hints[puzzle], level)
            hint_times[puzzle].setdefault(level, e["t"])
            hint_events += 1
            # v1-v3 had only timed hints and logged no source.
            hint_sources[p.get("source", "auto")] += 1
        elif ev == "build_info":
            version = p.get("version")
        elif ev == "menu_opened":
            menu_open_at = e["t"]
        elif ev == "menu_closed":
            start = menu_open_at if menu_open_at is not None else e["t"] - float(p.get("paused_sec", "0") or 0)
            pauses.append((start, e["t"]))
            menu_open_at = None
        elif ev == "object_inspected":
            obj = p.get("object", "?")
            inspected[obj] += 1
            first_inspect = first_inspect or obj
        elif ev == "escaped":
            escaped = {"total": float(p.get("total_sec", e["t"])), "hints": int(p.get("hints_used", "0") or 0), "t": e["t"]}
        elif ev == "session_end":
            end_reason = p.get("reason")
            # The plugin folds identical records within 0.5s into one, so this counts rapid E repeats.
            repeats = int(p.get("duplicates_suppressed", "0") or 0)
    last_t = session["events"][-1]["t"] if session["events"] else 0.0
    if menu_open_at is not None:
        pauses.append((menu_open_at, last_t))
    # Esc-menu time is not play time: take it out of puzzle and total times.
    durations = {n: solved[n] - started[n] - paused_between(pauses, started[n], solved[n])
                 for n in PUZZLES if n in started and n in solved}
    paused_total = sum(end - start for start, end in pauses)
    if escaped and pauses:
        escaped["total"] = escaped["t"] - paused_between(pauses, 0.0, escaped["t"])
    # Longest stretch with no player action, outside the menu (e.g. a tester writing notes).
    action_times = [0.0] + [e["t"] for e in session["events"]
                            if e["event"] not in SYSTEM_EVENTS or e["p"].get("source") == "request"] + [last_t]
    idle = max((b - a - paused_between(pauses, a, b) for a, b in zip(action_times, action_times[1:])), default=0.0)
    after_hint1 = {n: solved[n] - hint_times[n][1] for n in PUZZLES if n in solved and 1 in hint_times[n]}
    reached = max(started) if started else 0
    return {
        "file": session["file"], "durations": durations, "after_hint1": after_hint1,
        "wrong": wrong, "hints": hints, "hint_events": hint_events,
        "inspected": inspected, "first_inspect": first_inspect, "escaped": escaped,
        "end_reason": end_reason, "repeats": repeats, "last_t": last_t, "reached": reached,
        "version": version, "paused": paused_total, "idle": idle, "hint_sources": hint_sources,
    }


def normalize_input(puzzle, value):
    # Paint pairs are logged in click order; "blue+red" and "red+blue" are the same answer.
    if puzzle == 2 and "+" in value:
        return "+".join(sorted(value.split("+")))
    return value


def fmt_sec(value):
    return "-" if value is None else f"{value:.0f}s"


def report(summaries):
    out = []
    n = len(summaries)
    out.append(f"# 플레이테스트 결과 ({n}판)\n")

    out.append("## 판별 요약\n")
    out.append("시간은 Esc 메뉴로 멈춘 시간을 뺀 값입니다. 힌트는 요청/자동. 가장 긴 무입력은 메뉴 밖에서 플레이어 입력이 없던 가장 긴 구간입니다.\n")
    out.append("| 파일 | 빌드 | 결과 | 전체 | 퍼즐 1 | 퍼즐 2 | 퍼즐 3 | 오답 | 힌트 | 멈춤 | 가장 긴 무입력 | 0.5초 안 연타 | 처음 조사한 것 |")
    out.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for s in summaries:
        if s["escaped"]:
            result, total = "탈출", s["escaped"]["total"]
        else:
            result, total = f"중단 (퍼즐 {s['reached']}에서)", s["last_t"]
        wrong_total = sum(len(v) for v in s["wrong"].values())
        hint_total = f"{s['hint_sources']['request']}/{s['hint_sources']['auto']}"
        cells = [fmt_sec(s["durations"].get(k)) for k in PUZZLES]
        repeats = "-" if s["repeats"] is None else s["repeats"]
        out.append(f"| {s['file']} | {s['version'] or '-'} | {result} | {fmt_sec(total)} | {' | '.join(cells)} | {wrong_total} | {hint_total} | {fmt_sec(s['paused'])} | {fmt_sec(s['idle'])} | {repeats} | {s['first_inspect'] or '-'} |")
    out.append("")

    out.append("## 퍼즐별\n")
    out.append(f"| 퍼즐 | 목표 | 중앙값 | 최댓값 | 목표 2배 넘은 판 | 힌트 없이 푼 판 | 힌트 1 뒤 {QUICK_AFTER_HINT_SEC}초 안에 푼 판 | 힌트 2단계까지 간 판 | 오답 합계 | 가장 많은 오답 |")
    out.append("|---|---|---|---|---|---|---|---|---|---|")
    flags = []
    for k in PUZZLES:
        times = [s["durations"][k] for s in summaries if k in s["durations"]]
        solved_n = len(times)
        over = sum(1 for t in times if t > 2 * TARGET_SEC[k])
        no_hint = sum(1 for s in summaries if k in s["durations"] and s["hints"].get(k, 0) == 0)
        after = [s["after_hint1"][k] for s in summaries if k in s["after_hint1"]]
        quick = sum(1 for t in after if t <= QUICK_AFTER_HINT_SEC)
        hint2 = sum(1 for s in summaries if s["hints"].get(k, 0) >= 2)
        inputs = Counter(normalize_input(k, v) for s in summaries for v in s["wrong"].get(k, []))
        top = ", ".join(f"{v} ({c})" for v, c in inputs.most_common(2)) or "-"
        med = statistics.median(times) if times else None
        mx = max(times) if times else None
        out.append(f"| {k} | {TARGET_SEC[k]}s | {fmt_sec(med)} | {fmt_sec(mx)} | {over}/{solved_n} | {no_hint}/{solved_n} | {quick}/{len(after)} | {hint2}/{n} | {sum(inputs.values())} | {top} |")
        if len(after) >= 2 and quick * 2 >= len(after):
            flags.append(f"퍼즐 {k}: 힌트 1을 본 판 대부분이 {QUICK_AFTER_HINT_SEC}초 안에 풂. 힌트 1이 알려 주는 연결이 방 안 단서만으로는 안 보일 가능성.")
        if over:
            flags.append(f"퍼즐 {k}: {over}판이 목표({TARGET_SEC[k]}s)의 2배를 넘김. 단서 위치와 조명, 힌트 시간을 의심.")
        repeated = [v for v, c in inputs.items() if c >= 2]
        if repeated:
            flags.append(f"퍼즐 {k}: 같은 오답이 반복됨 ({', '.join(repeated)}). 단서를 다르게 읽었을 가능성.")
        if hint2 * 2 >= n and n:
            flags.append(f"퍼즐 {k}: 절반 이상이 힌트 2단계까지 감. 퍼즐 자체가 어려움.")
        fast = sum(1 for s in summaries if s["durations"].get(k, 99) < 10 and s["hints"].get(k, 0) == 0)
        if fast * 2 >= n and n:
            flags.append(f"퍼즐 {k}: 절반 이상이 힌트 없이 10초 안에 풂. 너무 쉬울 수 있음.")
    out.append("")

    out.append("## 소품별 조사 횟수\n")
    out.append("| 소품 | 조사한 판 | 총 횟수 |")
    out.append("|---|---|---|")
    objects = sorted({o for s in summaries for o in s["inspected"]})
    for o in objects:
        sessions_with = sum(1 for s in summaries if s["inspected"][o])
        total = sum(s["inspected"][o] for s in summaries)
        out.append(f"| {o} | {sessions_with}/{n} | {total} |")
    out.append("")
    out.append("단서 소품인데 이 표에 없거나 조사한 판이 적으면, 시선이 그쪽으로 가지 않는다는 뜻입니다.\n")

    escaped = [s["escaped"]["total"] for s in summaries if s["escaped"]]
    out.append("## 기획서 기준과 비교\n")
    if escaped:
        out.append(f"- 전체 클리어: 탈출 {len(escaped)}/{n}판, 중앙값 {fmt_sec(statistics.median(escaped))} (목표 {TARGET_TOTAL_SEC}s 안팎)")
        over5 = sum(1 for t in escaped if t > FAIL_TOTAL_SEC)
        out.append(f"- 5분 넘은 판: {over5 + (n - len(escaped))}/{n} (중단 포함, 목표 0)")
    else:
        out.append("- 탈출한 판이 없습니다.")
    for f in flags or ["목표에서 크게 벗어난 퍼즐은 없습니다."]:
        out.append(f"- {f}")
    out.append("")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("paths", nargs="+", help="CSV files or folders containing them")
    ap.add_argument("-o", "--output", help="write the report here instead of stdout")
    args = ap.parse_args()

    files = []
    for p in map(Path, args.paths):
        files += sorted(p.glob("*.csv")) if p.is_dir() else [p]
    if not files:
        sys.exit("No CSV files found.")

    text = report([summarize(load_session(f)) for f in files])
    if args.output:
        Path(args.output).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
