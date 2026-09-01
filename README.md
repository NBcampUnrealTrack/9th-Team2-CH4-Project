# BARUGame (Project BARU)

> **Unreal Engine 5.8 기반의 Listen Server 멀티플레이어 익스트랙션 호러 게임**

---

## 1. 프로젝트 개요 (Overview)

* **게임 타이틀:** B.A.R.U (Buried Asset Recovery Unit, 매몰 자산 회수단)
* **장르:** 멀티플레이어 익스트랙션 호러 (Listen Server)
* **타깃 시점:** 1인칭(FPS) + 3인칭
* **개발 엔진:** Unreal Engine 5.8
* **주요 타깃:** PC (Windows 64-bit)
* **네이티브 모듈 / API 접두사:** `BARUGAME_API` / `Baru`

---

## 2. 개발 환경 & 기술 스택 (Tech Stack)

| 구분 | 사양 및 도구 | 비고 |
| :--- | :--- | :--- |
| **Engine** | Unreal Engine 5.8 | 소스 빌드 / 런처 빌드 통일 |
| **IDE** | JetBrains Rider | `RiderUprojectSourceCodeAccessor` 표준화 |
| **Network** | Listen Server + OnlineSubsystem Steam | Steam 기반 세션 생성·검색·참가 |
| **Core Framework**| Gameplay Ability System (GAS) | 스킬, 스탯, Buff/Debuff 네트워크 동기화 |
| **UI System** | CommonUI + MVVM | C++ 백엔드와 UI 의존성 완전 분리 |
| **VCS** | Git + Git LFS + GitHub Desktop | 바이너리 에셋 LFS 추적 및 Lock 관리 |

---

## 3. 시작하기 (Getting Started)

### 사전 필수 설치 (Prerequisites)
1. **Git LFS 설치:** 터미널에서 `git lfs install` 실행 필수
2. **Unreal Engine 5.8** 설치
3. **JetBrains Rider** 및 UnrealLink 플러그인 설치

### 프로젝트 설치 및 에디터 실행
```bash
# 1. 원격 레포지토리 클론 (Git LFS 에셋 자동 다운로드)

# 2. develop 브랜치로 전환
git checkout dev
```
* `BARUGame.uproject`를 우클릭하거나 JetBrains Rider에서 `.uproject`를 직접 열어 C++ 프로젝트를 빌드합니다.
* 에디터 멀티플레이 테스트는 **Listen Server + Multi Client** 구성을 사용합니다.

---

## 4. Git 협업 및 에셋 관리 수칙 (Git Rules)

### 브랜치 전략 (Git Flow 변형)
* `main`: 상시 패키징 빌드가 검증된 최종 안정 브랜치 (직접 Push 금지, PR 필수)
* `develop`: 팀원들의 기능이 통합되는 메인 개발 브랜치
* `feat/[파트]-[기능명]`: 개별 기능 및 캐릭터 전담 브랜치
  * 예: `code/feat/hero-tanker`, `content/feat/framework-core`

### 커밋 메시지 컨벤션 (Commit Convention)
```text
[태그] 작업 요약 한 줄 (명문화된 동사 사용)

- 세부 변경 사항 1
- 세부 변경 사항 2
```
* `[Feat]`: 새로운 기능, 캐릭터 스킬, C++ 클래스 추가
* `[Fix]`: 버그 수정 및 네트워크 동기화 오류 해결
* `[Refactor]`: 로직 개선 및 코드 리팩토링 (기능 변경 없음)
* `[Chore]`: 빌드 스크립트, Config, `.gitignore` 등 인프라 설정 수정
* `[Asset]`: 애니메이션, 메쉬, 머티리얼 등 바이너리 에셋 추가/수정

### 바이너리 에셋 충돌 방지 3대 철칙
1. **Directory Isolation (폴더 격리):**
   * `/Content/Characters/HeroA/`와 같이 개인 전담 폴더 내의 에셋만 수정합니다.
   * 타인의 캐릭터 폴더나 공용 폴더(`/Content/_Shared/`) 에셋을 무단 수정하지 않습니다.
2. **Git LFS Lock:**
   * 공용 머티리얼(`M_Master`), 공용 레벨(`.umap`) 수정 필요 시 반드시 사전 공유 및 `git lfs lock`을 수행합니다.
3. **Data-Driven Design:**
   * C++ 및 블루프린트 하드코딩을 금지하며, 수치 밸런싱은 `DataAsset` 및 `DataTable`로 분리하여 관리합니다.

---

## 5. 네트워크 & 아키텍처 규약 (Network & Architecture)

### 1) 데이터 주권자 (Data Sovereign) 정의
* 모든 게임플레이 판정(체력 차감, 스킬 피격 판정, 사망/리스폰)은 **Server Authority (서버 전속 권한)**로 수행합니다.
* 클라이언트는 **Client-side Prediction(예측 연출)**만 수행하며, 서버의 확정 상태를 복제(Replicate)받습니다.

### 2) 백엔드-UI 분리 (`UGameplayMessageSubsystem`)
* C++ 백엔드 로직에서 UI 위젯 클래스를 직접 참조(`Cast<UUserWidget>`)하는 것을 전면 금지합니다.
* C++ 백엔드는 `GameplayTag` 기반으로 무전(Broadcast)을 발송하고, UI는 해당 신호를 구독하여 화면을 갱신합니다.

### 3) 로깅 표준 (`BaruLog.h`)
* 일반 `UE_LOG` 대신 전용 로그 매크로를 사용하여 서버/클라이언트 출처와 NetRole을 구분합니다.
* `SERVER_LOG(...)`, `CLIENT_LOG(...)` 매크로 사용 준수.

### 4) CommonUI 기반 UI 구조

BARU의 UI는 화면마다 `AddToViewport()`를 직접 호출하는 방식 대신,
Lyra의 CommonUI 구조를 참고한 **Primary Game Layout + GameplayTag Layer Stack** 방식으로 관리합니다.

```text
Local Player
    └─ UBaruUIManagerSubsystem
          └─ UBaruPrimaryGameLayout
                ├─ UI.Layer.Game
                ├─ UI.Layer.GameMenu
                ├─ UI.Layer.Menu
                └─ UI.Layer.Modal
```

각 레이어는 `UCommonActivatableWidgetStack`이며, 같은 레이어에 위젯을 Push하면
가장 최근에 추가한 위젯이 맨 위에서 활성화됩니다. Pop하면 맨 위 위젯이 닫히고
그 아래 위젯으로 돌아갑니다.

| Gameplay Tag | 용도 | 사용 예시 |
| :--- | :--- | :--- |
| `UI.Layer.Game` | 게임 플레이 중 항상 표시하는 UI | 메인 HUD, 체력, 탄약, 조준점 |
| `UI.Layer.GameMenu` | 게임 중 단축키로 열고 닫는 UI | 인벤토리, 상태창 |
| `UI.Layer.Menu` | 게임 밖의 전체 화면 메뉴 | 타이틀, 로비, 메인 메뉴 |
| `UI.Layer.Modal` | 다른 UI보다 위에 표시할 팝업 | 확인창, 경고창 |

팀원이 UI를 열거나 닫을 때는 `UBaruPrimaryGameLayout`이나 Layer Stack을 직접
찾지 않고, 로컬 플레이어의 `UBaruUIManagerSubsystem`을 공용 진입점으로 사용합니다.

#### C++에서 UI 열기

```cpp
ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
if (!IsValid(LocalPlayer))
{
    return;
}

UBaruUIManagerSubsystem* UIManager =
    LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>();

if (IsValid(UIManager))
{
    UIManager->PushWidgetToLayer(
        /* 태그 작성 */,
        WidgetClass);
}
```

* `LayerTag`: 위젯을 표시할 레이어의 GameplayTag입니다.
* `WidgetClass`: `WBP_Inventory`, `WBP_Lobby`처럼 실제 디자인이 들어 있는
  Widget Blueprint 클래스입니다.
* `PushWidgetToLayer()`는 클래스만 보관하는 함수가 아니라, 지정된 레이어에
  실제 위젯 인스턴스를 생성하고 활성화합니다.

#### C++에서 UI 닫기

```cpp
if (IsValid(UIManager))
{
    UIManager->PopWidgetFromLayer(
        /* 태그 작성 */);
}
```

`PopWidgetFromLayer()`는 해당 레이어의 맨 위에 있는 위젯을 닫습니다.
따라서 열 때와 닫을 때 같은 레이어 태그를 사용해야 합니다.

#### Blueprint에서 UI 열기·닫기

`PushWidgetToLayer()`와 `PopWidgetFromLayer()`는 `BlueprintCallable`로 공개되어
있으므로 Blueprint에서도 동일한 흐름으로 사용할 수 있습니다.

```text
Input Action 또는 Button Event
    → Get Local Player Subsystem
        Class: Baru UI Manager Subsystem
    → Push Widget To Layer
        Layer Tag: 태그 작성
        Widget Class: 표시할 WBP 클래스
```

닫을 때는 동일한 Subsystem에서 `Pop Widget From Layer`를 호출합니다.
여기서 “Blueprint로 UI를 불러온다”는 표현은 보통 C++ 기반 위젯 클래스를
직접 생성한다는 뜻이 아니라, `WBP_Inventory` 같은 Widget Blueprint 클래스를
`Widget Class` 입력으로 전달해 UI Manager가 생성하도록 요청한다는 뜻입니다.

### 5) HUD와 초기 UI 생성 흐름

`ABaruHUD`는 로컬 플레이어의 Primary Game Layout을 만들고, 맵에 진입했을 때
처음 보여줄 위젯을 지정된 레이어에 추가합니다.

HUD Blueprint의 Class Defaults에서 다음 값을 설정합니다.

| 설정 | 역할 |
| :--- | :--- |
| `PrimaryGameLayoutClass` | 모든 UI 레이어를 가진 `WBP_PrimaryGameLayout` |
| `InitialWidgetClass` | 진입 직후 표시할 Widget Blueprint |
| `InitialWidgetLayerTag` | 초기 위젯을 Push할 UI 레이어 |

예시:

| 화면 | InitialWidgetClass | InitialWidgetLayerTag |
| :--- | :--- | :--- |
| 타이틀 | `WBP_Title` | `UI.Layer.Menu` |
| 로비 | `WBP_Lobby` | `UI.Layer.Menu` |
| 게임 플레이 | `WBP_MainHUD` | `UI.Layer.Game` |

실제 호출 순서는 다음과 같습니다.

```text
ABaruHUD::BeginPlay()
    → 로컬 PlayerController인지 확인
    → WBP_PrimaryGameLayout 인스턴스 생성
    → Player Screen에 PrimaryGameLayout 추가
    → UBaruUIManagerSubsystem에 Layout 등록
    → InitialWidgetClass를 InitialWidgetLayerTag에 Push
    → 초기 Widget 활성화
```

`ABaruHUD::EndPlay()`에서는 UI Manager가 제거된 Layout을 계속 참조하지 않도록
먼저 등록을 해제하고, PrimaryGameLayout을 화면에서 제거합니다.

### 6) C++ 기반 클래스와 Widget Blueprint의 역할 분리

UI 클래스는 다음과 같이 역할을 나눕니다.

```text
C++ Widget 클래스
    ├─ 버튼 이벤트 연결
    ├─ Subsystem 및 Delegate 연결
    ├─ 화면 상태와 데이터 흐름 관리
    └─ 공통 동작 제공

Widget Blueprint
    ├─ 버튼, 텍스트, 이미지 배치
    ├─ 크기, 색상, 애니메이션 설정
    └─ C++의 BindWidget 이름에 맞는 실제 위젯 제공
```

예를 들어 `UBaruLobbyWidget`은 동작을 담당하고, 이를 부모로 만든 `WBP_Lobby`는
실제 화면 디자인을 담당합니다. `meta = (BindWidget)`으로 선언된 C++ 변수는
Widget Blueprint Hierarchy에 이름과 타입이 정확히 일치하는 위젯이 필요합니다.

```cpp
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
TObjectPtr<UButton> Button_CloseContract;
```

위 선언이 있다면 `WBP_Lobby` 안에 `Button_CloseContract`라는 정확한 이름의
Button이 있어야 합니다. 대소문자, 밑줄 또는 위젯 타입이 다르면 Blueprint
컴파일 단계에서 Required Widget Binding 오류가 발생합니다.

### 7) 로비 UI 화면 전환

`WBP_Lobby`는 로비 관련 화면을 각각 별도 위젯으로 생성·제거하지 않고,
하나의 `WidgetSwitcher` 안에 배치한 뒤 활성 패널을 변경합니다.

```text
Switcher_LobbyView
    ├─ Panel_LobbyHome
    ├─ Panel_ContractSelection
    ├─ Panel_SearchLoading
    └─ Panel_SearchResults
```

`EBaruLobbyView`는 현재 표시할 화면을 구분합니다.

| 값 | 표시 화면 |
| :--- | :--- |
| `Home` | 기본 로비와 파티 정보 |
| `ContractSelection` | 계약 목록과 선택 계약 상세 정보 |
| `SearchLoading` | 생성된 방 검색 중 화면 |
| `SearchResults` | 세션 검색 결과 화면 |

`ShowLobbyView()`는 enum 값을 실제 패널로 변환하고 `SetActiveWidget()`을 호출한 뒤,
현재 화면을 `CurrentLobbyView`에 기록합니다.

```text
로비 활성화
    → Home

계약 선택 버튼
    → ContractSelection
    → 뒤로 가기
    → Home

세션 검색 버튼
    → SearchLoading
    → 검색 완료 Delegate 수신
    → SearchResults
    → 뒤로 가기
    → Home
```

### 8) 로비 세션 검색 데이터 흐름

`UBaruLobbyWidget`은 UI가 활성화될 때 `UGameInstance`에서
`UBaruSessionSubsystem`을 가져옵니다. 사용자가 세션 검색 버튼을 누르면
검색 화면을 먼저 표시하고 `FindSessions(50, false)`를 호출합니다.

```text
Button_FindSession 클릭
    → Panel_SearchLoading 표시
    → UBaruSessionSubsystem::FindSessions(50, false)
    → OnlineSubsystem이 비동기로 세션 검색
    → OnFindSessionsCompleteEvent Broadcast
    → UBaruLobbyWidget::HandleFindSessionsComplete()
    → 결과 배열을 SessionSearchResults에 저장
    → Panel_SearchResults 표시
```

`bWasSuccessful == true`이고 결과 개수가 0개라면 오류가 아니라,
검색 요청은 성공했지만 현재 참가 가능한 BARU 세션이 없다는 의미입니다.

현재 구현은 검색 결과를 `SessionSearchResults`에 저장하고 결과 화면으로
전환하는 단계까지 포함합니다. 실제 방 목록을 동적으로 표시하고 선택한 방에
참가하는 UI는 이후 `ListView`와 세션 참가 기능을 연결해야 합니다.

### 9) UI Lifecycle과 Delegate 수명

Widget의 호출 시점은 다음과 같습니다.

| 함수 | 호출 시점 | 현재 역할 |
| :--- | :--- | :--- |
| `NativeOnInitialized()` | Widget 인스턴스가 초기화될 때 한 번 | 내부 Button `OnClicked` 연결 |
| `NativeOnActivated()` | CommonUI Stack에서 활성화될 때 | Subsystem 획득, 외부 Delegate 등록, Home 화면 초기화 |
| `NativeOnDeactivated()` | Stack에서 비활성화될 때 | 외부 Delegate 해제 및 Subsystem 참조 정리 |

버튼은 Widget 내부 요소이고 Widget과 수명이 같으므로 초기화 시 연결합니다.
반면 `UBaruSessionSubsystem`은 `GameInstanceSubsystem`이라 Widget보다 오래 살아남습니다.
따라서 Widget이 비활성화될 때 검색 완료 Delegate를 반드시 해제해야 합니다.
해제하지 않으면 닫힌 Widget을 대상으로 콜백이 실행되거나, 다시 활성화할 때
동일한 콜백이 중복 등록될 수 있습니다.

재활성화 시에는 안전하게 다음 순서로 연결합니다.

```text
기존 Delegate RemoveDynamic
    → Delegate AddDynamic
    → 비활성화 시 RemoveDynamic
    → SessionSubsystem 참조를 nullptr로 정리
```

### 10) 현재 로비 UI 구현 범위와 다음 작업

현재 완료된 범위:

- 로비 Home, 계약 선택, 검색 로딩, 검색 결과 패널 구성
- `WidgetSwitcher` 기반 화면 전환
- 계약 선택 화면 열기·닫기
- 세션 검색 요청 및 완료 Delegate 수신
- 검색 결과 배열 보관
- Widget 비활성화 시 외부 Delegate 해제
- 계약 선택 화면의 목록·상세 정보·하단 버튼 기본 배치

아직 연결이 필요한 범위:

- 계약 데이터를 DataAsset 또는 담당 시스템에서 전달받기
- 계약 목록을 `ListView`로 동적 생성하기
- 선택된 계약의 이름·이미지·설명·보상을 실시간 갱신하기
- 선택 계약을 Server RPC로 요청하고 GameState의 복제 결과 표시하기
- 검색된 세션을 `ListView`에 표시하고 선택한 세션에 참가하기
- 방장만 계약 선택 및 게임 시작을 요청할 수 있도록 권한 검증하기

디자이너에 입력한 계약 이름과 설명은 현재 레이아웃 확인용 기본 문구입니다.
최종 단계에서는 Delegate 또는 MVVM FieldNotify를 통해 데이터가 변경될 때만
Text와 Image를 갱신하는 이벤트 기반 구조로 연결합니다.

---

## 6. Definition of Done (DoD / 개발 완료 기준)

새로운 기능이나 캐릭터 구현을 완료하고 PR을 생성하기 전, 아래 항목을 모두 자체 검증해야 합니다.

- [ ] **Listen Server 멀티플레이 검증:** Listen Server + Client 2인 이상 환경에서 판정 및 동기화 정상 작동 확인
- [ ] **네트워크 패킷 이상 검사:** `Network Insights` 상에서 비정상적인 RPC 난사 및 패킷 스파이크 없음
- [ ] **가상 지연 테스트 통과:** 콘솔 명령 `Net PktLag=150`, `Net PktLoss=3` 환경에서 치명적 위치 튐(Rubberbanding) 없음
- [ ] **데이터 검증기(Validator) 통과:** DataAsset 내 빈 포인터(`nullptr`) 및 필수 GameplayTag 누락 없음
- [ ] **빌드 안정성:** C++ 경고(Warning) 0개 및 에디터 독립 패키징 빌드 크래시 없음
- [ ] **Trouble Shooting 기록:** 디버깅 과정 중 특이 이슈 발생 시 노션 트러블슈팅 DB에 영상(GIF)과 함께 원인/해결책 문서화

---

## 7. 팀 협업 링크 (Workspace)

* 📘 [Team Notion Workspace](https://app.notion.com/p/teamsparta/3ac2dc3ef51480e891f5dff50ea77c0e)
* 🎨 [Miro Idea & Workflow Board]([https://miro.com](https://miro.com/app/board/uXjVHzRnd3M=/))

---
*Copyright © 2026 BARUGame Team. All rights reserved.*
