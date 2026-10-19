# v4 변경 사항 (두 번째 PC에서 다시 만듦, 2026-10-19)

첫 번째 PC에서 만든 v3/v4는 push되지 않아, 같은 규칙과 값으로 main(v2, `d60b3ff`)에서 다시 만들었다.
엔진 UE 5.8.3, VS 2022 Professional 17.14. 빌드 구분: CSV 첫 줄 다음의 `build_info`가 `build=work-pc;version=v4`.

| 영역 | v2 | v4 (work-pc) |
|---|---|---|
| 퍼즐 1 | - | 변경 없음 |
| 퍼즐 2 물감 | 빨강·파랑·노랑 3개(조합 3) | 빨강·파랑·하양·노랑·검정 5개(조합 10). 파랑과 노랑은 이웃하지 않음. 정답(파랑+노랑), 단서, 힌트 2(파랑·노랑이 빛남), 오답 기록 방식은 그대로 |
| 퍼즐 3 연결 단서 | 실루엣 크기 = 액자 크기, 안쪽은 바탕색 | 색이 연결 고리. 다이얼 뒤에 그 그림의 바탕색 판, 다이얼 위에 같은 바탕색 실루엣 + 그림 속 물체 색의 점 2개(빨간 공 그림 → 빨간 점). 실루엣 크기는 모두 같음(12×15cm), 크기 화살표 없음, 다이얼 간격 13 → 16cm. 코드 3715와 다이얼 → 그림 대응은 그대로 |
| 퍼즐 3 힌트 문구 | "그림 속에 그려진 것들을 세어 보자." / "작은 액자부터 차례로 빛난다." | "다이얼 뒤 색과 같은 바탕의 그림을 찾아, 그 안에 그려진 것을 세어 보자." / "왼쪽 다이얼부터 차례로, 짝이 되는 그림이 하나씩 빛난다." (힌트 2: 다이얼 순서대로 그림이 하나씩 빛남) |
| 조명 | 액자 60, 책상 45, 이젤 50, 램프 25, 금고 50. 비활성 퍼즐 조명 0.08배 | 모두 절반(30 / 22.5 / 25 / 12.5 / 25). 비활성 퍼즐 조명 0.6배. 천장 조명 12, 노출 고정(EV 3.6) 유지 |
| 힌트 문구 버그 | 퍼즐이 바뀌어도 이전 퍼즐 힌트 문구가 남음 | 퍼즐이 바뀌는 순간 지움 |
| 힌트 방식 | 30초/60초 자동 | H를 누르면 현재 퍼즐의 다음 단계(1 → 2). 오른쪽 위에 항상 "[H] 힌트 n/2". 자동 힌트는 안전망으로만 90초/150초(실제 진행 없을 때, `HintDelay1/2`로 조절). `hint_shown`에 `source=request|auto` 추가 |
| 팔레트 | 납작한 상자 | 타원형 판 + 엄지 구멍 + 물감 5색 덩어리(기본 도형) |
| 소지품 표시 | 왼쪽 위 글자 | 왼쪽 아래 아이콘: 서랍 열쇠, 팔레트, 고른 물감 2칸, 문 열쇠 |
| Esc | 바로 종료 | 메뉴: 계속하기, 마우스 감도 슬라이더 0.1~1.5, 게임 종료 → "정말 종료할까요?". 키: Esc 계속, Q 종료, Y/N 확인. 메뉴가 열려 있으면 게임·힌트 타이머 정지 |
| 새 이벤트 | - | `build_info{version,build}`, `menu_opened{puzzle}`, `menu_closed{paused_sec}`, `setting_changed{key=mouse_sensitivity,value=0.70}`. 메뉴에서 종료하면 `session_end reason=Quit`. 나머지 이벤트 이름과 파라미터는 v2와 같음 |

플레이테스터 안내: `Docs/Playtest/플레이테스트_안내_v4.txt` (조작에 H 힌트, Esc 메뉴, "메모할 땐 Esc 메뉴를 열어 두세요" 추가. 스포일러 없음).

## 확인 결과

- 에디터 빌드(Development) 성공, 경고 없음.
- 플러그인 테스트 7개 모두 통과: CsvEscape, FlushThreshold, ParamEscape, ParamOrderIndependence, RecordAgainAfterWindow, RecordAndDuplicate, SessionRestart.
- `Escape.Autoplay 4 quit`: 탈출 성공(30.0초, 힌트 6회). CSV에 `build_info`, `hint_shown source=request/auto`, `menu_opened` / `setting_changed value=0.70` / `menu_closed paused_sec=1.9`, 오답 `red+blue`, `white+black`. 퍼즐 1 → 2로 바뀐 직후 힌트 문구 표시 여부 = 0(지워짐). 트레이스 실패 없음.
- 밝기(같은 카메라, 각 단계 시작 직후, UI 제외 스크린숏 평균 휘도): P1 153.6 → P2 150.9(-1.8%) → P3 161.1(+6.8%) → 문 160.0(-0.7%). 모든 단계에서 포화(≥250) 픽셀 0%.
- Shipping 패키지(BuildCookRun, Win64 Shipping) 성공. 경고는 날짜 라벨 FText 쿡 경고 4개뿐(동작 영향 없음).
- Shipping exe 1회 실행(키 입력 H → Esc → Esc → Esc → Q → Y): CSV가 `%LOCALAPPDATA%\SessionAnalyticsDemo\Saved\Analytics`에 생성됨.
  `build_info build=work-pc;version=v4`, `hint_shown level=1;puzzle=1;source=request`, `menu_opened`/`menu_closed paused_sec=3.3`,
  `menu_opened`/`menu_closed paused_sec=4.6`, `session_end reason=Quit`.

빌드: `Packaged/EscapeRoom_Playtest_v4.zip` (244.9MB, 루트에 `SessionAnalyticsDemo.exe`와 `플레이테스트_안내.txt`).
