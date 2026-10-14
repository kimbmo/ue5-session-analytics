# Session Analytics (UE5 Plugin)

A lightweight Unreal Engine 5 plugin that collects gameplay/UI events per session and exports them to CSV.

Built to solve problems I hit while instrumenting a real-time UE5 product:
duplicate events from repeated UI callbacks, events fired before a session existed,
and analytics code tangled with gameplay Blueprints.

## Features
- `GameInstanceSubsystem`: one session per game instance, no actor to place in levels
- Automatic `session_start` / `session_end` events (end reason + suppressed duplicate count)
- Duplicate suppression: same event + same params inside a configurable window is dropped
- Order-independent param serialization (sorted keys) so duplicates are detected reliably
- Buffered, append-only CSV writing with RFC 4180 escaping; failed writes are retried on the next flush
- Events are flushed every N seconds (default 2), when the buffer reaches a threshold, and on session end
- Every call returns a result (`Recorded` / `Duplicate` / `NoSession` / `InvalidName`) so callers can verify what happened
- Blueprint-friendly API and Project Settings page

## Repository layout
- `Plugins/SessionAnalytics/` – the plugin (source, tests, settings)
- `Source/SessionAnalyticsDemo/` – demo module with `AAnalyticsDemoActor`, which fires a scripted set of events including one duplicate
- `Content/Maps/AnalyticsDemo.umap` – demo map (one `AnalyticsDemoActor`); `Scripts/create_demo_map.py` recreates it headless
- Open `SessionAnalyticsDemo.uproject` with UE 5.8 (also builds on 5.7: right-click the .uproject > Switch Unreal Engine version), build, press Play: the CSV appears under `Saved/Analytics/`

## Install
1. Copy `Plugins/SessionAnalytics` from this repo to `<YourProject>/Plugins/SessionAnalytics`.
2. Regenerate project files, build, and enable **Session Analytics** in the Plugins window.
3. Options: **Project Settings > Plugins > Session Analytics**.

| Setting | Default | Meaning |
|---|---|---|
| `bAutoStartSession` | true | Start a session when the game instance starts |
| `DuplicateWindowSeconds` | 0.5 | Same event + params inside this window is a duplicate; 0 disables |
| `FlushThreshold` | 20 | Write to disk once this many events are buffered |
| `FlushIntervalSeconds` | 2.0 | Also write to disk this often; 0 disables |
| `OutputSubdirectory` | Analytics | Output folder under `Saved/` |

## Usage
Blueprint: `Get Game Instance Subsystem (SessionAnalyticsSubsystem)` → `Record Event`.

C++:
```cpp
if (auto* Analytics = GetGameInstance()->GetSubsystem<USessionAnalyticsSubsystem>())
{
	Analytics->RecordEvent(TEXT("color_selected"), { { TEXT("part"), TEXT("exterior") }, { TEXT("color"), TEXT("Red") } });
}
```

Output: `Saved/Analytics/<utc>_<sessionid>.csv`. Real output from the demo map (the second `Red` arrived inside the duplicate window and was dropped):
```
session_id,seq,timestamp_utc,elapsed_sec,event,params
C9F576A4-...,0,2026-10-14T01:34:02.128Z,0.000,session_start,
C9F576A4-...,1,2026-10-14T01:34:02.581Z,0.454,view_changed,view=exterior
C9F576A4-...,2,2026-10-14T01:34:02.704Z,0.576,color_selected,color=Red
C9F576A4-...,3,2026-10-14T01:34:02.946Z,0.819,color_selected,color=Blue
C9F576A4-...,4,2026-10-14T01:34:03.067Z,0.940,view_changed,view=interior
C9F576A4-...,5,2026-10-14T01:34:03.184Z,1.056,session_end,duplicates_suppressed=1;reason=DemoFinished
```

The `params` column is `key=value` pairs joined with `;`, sorted by key. A `\`, `;` or `=` inside a key or value
is written with a leading backslash (`\\`, `\;`, `\=`), so the column can always be split back unambiguously.

## Tests
Automation tests (`Session Frontend > Automation`, filter `SessionAnalytics`):
- `CsvEscape`: commas/quotes in params are CSV-escaped, keys are sorted
- `ParamEscape`: `\`, `;`, `=` inside params cannot be confused with separators
- `ParamOrderIndependence`: insertion order of params does not change the duplicate key
- `RecordAndDuplicate`: no-session, duplicate and invalid-name paths and the written row count
- `RecordAgainAfterWindow`: the same event is recorded again once the window has passed; window 0 disables suppression
- `FlushThreshold`: buffered events reach disk exactly at the threshold, on manual flush and on session end
- `SessionRestart`: restarting writes `session_end` (reason=Restart) to the old file and starts a fresh file and sequence

Headless run:
```
UnrealEditor-Cmd <Project>.uproject -ExecCmds="Automation RunTests SessionAnalytics; Quit" -unattended -nullrhi
```

Verified on UE 5.8.3 and UE 5.7.4 (Win64, Editor target): build OK, all 7 tests pass, demo map produces the CSV above.
A forced kill after 5 seconds still left `session_start` on disk thanks to the periodic flush.

## Design notes
- **Collection vs. output are separate.** Gameplay code only calls `RecordEvent`; buffering and file I/O live in the subsystem.
- **Duplicates are decided by content, not by caller.** UI widgets can fire the same callback several times; the subsystem filters that centrally.
- **Rejected events are reported, not hidden.** Events without a session return `NoSession` and log a warning instead of disappearing.
- **Loss on a crash is bounded.** Buffered events reach disk every `FlushIntervalSeconds`, so a crash or forced kill can lose
  at most the events of the last interval, plus the `session_end` row. A normal shutdown or `EndSession` writes everything.
