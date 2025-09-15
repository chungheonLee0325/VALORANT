# Valorant Project Wiki

> 발로란트 클론 코딩 프로젝트의 핵심 시스템을 기록하고 공유하는 기술 위키입니다.

---

## 📚 목차 (Table of Contents)

### 1. Gameplay Ability System
> 캐릭터의 스킬과 상호작용의 근간이 되는 Gameplay Ability System(GAS) 아키텍처와 구현을 다룹니다.
> 
> *GAS의 기본 개념이 익숙하지 않다면, 먼저 [GAS 기본 개념 정리 (GAS Fundamentals)](./Ref/GAS-Fundamentals.md) 문서를 읽어보시는 것을 권장합니다.*
*   [**1.1. GAS 선정 이유**](./1.1_Why-I-Chose-GAS.md) - 프로젝트에 GAS를 도입한 이유에 대해 설명합니다.
*   [**1.2. GAS 아키텍처**](./1.2_Project-GAS-Architecture.md) - 이 프로젝트에서 GAS를 어떻게 확장하고 활용했는지 설명합니다.
*   [**1.3. 어빌리티 상세 구현과 흐름**](./1.3_Skill-Implementation.md) - `UBaseGameplayAbility`의 상태 머신과 활성화 흐름을 분석합니다.
*   [**1.4. 확장 가능한 스킬 대량 생산**](./1.4_Scalable-Skill-Production.md) - 12개 스킬을 효율적으로 구현한 하이브리드 아키텍처를 소개합니다.
*   [**1.5. 입력 및 HUD와 능력 시스템 연동**](./1.5_Input_HUD_ASC.md) - UI 및 입력 처리가 GAS와 어떻게 상호작용하는지 다룹니다.

### 2. Core Gameplay & Economy
> 게임의 승패를 결정하는 핵심 메카닉과 라운드 기반의 경제 시스템을 다룹니다.

*   [**2.1. 섬광(Flash) 시스템**](./2.1_Flash-System.md) - 플레이어의 화면을 제어하는 섬광탄 시스템을 설명합니다.
*   [**2.2. 스파이크 시스템**](./2.2_Spike-System.md) - 스파이크의 설치, 해체, 라운드 판정 로직을 다룹니다.
*   [**2.3. 상점 및 경제 시스템**](./2.3_Shop-Economy.md) - 라운드 기반의 재화 획득 및 아이템 구매 시스템을 설명합니다.

### 3. 프로젝트 회고 (Project Retrospective)
> 프로젝트를 진행하며 얻은 기술적 교훈과 성장 과정을 기록합니다.

*   [**3.1. 첫 GAS 적용 회고**](./3.1_Project_Retrospective.md) - 첫 GAS 프로젝트의 잘한 점, 아쉬웠던 점, 그리고 개선 방향을 정리합니다.

### 4. 기술 부채 개선 사례 (Tech Debt Improvement Case)
> 회고에서 논의된 기술 부채를 실제로 어떻게 개선할 수 있는지 구체적인 코드로 제시합니다.

*   [**4.1. GameplayCue 리팩토링 예시**](./4.1_Refactoring_Example-GameplayCue.md) - Multicast RPC를 GameplayCue 시스템으로 전환하는 리팩토링 사례를 분석합니다.
