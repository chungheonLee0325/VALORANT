# Multiplayer UI Regression

상점·HUD 리팩터링의 동작을 실제 멀티플레이 프로세스에서 확인하는 회귀 검사다. 결과 기준 커밋은 17d28b6f이다.

## 구성

UE 5.5.4 Win64 Development Editor에서 실제 MatchMap을 호스트 1개와 원격 3개로 실행한다. 루프백 Listen Server로 연결하고, 네 요원이 확정된 뒤 구매·장비·UI 검사를 시작한다. 입력은 Slate·Enhanced Input 이벤트로 주입한다. 화면 캡처용 원격 하나만 DX12 오프스크린으로 렌더링하고 나머지는 NullRHI다.

검사 도구는 WITH_DEV_AUTOMATION_TESTS가 켜진 비 Shipping 빌드에서만 컴파일된다. 실행 플래그와 루프백 연결이 함께 있을 때만 시험 상태를 바꾼다. 실행 스크립트는 [Run-UIRegression.ps1](../../UnrealEngine/Valorant/Tools/Run-UIRegression.ps1), 필수 검사 목록은 [UIRegressionChecks.json](../../UnrealEngine/Valorant/Tools/UIRegressionChecks.json)이다. 공개 소스 미러에는 필수 WBP와 MatchMap이 없어 원본 프로젝트에서 실행해야 한다.

## 판정

모든 필수 검사가 지정한 프로세스에서 정확히 한 번 PASS이고 네 프로세스가 판정까지 살아 있어야 통과다. 실패, 누락, 중복, 알 수 없는 ID, 비정상 종료는 모두 실패로 끝난다.

## 실패 주입

- 무기 생성 거부와 부착 실패를 일으킨 뒤 기존 장비, 잔액, 월드 무기의 소유권·물리·충돌·Actor 수가 그대로인지 비교한다.
- 원격의 구매 결과 전달을 보류해 10초 타임아웃을 낸 뒤 서버의 실제 지급과 새 요청 상태를 확인한다.
- 이미 처리했거나 더 오래된 요청 번호를 보내 잔액이 바뀌지 않는지 확인한다.

## 결과

[results.json](Evidence/results.json) 기준 52개 필수 검사가 모두 PASS다. 대표 로그는 [regression-results.txt](Evidence/regression-results.txt)에 있다.

| 항목 | 기대 결과 |
| --- | --- |
| 구매·환불 | 서버 판정과 실제 슬롯·잔액이 일치하고 최대 방어구·스킬을 거부한다. |
| 월드 무기 | 실패 시 소유권·물리·충돌·기존 슬롯을 보존하고 재획득한다. |
| 표시 수명 | 교체·드롭·사망에서 종속 Actor가 누적되지 않는다. |
| 세션·응답 | 늦은 결과가 새 요청을 끝내지 않고, 타임아웃 뒤 실제 지급을 유지한다. |
| UI·입력 | Back·상점 키·LB/RB와 모달 확인·취소가 CommonUI 액션으로 동작하고, 입력 방식에 따라 키 아이콘이 바뀐다. 모달 취소 후 포커스가 돌아오며 메뉴 중 시작 입력과 누른 키 유출을 막는다. |
| HUD | 발사·재장전·스킬 소모·피격·킬 피드·HUD 재생성을 반영한다. |

![게임패드 입력의 구매 확인](Evidence/shop-confirmation.png)

![키보드·마우스 입력의 구매 확인](Evidence/shop-confirmation-keyboard.png)

## 실행하지 않은 조건

실물 Xbox·PlayStation 컨트롤러, 물리 키보드·마우스 혼용, Win64 패키지, Steam·전용 서버, 실제 네트워크 지연·손실, 독립 PlayerState·ASC 교체와 ServerTravel은 NOT RUN이다. 입력 주입 결과는 로컬 재현 범위다.

키 아이콘 전환은 입력 방식을 코드로 바꿔 확인했고, 아이콘은 표시된 텍스처 이름으로 판정했다. Enter·Esc·B(상점 키)·View 키 입력은 주입하지 않았다. Windows 외 패드 이름에서의 아이콘 선택과 패키지 빌드의 입력 데이터 쿡은 확인하지 않았다.
