# v5 변경 사항 (v4 플레이테스트 1명 기반, 회사 PC, 2026-10-19)

데이터: `Docs/Playtest/data/v4/tester5.csv`. 이 CSV는 `build_info`에 `build`가 없어 집 PC v4 빌드로 플레이한 것으로 보인다.

| v4 발견 | v5 변경 |
|---|---|
| 마지막 액자를 들면 열쇠가 자동으로 들어옴. 화면 아래 문구를 못 읽어서 열쇠를 얻은 줄 몰랐다(피드백). 열쇠를 얻고 서랍을 열기까지 82초(29.7초 → 112초), 그동안 액자를 20번 넘게 다시 누름 | 액자를 들면 액자가 올라간 채로 있고, 열쇠 모양(둥근 머리, 몸통, 이)이 벽에 걸려 은은하게 깜빡인다. **E로 직접 집어야** 얻는다(테스터 제안). 프롬프트는 "[E] 열쇠 집기". 퍼즐 1은 집는 순간 풀림 |
| 획득 알림이 아래쪽 문구 한 줄뿐 | 아이템을 얻으면 화면 가운데 위에 큰 글씨 "서랍 열쇠 획득!" / "팔레트 획득!" / "문 열쇠 획득!"(2.5초, `ItemToastSeconds`). 왼쪽 아래 해당 아이콘 테두리가 노랗게 깜빡임 |
| 열쇠를 가진 채 서랍으로 가지 않음 | 열쇠를 얻고 20초(`DrawerNudgeDelay`) 동안 서랍을 열지 않으면 서랍과 손잡이가 은은하게 빛남. 힌트로 세지 않음 |
| (기획 요청) 아이템 획득 방식을 통일 | 팔레트: 서랍을 열면 안에 보이고, E로 집으면 서랍에서 사라짐("[E] 팔레트 집기"). 서랍 열쇠 아이콘은 서랍을 여는 순간 사라짐. 문 열쇠: 금고가 열리면 금고 안쪽 면에 은색 열쇠(아이콘과 같은 색)가 깜빡이고, E로 집어야 얻음. 퍼즐 3은 집는 순간 풀림 |
| 퍼즐 1을 찍어서 풂(액자 4개를 다 누름) | 변경 없음(v2 기획 결정 유지: 오답 쿨다운 없음) |
| 퍼즐 2: 오답 2번 후 상식으로 해결, 퍼즐 3: 36.7초에 한 번에 해결 | 변경 없음(의도한 난이도) |

새 이벤트: `item_found{item=drawer_key|palette|door_key}`(아이템이 보이게 됨: 액자 들기, 서랍 열기, 금고 열기), `item_picked{item=...}`(E로 집음), `nudge_shown{target=drawer}`. `build_info`는 `build=work-pc;version=v5`.
퍼즐 1과 3의 `puzzle_solved`는 이제 열쇠를 집는 순간 기록되므로, v4 이전보다 드러남 → 집기 시간만큼 늦게 찍힌다(`item_found`와 `item_picked` 차이로 구할 수 있음).

개발용: `Escape.Autoplay ... topview`로 천장을 숨긴 평면도 스크린숏(`plan_top.png`)을 찍는다.

## 확인 결과

- 에디터 빌드 성공(경고 없음), 플러그인 테스트 7개 통과.
- `Escape.Autoplay 4 quit`: 35.4초에 탈출. 서랍 열쇠·팔레트·문 열쇠 모두 `item_found` → `item_picked` 순서로 기록, 열쇠 두 개는 집는 순간 `puzzle_solved`. `nudge_shown drawer`(지연 4초로 단축). 팔레트를 집은 뒤 서랍이 빈 것 스크린숏으로 확인. 트레이스 실패 없음.
- Shipping exe 1회 실행(H, Esc, Esc, Esc, Q, Y): `build_info build=work-pc;version=v5`, `hint_shown source=request`, 메뉴 열기/닫기 2번(`paused_sec` 3.3, 4.6), `session_end reason=Quit`.
- 장표용 1920×1080 스크린숏 18장: `Docs/EscapeRoom/v5/`.

알려진 문제: `item_picked`의 팔레트가 `item=Palette`(대문자)로 기록될 수 있다(FName이 같은 이름의 컴포넌트와 대소문자 표기를 공유). 분석 시 대소문자 무시. v5에서는 고치지 않음.

빌드: `Packaged/EscapeRoom_Playtest_v5.zip` (245.0MB, 안내문 `플레이테스트_안내.txt` 포함).
