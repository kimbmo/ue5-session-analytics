# 방탈출 프로토타입 (L_Studio)

화가의 작업실에서 3분 안에 나가는 1인칭 방탈출. 모든 플레이는 SessionAnalytics 플러그인으로 `Saved/Analytics/*.csv`에 기록된다.

## 플레이 방법
- 에디터에서 `Content/EscapeRoom/L_Studio`를 열고 Play (기본 맵으로도 설정돼 있음). 뷰포트를 한 번 클릭해야 마우스가 잡힌다.
- **WASD** 이동, **마우스** 시점, **E** 상호작용 (카메라에서 250cm 라인 트레이스). 메시지는 2초 동안 화면에 뜬다.
- 퍼즐은 순서대로, 현재 퍼즐만 활성화된다.
  1. **찾기**: 책상 메모 "열쇠는 늘 마지막 작품 뒤에" → 날짜 라벨(YYYY.MM)이 가장 늦은 액자를 E로 들면 서랍 열쇠. 다른 액자를 들면 오답.
  2. **조합**: 서랍을 열면 팔레트. 물감 두 개를 차례로 E → 파랑+노랑 = 문 위 램프의 초록이면 정답. 금고 덮개가 사라지고 조명이 바뀐다.
  3. **순서**: 그림 속 물체 개수를 작은 액자부터 큰 액자 순으로 읽은 4자리(기본 3715)를 금고 다이얼(E로 0-9 돌림)에 맞추고 손잡이 E → 문 열쇠. 문에 E → 탈출.
     - 다이얼은 금고 앞에서 볼 때 왼쪽부터 1·2·3·4번째 자리.
     - 순서 단서(글자 없음): 각 다이얼 위에 액자 모양 실루엣이 있다. 왼쪽부터 점점 커지고, 비율이 실제 액자와 같아서 "이 다이얼엔 이 액자의 개수"로 읽힌다. 실루엣 크기는 레벨의 액자(`FrameSize`)에서 자동으로 계산된다.

정답: 2024.02 액자 → 파랑+노랑 → 3715 → 문.

## 클래스 역할 (Source/SessionAnalyticsDemo/EscapeRoom)
| 클래스 | 역할 |
|---|---|
| `AEscapeRoomManager` | 퍼즐 진행(현재 퍼즐, 시도 횟수), 힌트 타이머, 조명, 소지품, 분석 이벤트 기록. 레벨에 1개. **튜닝 값은 전부 여기 Details 패널** |
| `AEscapeInteractable` | E로 상호작용 가능한 모든 물체의 부모. 기본은 "살펴보기"(object_inspected 기록 + 메시지). 메모·금고 덮개가 이 클래스 그대로 |
| `AEscapeFrame` | 액자. 날짜 라벨, 크기, 그림 속 물체 수. 퍼즐1에선 들어 올리기, 퍼즐3에선 관찰 대상 |
| `AEscapeDesk` | 책상 + 잠긴 서랍(안에 팔레트) |
| `AEscapePaint` | 물감 통. PaintId(로그용)와 색 |
| `AEscapeEasel` | 이젤. 빈 칸이 퍼즐2 정답 후 초록으로 채워짐 |
| `AEscapeSafe` | 다이얼 4개 + 손잡이 금고. 다이얼 위 액자 실루엣(순서 단서) |
| `AEscapeDoor` | 출구 문 + 초록 램프 |
| `AEscapeCharacter` | 1인칭 이동, E 트레이스, 크로스헤어 아래 프롬프트 |
| `AEscapeHUD` | Slate 화면 표시: 크로스헤어, 프롬프트, 메시지, 퍼즐/시간, 소지품, 팔레트 |
| `AEscapeGameMode` | 위 캐릭터와 HUD 사용 |
| `EscapeAutoplay.cpp` | 개발용 콘솔 명령 `Escape.Autoplay` (아래) |

로직은 C++, `Content/EscapeRoom/Blueprints`의 BP_*는 C++을 그대로 상속한 얇은 블루프린트(값 조정용)다.
게임플레이 코드는 `Manager->RecordEvent`만 부르고, 파일 쓰기·중복 제거는 플러그인이 맡는다.

## 튜닝 값 (코드 수정 없이 Details 패널)
**EscapeRoomManager**
- `HintDelay1` 30초 / `HintDelay2` 60초: 진행 없이 이 시간이 지나면 힌트 1 / 2. "진행" = 현재 퍼즐에 속한 물체를 살펴보거나 답을 입력(다이얼 돌리기 포함).
- `HintGlowStrength`, `HintPulseSpeed`, `SequenceStepSeconds`: 힌트 빛의 세기, 깜빡임 속도, 순차 힌트 간격.
- `Puzzles[0..2]`: 퍼즐별 힌트 태그(`Hint1Tag`/`Hint2Tag`), 조명 태그(`LightTag`), 힌트2 순차 여부, 힌트 문구.
- `CorrectPaintA/B` (blue/yellow), `SafeCode` (3715), `SafeCover`, `InactiveLightScale` (다른 퍼즐 조명 밝기 비율), `MessageSeconds` (2초).
- `SafeCode`가 액자 물체 수와 다르면 Play 시 경고가 화면/로그에 뜬다.

**태그로 힌트 대상과 조명 지정**
- 액터에 태그를 달면 액터 전체가, 컴포넌트에 달면 그 부분만 빛난다.
- 기본값: 메모 `P1H1`, 액자 날짜 라벨(컴포넌트) `P1H2`, 램프(컴포넌트) `P2H1`, 파랑·노랑 물감 `P2H2`, 액자 `P3H1`·`P3H2`(작은 것부터 순차).
- 조명: `Light_P1`/`Light_P2`/`Light_P3` 태그가 붙은 라이트는 해당 퍼즐일 때만 원래 밝기, 아니면 `InactiveLightScale` 배.

**물체별**: 금고 `SilhouetteMaxSize`(가장 큰 실루엣 크기), `SilhouetteColor` / 액자 `DateLabel`, `FrameSize`, `ObjectCount`, 색 / 물감 `PaintId`, `ColorName`, `PaintColor` / 문 `LampColor` / 모든 물체 `ObjectId`(로그 이름), `InspectMessage`, `PuzzleIds` / 캐릭터 `InteractDistance`(250), `MouseSensitivity`, `WalkSpeed`.

## 기록되는 이벤트 (CSV `event`, `params`)
| 이벤트 | 파라미터 | 시점 |
|---|---|---|
| `session_start` / `session_end` | reason, duplicates_suppressed | 플러그인 자동. 탈출 시 reason=Escaped |
| `puzzle_started` | puzzle | 퍼즐 시작 |
| `puzzle_solved` | puzzle, attempts | 정답. attempts = 정답 포함 답 입력 횟수 |
| `wrong_attempt` | puzzle, input | 오답 (액자 id / "red+blue" / "0000") |
| `hint_shown` | puzzle, level | 힌트 1·2 |
| `object_inspected` | object | 물체에 E (memo, drawer, paint_blue, lamp ...) |
| `escaped` | total_sec, hints_used | 탈출 직후 EndSession("Escaped") |

## 자동 플레이 검증
```
UnrealEditor.exe SessionAnalyticsDemo.uproject /Game/EscapeRoom/L_Studio -game -windowed -ResX=1280 -ResY=720 -ExecCmds="Escape.Autoplay 4 quit"
```
실제 E 트레이스로 오답·힌트·정답을 모두 거쳐 탈출하고, `Saved/Screenshots/EscapeRoom/`에 스크린샷 16장, `Saved/Analytics/`에 CSV를 남긴다. 숫자 인자는 테스트용 힌트 지연(초). 게임 중 `~` 콘솔에서도 실행 가능. Shipping 빌드에는 포함되지 않는다.

## 레벨 다시 만들기
`Scripts/create_escape_room.py`가 머티리얼, BP, L_Studio를 만든다 (에디터를 닫고 실행, 레벨은 새로 만들어짐 → 레벨에서 손으로 바꾼 값은 사라짐):
```
UnrealEditor-Cmd.exe SessionAnalyticsDemo.uproject -run=pythonscript -script="<절대경로>/Scripts/create_escape_room.py" -unattended
```
