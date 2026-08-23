# BARUGame (Project BARU)

> **Unreal Engine 5.8 기반의 Dedicated Server 멀티플레이어 익스트랙션 호러 게임**

---

## 1. 프로젝트 개요 (Overview)

* **게임 타이틀:** B.A.R.U (Buried Asset Recovery Unit, 매몰 자산 회수단)
* **장르:** 멀티플레이어 익스트랙션 호러 (Dedicated Server)
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
| **Network** | Dedicated Server + EOS | Epic Online Services 세션/매치메이킹 연동 |
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
git clone [https://github.com/](https://github.com/)[Organization]/BARUGame.git

# 2. develop 브랜치로 전환
git checkout develop
```
* `BARUGame.uproject`를 우클릭하거나 JetBrains Rider에서 `.uproject`를 직접 열어 C++ 프로젝트를 빌드합니다.
* 에디터 실행 시 자동으로 **Dedicated Server + 2 Clients (960x540 FHD 분할)** 테스트 환경이 로드됩니다.

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

---

## 6. Definition of Done (DoD / 개발 완료 기준)

새로운 기능이나 캐릭터 구현을 완료하고 PR을 생성하기 전, 아래 항목을 모두 자체 검증해야 합니다.

- [ ] **Dedicated Server 멀티플레이 검증:** `Play as Client` 2인 환경에서 스킬 판정 및 동기화 정상 작동 확인
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
