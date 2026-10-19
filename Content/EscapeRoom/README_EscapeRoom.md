# 방탈출 프로토타입 (L_Studio)

화가의 작업실에서 3분 안에 나가는 1인칭 방탈출. 모든 플레이는 SessionAnalytics 플러그인으로 `Saved/Analytics/*.csv`에 기록된다. 현재 버전은 v5이고, 플레이테스트 기록과 버전별 변경은 `Docs/Playtest/`에 있다.

## 플레이 방법
- 에디터에서 `Content/EscapeRoom/L_Studio`를 열고 Play (기본 맵으로도 설정돼 있음). 뷰포트를 한 번 클릭해야 마우스가 잡힌다.
- **WASD** 이동, **마우스** 시점, **E** 상호작용 (카메라에서 250cm 라인 트레이스), **H** 힌트 요청, **Esc** 일시 정지 메뉴. 메시지는 3초 동안 화면에 뜬다.
- 열쇠, 팔레트 같은 물건은 보이는 곳에 나타나고 E로 직접 집는다. 집으면 "…획득!"이 화면에 크게 뜨고 왼쪽 아래 소지품 아이콘이 깜빡인다.
- 퍼즐은 순서대로, 현재 퍼즐만 활성화된다.
  1. **찾기**: 책상 메모 "열쇠는 늘 마지막 작품 뒤에" → 날짜 라벨(YYYY.MM)이 가장 늦은 액자를 E로 들면 뒤에 서랍 열쇠가 보이고, E로 집는다. 다른 액자를 들면 오답. 열쇠를 집고 20초 동안 서랍을 열지 않으면 서랍이 은은하게 빛난다(힌트로 세지 않음).
  2. **조합**: 서랍을 열면 팔레트가 있고, E로 집는다. 물감 다섯 개 중 두 개를 차례로 E → 파랑+노랑 = 문 위 램프의 초록이면 정답. 이젤의 빈 칸이 초록으로 채워지고, 금고 덮개가 사라지고 조명이 바뀐다.
  3. **색 맞추기**: 금고 다이얼마다 뒤에 색 판이 있다. 그 색과 바탕색이 같은 그림을 찾아, 그 그림 속 물체 개수를 그 다이얼(E로 0-9 돌림)에 넣고 손잡이 E(기본 3715) → 금고 속 문 열쇠를 E로 집는다. 문에 E → 탈출.
     - 다이얼은 금고 앞에서 볼 때 왼쪽부터 1·2·3·4번째 자리.
     - 단서(글자 없음): 다이얼 위의 액자 모양 실루엣도 같은 바탕색이고, 실루엣 위 점 두 개는 그 그림 속 물체의 색이다. 실루엣은 모두 같은 크기라 크기는 단서가 아니다. 색은 레벨의 액자(`CanvasColor`, `ObjectColor`)에서 자동으로 가져온다.
     - 다이얼 n번은 n번째로 작은 액자와 짝이라, v3까지의 "작은 액자부터 큰 액자 순" 코드와 같다. (v3 플레이테스트에서 크기 대신 색으로 푼 것을 보고 v4부터 색을 정식 단서로 바꿈)

정답: 2024.02 액자 → 파랑+노랑 → 3715 → 문.

## 클래스 역할 (Source/SessionAnalyticsDemo/EscapeRoom)
| 클래스 | 역할 |
|---|---|
| `AEscapeRoomManager` | 퍼즐 진행(현재 퍼즐, 시도 횟수), 힌트 타이머, 조명, 소지품, 분석 이벤트 기록. 레벨에 1개. **튜닝 값은 전부 여기 Details 패널** |
| `AEscapeInteractable` | E로 상호작용 가능한 모든 물체의 부모. 기본은 "살펴보기"(object_inspected 기록 + 메시지). 메모·금고 덮개가 이 클래스 그대로 |
| `AEscapeFrame` | 액자. 날짜 라벨, 크기, 바탕색, 그림 속 물체 수와 색. 퍼즐1에선 들어 올리기(정답 액자 뒤 서랍 열쇠), 퍼즐3에선 관찰 대상 |
| `AEscapeDesk` | 책상 + 잠긴 서랍(안에 팔레트, E로 집음) |
| `AEscapePaint` | 물감 통. PaintId(로그용)와 색 |
| `AEscapeEasel` | 이젤. 빈 칸이 퍼즐2 정답 후 초록으로 채워짐 |
| `AEscapeSafe` | 다이얼 4개 + 손잡이 금고. 다이얼 뒤 색 판과 위 실루엣(색 단서), 안에 문 열쇠 |
| `AEscapeDoor` | 출구 문 + 초록 램프 |
| `AEscapeCharacter` | 1인칭 이동, E 트레이스, 바라보는 부분 하이라이트, H 힌트 요청, Esc 메뉴(Esc 계속하기, Q 종료 → Y/N 확인) |
| `AEscapeHUD` | Slate 화면 표시: 크로스헤어(대상 있으면 노랑, E 누르면 반응), 프롬프트, 메시지, 힌트 문구, 퍼즐 번호, `[H] 힌트 n/2`, 획득 문구, 소지품 아이콘(서랍 열쇠, 팔레트, 물감 1·2, 문 열쇠), 조작 안내, 일시 정지 메뉴(마우스 감도, 종료 확인) |
| `AEscapeGameMode` | 위 캐릭터와 HUD 사용 |
| `EscapeAutoplay.cpp` | 개발용 콘솔 명령 `Escape.Autoplay` (아래) |

로직은 C++, `Content/EscapeRoom/Blueprints`의 BP_*는 C++을 그대로 상속한 얇은 블루프린트(값 조정용)다.
게임플레이 코드는 `Manager->RecordEvent`만 부르고, 파일 쓰기·중복 제거는 플러그인이 맡는다.

## 튜닝 값 (코드 수정 없이 Details 패널)
**EscapeRoomManager**
- 힌트는 H로 요청한다. 퍼즐마다 2단계이고, 다 보면 "이 퍼즐의 힌트는 모두 봤다."가 뜬다. (v3 플레이테스트에서 자동 힌트가 너무 빠르다는 말을 듣고 v4부터 요청형으로 바꿈)
- `HintDelay1` 90초 / `HintDelay2` 150초: H를 누르지 않는 사람을 위한 안전망. 마지막 "실제 단계" 뒤로 이 시간이 지나면 힌트 1 / 2가 자동으로 나온다. 실제 단계는 퍼즐 시작과 서랍 열기뿐이고, 살펴보기·다이얼·오답으로는 타이머가 다시 시작되지 않는다. (v1 플레이테스트에서 E를 연타하던 사람이 힌트를 못 받아서 바꿈) 메뉴가 열려 있는 동안은 타이머가 멈춘다.
- `HintGlowStrength`, `HintPulseSpeed`, `SequenceStepSeconds`: 힌트 빛의 세기, 깜빡임 속도, 순차 힌트 간격.
- `Puzzles[0..2]`: 퍼즐별 힌트 태그(`Hint1Tag`/`Hint2Tag`), 조명 태그(`LightTag`), 힌트2 순차 여부, 힌트 문구.
- `CorrectPaintA/B` (blue/yellow), `SafeCode` (3715), `SafeCover`, `InactiveLightScale` (다른 퍼즐 조명 밝기 비율, 0.6), `MessageSeconds` (3초), `HintMessageSeconds` (힌트 문구 4초, 화면 위 가운데), `HandleCooldownSeconds` (금고 손잡이 연타 무시 1초), `bShowElapsedTime` (경과 시간 표시, 기본 꺼짐).
- `DrawerNudgeDelay` (20초) / `DrawerNudgeStrength`: 서랍 열쇠를 집고 서랍을 열지 않을 때 서랍이 빛나기까지의 시간과 세기. 0이면 끔.
- `ItemToastSeconds` (2.5초): "…획득!" 문구와 아이콘 깜빡임 시간. `MinMouseSensitivity` / `MaxMouseSensitivity` (0.1 / 1.5): 메뉴의 마우스 감도 범위.
- `BuildVersion` (v5) / `BuildName`: 세션 시작 때 `build_info`로 기록돼, CSV마다 어느 빌드로 플레이했는지 남는다.
- `SafeCode`가 액자 물체 수와 다르면 Play 시 경고가 화면/로그에 뜬다.

**태그로 힌트 대상과 조명 지정**
- 액터에 태그를 달면 액터 전체가, 컴포넌트에 달면 그 부분만 빛난다.
- 기본값: 메모 `P1H1`, 액자 날짜 라벨(컴포넌트) `P1H2`, 램프(컴포넌트) `P2H1`, 파랑·노랑 물감 `P2H2`, 액자 `P3H1`·`P3H2`(왼쪽 다이얼부터 짝이 되는 그림이 하나씩 순차로).
- 조명: `Light_P1`/`Light_P2`/`Light_P3` 태그가 붙은 라이트는 해당 퍼즐일 때만 원래 밝기, 아니면 `InactiveLightScale` 배.

**물체별**: 금고 `SilhouetteSize`(모든 실루엣 크기, 12×15cm), `SilhouetteColor`(실루엣 테두리) / 액자 `DateLabel`, `LabelTextSize`(날짜 글자 높이, 10cm), `FrameSize`, `ObjectCount`, `CanvasColor`(바탕색), `ObjectColor`(물체 색), `LiftSeconds` / 물감 `PaintId`, `ColorName`, `PaintColor` / 문 `LampColor` / 모든 물체 `ObjectId`(로그 이름), `InspectMessage`, `PuzzleIds` / 캐릭터 `InteractDistance`(250), `FocusHighlight`, `InteractCooldownSeconds`(0.2초), `MouseSensitivity`(0.5), `WalkSpeed`.

## 기록되는 이벤트 (CSV `event`, `params`)
| 이벤트 | 파라미터 | 시점 |
|---|---|---|
| `session_start` / `session_end` | reason, duplicates_suppressed | 플러그인 자동. 탈출 시 reason=Escaped, 메뉴에서 종료 시 reason=Quit |
| `build_info` | version, build | 세션 시작 직후. 어느 빌드로 플레이했는지 |
| `puzzle_started` | puzzle | 퍼즐 시작 |
| `puzzle_solved` | puzzle, attempts | 정답. attempts = 정답 포함 답 입력 횟수 |
| `wrong_attempt` | puzzle, input | 오답 (액자 id / "red+blue" / "0000") |
| `hint_shown` | puzzle, level, source | 힌트 1·2. source=request(H) / auto(안전망) |
| `item_found` | item | 물건이 보이게 됐을 때 (drawer_key / palette / door_key) |
| `item_picked` | item | 물건을 E로 집었을 때 |
| `nudge_shown` | target | 서랍이 빛나기 시작할 때 (target=drawer, 힌트로 세지 않음) |
| `object_inspected` | object | 물체에 E (memo, drawer, paint_blue, lamp ...) |
| `menu_opened` / `menu_closed` | puzzle / paused_sec | Esc 메뉴를 열고 닫을 때. 멈춰 있던 시간 |
| `setting_changed` | key, value | 메뉴에서 마우스 감도를 바꿨을 때 (key=mouse_sensitivity) |
| `escaped` | total_sec, hints_used | 탈출 직후 EndSession("Escaped") |

## 자동 플레이 검증
```
UnrealEditor.exe SessionAnalyticsDemo.uproject /Game/EscapeRoom/L_Studio -game -windowed -ResX=1280 -ResY=720 -ExecCmds="Escape.Autoplay 4 quit"
```
실제 E 트레이스로 오답·힌트 요청·메뉴·정답을 모두 거쳐 탈출하고, `Saved/Screenshots/EscapeRoom/`에 단계별 스크린샷, `Saved/Analytics/`에 CSV를 남긴다. 숫자 인자는 테스트용 힌트 지연(초), `quit`은 탈출 뒤 메뉴로 종료, `topview`는 평면도 한 장만 찍는다. 게임 중 `~` 콘솔에서도 실행 가능. Shipping 빌드에는 포함되지 않는다.

플레이테스트 기록은 `python Scripts/analyze_playtest.py Docs/Playtest/data/v5`처럼 버전 폴더를 넣어 요약한다 (메뉴로 멈춘 시간은 빼고 계산).

## 레벨 다시 만들기
`Scripts/create_escape_room.py`가 머티리얼, BP, L_Studio를 만든다 (에디터를 닫고 실행, 레벨은 새로 만들어짐 → 레벨에서 손으로 바꾼 값은 사라짐):
```
UnrealEditor-Cmd.exe SessionAnalyticsDemo.uproject -run=pythonscript -script="<절대경로>/Scripts/create_escape_room.py" -unattended
```
