// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHUD.h"

#include "MatchMapHudTopWidget.h"
#include "Valorant.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "GameManager/ValorantGameInstance.h"
#include "Player/Agent/BaseAgent.h"

void UMatchMapHUD::SetTrueVo()
{
	bPlayed60SecLeftVo = true;
	bPlayed30SecLeftVo = true;
	bPlayed10SecLeftVo = true;
}

void UMatchMapHUD::SetFalseVo()
{
	bPlayed60SecLeftVo = false;
	bPlayed30SecLeftVo = false;
	bPlayed10SecLeftVo = false;
}

void UMatchMapHUD::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 게임 인스턴스 가져오기
	GameInstance = UValorantGameInstance::Get(GetWorld());

	auto* GameState = GetWorld()->GetGameState<AMatchGameState>();
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapHUD::UpdateTime);
	GameState->OnRoundSubStateChanged.AddDynamic(this, &UMatchMapHUD::OnRoundSubStateChanged);
	GameState->OnRoundEnd.AddDynamic(this, &UMatchMapHUD::OnRoundEnd);

	AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (pc)
	{
		BindToDelegatePC(pc->GetCacehdASC(),pc);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("MatchMap HUD, PC NULL"));
		return;
	}

	if (AAgentPlayerState* ps = pc->GetPlayerState<AAgentPlayerState>())
	{
		InitUI(ps);
		
		// 어빌리티 스택 변경 델리게이트 바인딩
		ps->OnAbilityStackChanged.AddDynamic(this, &UMatchMapHUD::HandleAbilityStackChanged);
		
		// 서버에 스택 정보 요청
		ps->Server_RequestAbilityStackSync();
		
		// 어빌리티 스택 초기화
		InitializeAbilityStacks();
		
		// 어빌리티 데이터 로드 및 UI 업데이트
		LoadAllAbilityData();
		UpdateAbilityUI();

		// 크레딧 정보 바인딩
		ps->OnCreditChangedDelegate.AddDynamic(this, &UMatchMapHUD::UpdateCreditDisplay);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("MatchMap HUD, PS NULL"));
	}
}

void UMatchMapHUD::UpdateTime(float Time)
{
	const int Minute = static_cast<int>(Time / 60);
	const int Seconds = static_cast<int>(Time) % 60;
	if (bIsPreRound && FMath::IsNearlyEqual(Time, 3.f, 0.5f))
	{
		PlayRemTimeVO(3);
		bIsPreRound = false;
	}
	if (Time <= 60)
	{
		if (false == bPlayed60SecLeftVo && FMath::IsNearlyEqual(Time, 60.f, 0.5f))
		{
			PlayRemTimeVO(0);
			bPlayed60SecLeftVo = true;
		}
		if (Time <= 30)
		{
			if (false == bPlayed30SecLeftVo && FMath::IsNearlyEqual(Time, 30.f, 0.5f))
			{
				PlayRemTimeVO(1);
				bPlayed30SecLeftVo = true;
			}
			if (Time <= 10)
			{
				if (false == bPlayed10SecLeftVo && FMath::IsNearlyEqual(Time, 10.f, 0.5f))
				{
					PlayRemTimeVO(2);
					bPlayed10SecLeftVo = true;
				}
			}
		}
	}
}

void UMatchMapHUD::OnRoundSubStateChanged(const ERoundSubState RoundSubState, const float TransitionTime)
{
	SetTrueVo();
	switch (RoundSubState) {
	case ERoundSubState::RSS_None:
		break;
	case ERoundSubState::RSS_SelectAgent:
		break;
	case ERoundSubState::RSS_PreRound:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, TransitionTime);
		break;
	case ERoundSubState::RSS_BuyPhase:
		DisplayAnnouncement(EMatchAnnouncement::EMA_BuyPhase, TransitionTime);
		break;
	case ERoundSubState::RSS_InRound:
		SetFalseVo();
		break;
	case ERoundSubState::RSS_EndPhase:
		break;
	}
	DebugRoundSubState(StaticEnum<ERoundSubState>()->GetNameStringByValue(static_cast<int>(RoundSubState)));
}

void UMatchMapHUD::OnRoundEnd(bool bBlueWin, const ERoundEndReason RoundEndReason, const float TransitionTime)
{
	auto* PlayerState = GetOwningPlayer()->GetPlayerState<AMatchPlayerState>();
	if (nullptr == PlayerState)
	{
		return;
	}

	bool bIsPlayerWin = (PlayerState->bIsBlueTeam == bBlueWin);
	
	// 라운드 종료 이유에 따라 적절한 UI 표시
	switch (RoundEndReason)
	{
	case ERoundEndReason::ERER_SpikeActive:
		// 스파이크 폭발로 인한 종료
		if (bIsPlayerWin)
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeActivated_Won, TransitionTime);
		}
		else
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeActivated_Lost, TransitionTime);
		}
		break;
	case ERoundEndReason::ERER_SpikeDefuse:
		// 스파이크 해제로 인한 종료
		if (bIsPlayerWin)
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeDefused_Won, TransitionTime);
		}
		else
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_SpikeDefused_Lost, TransitionTime);
		}
		break;
	default:
		// 일반적인 승리/패배
		if (bIsPlayerWin)
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_Won, TransitionTime);
		}
		else
		{
			DisplayAnnouncement(EMatchAnnouncement::EMA_Lost, TransitionTime);
		}
		break;
	}
	
	PlayRoundEndVFX(bIsPlayerWin);
}

void UMatchMapHUD::DisplayAnnouncement(EMatchAnnouncement MatchAnnouncement, float DisplayTime)
{
	NET_LOG(LogTemp, Warning, TEXT("%hs Called, Idx: %d, TransitionTime: %f"), __FUNCTION__, static_cast<int32>(MatchAnnouncement), DisplayTime);
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Visible);
	WidgetSwitcherAnnouncement->SetActiveWidgetIndex(static_cast<int32>(MatchAnnouncement));

	TopWidget->CanvasPanelTimer->SetVisibility(ESlateVisibility::Visible);
	TopWidget->ImageSpike->SetVisibility(ESlateVisibility::Hidden);
	
	OnLowState(false);
	HideFollowUpInputUI();
	OnSpikeOwnChanged(false);
	
	GetWorld()->GetTimerManager().SetTimer(AnnouncementTimerHandle, this, &UMatchMapHUD::HideAnnouncement, DisplayTime, false);
	bIsPreRound = true;
}

void UMatchMapHUD::HideAnnouncement()
{
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Hidden);
}

void UMatchMapHUD::UpdateDisplayHealth(const float health, bool bIsDamage)
{
	int32 intHealth = FMath::TruncToInt(health);
	txt_HP->SetText(FText::AsNumber(intHealth));
}

void UMatchMapHUD::UpdateDisplayArmor(const float armor)
{
	int32 intArmor = FMath::TruncToInt(armor);
	txt_Armor->SetText(FText::AsNumber(intArmor));
}

void UMatchMapHUD::UpdateAmmo(bool bDisplayWidget, int MagazineAmmo, int SpareAmmo)
{
	if (bDisplayWidget)
	{
		HorizontalBoxAmmo->SetVisibility(ESlateVisibility::Visible);
		TextBlockMagazineAmmo->SetText(FText::FromString(FString::Printf(TEXT("%d"), MagazineAmmo)));
		TextBlockSpareAmmo->SetText(FText::FromString(FString::Printf(TEXT("%d"), SpareAmmo)));
	}
	else
	{
		HorizontalBoxAmmo->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMatchMapHUD::OnDamaged(const FVector& HitOrg, const EAgentDamagedPart DamagedPart,
	const EAgentDamagedDirection DamagedDirection, const bool bDie, const bool bLarge, const bool bLowState)
{
	OnDisplayIndicator(HitOrg);
	if (bLowState)
	{
		OnLowState(true);
	}
}

void UMatchMapHUD::BindToDelegatePC(UAgentAbilitySystemComponent* asc, AAgentPlayerController* pc)
{
	pc->OnDamaged_PC.AddDynamic(this, &UMatchMapHUD::OnDamaged);
	pc->OnHealed_PC.AddDynamic(this, &UMatchMapHUD::OnHealed);
	pc->OnKillEvent_PC.AddDynamic(this, &UMatchMapHUD::OnKillEvent);
	pc->OnHealthChanged_PC.AddDynamic(this, &UMatchMapHUD::UpdateDisplayHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &UMatchMapHUD::UpdateDisplayArmor);
	pc->OnChangedAmmo.AddDynamic(this, &UMatchMapHUD::UpdateAmmo);
	pc->GetCacehdASC()->RegisterGameplayTagEvent(FValorantGameplayTags::Get().State_Debuff_Suppressed,EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UMatchMapHUD::OnSuppressed);

	if (asc == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AgentWidget, ASC NULL"));
		return;
	}
	ASC = asc;
}

void UMatchMapHUD::InitUI(AAgentPlayerState* ps)
{
	txt_HP->SetText(FText::AsNumber(ps->GetHealth()));
	txt_Armor->SetText(FText::AsNumber(ps->GetArmor()));
}

void UMatchMapHUD::DebugRoundSubState(const FString& RoundSubStateStr)
{
	// TextBlockRoundSubStateDbg->SetText(FText::FromString(TEXT("RoundSubState: ") + RoundSubStateStr));
}

// 어빌리티 스택 처리 함수 구현
void UMatchMapHUD::HandleAbilityStackChanged(int32 AbilityID, int32 NewStack)
{
	// 스택 정보 캐시 업데이트
	AbilityStacksCache.Add(AbilityID, NewStack);
	
	// 해당 어빌리티가 어떤 슬롯에 해당하는지 확인하고 업데이트
	if (AbilityID == SkillCID)
	{
		UpdateSlotStackInfo(EAbilitySlotType::Slot_C, AbilityID);
	}
	else if (AbilityID == SkillQID)
	{
		UpdateSlotStackInfo(EAbilitySlotType::Slot_Q, AbilityID);
	}
	else if (AbilityID == SkillEID)
	{
		UpdateSlotStackInfo(EAbilitySlotType::Slot_E, AbilityID);
	}
	else if (AbilityID == SkillXID)
	{
		UpdateSlotStackInfo(EAbilitySlotType::Slot_X, AbilityID);
	}
	
	// 어빌리티 정보 캐시 업데이트
	FHUDAbilityInfo* AbilityInfo = AbilityInfoCache.Find(AbilityID);
	if (AbilityInfo)
	{
		AbilityInfo->CurrentStack = NewStack;
		
		// 슬롯 타입 확인 및 UI 업데이트
		if (AbilityID == SkillCID)
		{
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_C, *AbilityInfo);
		}
		else if (AbilityID == SkillQID)
		{
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_Q, *AbilityInfo);
		}
		else if (AbilityID == SkillEID)
		{
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_E, *AbilityInfo);
		}
		else if (AbilityID == SkillXID)
		{
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_X, *AbilityInfo);
		}
	}
}

void UMatchMapHUD::InitializeAbilityStacks()
{
	// 플레이어 스테이트와 게임 인스턴스가 유효한지 확인
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}
	
	AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}
	
	if (!GameInstance)
	{
		GameInstance = UValorantGameInstance::Get(GetWorld());
		if (!GameInstance)
		{
			return;
		}
	}
	
	// 현재 캐릭터의 능력 ID 가져오기
	int32 AgentID = PS->GetAgentID();

	if (AgentID == 0)
	{
		return;
	}
	
	FAgentData* AgentData = GameInstance->GetAgentData(AgentID);
	if (!AgentData)
	{
		return;
	}
	
	// 능력 ID 저장
	if (AgentData->AbilityID_C > 0)
	{
		SkillCID = AgentData->AbilityID_C;
	}
	
	if (AgentData->AbilityID_Q > 0)
	{
		SkillQID = AgentData->AbilityID_Q;
	}
	
	if (AgentData->AbilityID_E > 0)
	{
		SkillEID = AgentData->AbilityID_E;
	}
	
	if (AgentData->AbilityID_X > 0)
	{
		SkillXID = AgentData->AbilityID_X;
	}
	
	// 각 능력의 스택 정보 초기화
	AbilityStacksCache.Empty();
	SlotStackInfoMap.Empty();
	
	// C 능력 스택 로드
	if (SkillCID > 0)
	{
		int32 StackC = PS->GetAbilityStack(SkillCID);
		int32 MaxStackC = PS->GetMaxAbilityStack(SkillCID);
		AbilityStacksCache.Add(SkillCID, StackC);

		// 어빌리티 슬롯 max 갯수를 위한 초기화
		InitializeAbilityMaxStacks(EAbilitySlotType::Slot_C, MaxStackC);
		
		// 슬롯 스택 정보 초기화 및 업데이트
		UpdateSlotStackInfo(EAbilitySlotType::Slot_C, SkillCID);
	}
	
	// Q 능력 스택 로드
	if (SkillQID > 0)
	{
		int32 StackQ = PS->GetAbilityStack(SkillQID);
		int32 MaxStackQ = PS->GetMaxAbilityStack(SkillQID);
		AbilityStacksCache.Add(SkillQID, StackQ);
		
		// 어빌리티 슬롯 max 갯수를 위한 초기화
		InitializeAbilityMaxStacks(EAbilitySlotType::Slot_Q, MaxStackQ);
		
		// 슬롯 스택 정보 초기화 및 업데이트
		UpdateSlotStackInfo(EAbilitySlotType::Slot_Q, SkillQID);
	}
	
	// E 능력 스택 로드
	if (SkillEID > 0)
	{
		int32 StackE = PS->GetAbilityStack(SkillEID);
		int32 MaxStackE = PS->GetMaxAbilityStack(SkillEID);
		AbilityStacksCache.Add(SkillEID, StackE);
		
		// 어빌리티 슬롯 max 갯수를 위한 초기화
		InitializeAbilityMaxStacks(EAbilitySlotType::Slot_E, MaxStackE);

		// 슬롯 스택 정보 초기화 및 업데이트
		UpdateSlotStackInfo(EAbilitySlotType::Slot_E, SkillEID);
	}
	
	// X 능력 스택 로드
	if (SkillXID > 0)
	{
		int32 StackX = PS->GetAbilityStack(SkillXID);
		int32 MaxStackX = PS->GetMaxAbilityStack(SkillXID);
		AbilityStacksCache.Add(SkillXID, StackX);
		
		// 어빌리티 슬롯 max 갯수를 위한 초기화
		InitializeAbilityMaxStacks(EAbilitySlotType::Slot_X, MaxStackX);

		// 슬롯 스택 정보 초기화 및 업데이트
		UpdateSlotStackInfo(EAbilitySlotType::Slot_X, SkillXID);
	}
}

void UMatchMapHUD::UpdateSlotStackInfo(EAbilitySlotType AbilitySlot, int32 AbilityID)
{
	// 플레이어 스테이트 가져오기
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}
	
	AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}
	
	// 현재 스택 및 최대 스택 가져오기
	int32 CurrentStack = PS->GetAbilityStack(AbilityID);
	int32 MaxStack = PS->GetMaxAbilityStack(AbilityID);
	
	// 슬롯 스택 정보 업데이트
	FAbilitySlotStackInfo StackInfo;
	StackInfo.Initialize(AbilityID, CurrentStack, MaxStack);
	SlotStackInfoMap.Add(AbilitySlot, StackInfo);
	
	// 블루프린트 이벤트 호출
	OnSlotStackInfoUpdated(AbilitySlot, StackInfo);
}

FAbilitySlotStackInfo UMatchMapHUD::GetSlotStackInfo(EAbilitySlotType AbilitySlot) const
{
	// 캐시된 슬롯 스택 정보 반환
	const FAbilitySlotStackInfo* StackInfoPtr = SlotStackInfoMap.Find(AbilitySlot);
	if (StackInfoPtr)
	{
		return *StackInfoPtr;
	}
	
	// 정보가 없으면 기본값 반환
	return FAbilitySlotStackInfo();
}

// 특정 어빌리티 데이터 로드
void UMatchMapHUD::LoadAbilityData(int32 AbilityID)
{
	if (!GameInstance)
	{
		return;
	}
	
	FAbilityData* AbilityData = GameInstance->GetAbilityData(AbilityID);
	if (!AbilityData)
	{
		return;
	}
	
	// 현재 스택 정보 가져오기
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}
	
	AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}
	
	int32 CurrentStack = PS->GetAbilityStack(AbilityID);
	int32 MaxStack = PS->GetMaxAbilityStack(AbilityID);
	
	// 어빌리티 정보 구조체 생성 및 캐싱
	FHUDAbilityInfo AbilityInfo;
	AbilityInfo.AbilityID = AbilityID;
	AbilityInfo.AbilityName = AbilityData->AbilityName;
	AbilityInfo.AbilityDescription = AbilityData->AbilityDescription;
	AbilityInfo.AbilityIcon = AbilityData->AbilityIcon;
	AbilityInfo.ChargeCost = AbilityData->ChargeCost;
	AbilityInfo.CurrentStack = CurrentStack;
	AbilityInfo.MaxStack = MaxStack;
	
	// 캐시에 저장
	AbilityInfoCache.Add(AbilityID, AbilityInfo);
}

// 모든 어빌리티 데이터 로드
void UMatchMapHUD::LoadAllAbilityData()
{
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return;
	}
	
	AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return;
	}
	
	// 캐릭터 ID 가져오기
	int32 AgentID = PS->GetAgentID();
	if (AgentID == 0 || !GameInstance)
	{
		return;
	}
	
	FAgentData* AgentData = GameInstance->GetAgentData(AgentID);
	if (!AgentData)
	{
		return;
	}
	
	// 각 슬롯의 어빌리티 ID 로드
	if (AgentData->AbilityID_C > 0)
	{
		SkillCID = AgentData->AbilityID_C;
		LoadAbilityData(SkillCID);
	}
	
	if (AgentData->AbilityID_Q > 0)
	{
		SkillQID = AgentData->AbilityID_Q;
		LoadAbilityData(SkillQID);
	}
	
	if (AgentData->AbilityID_E > 0)
	{
		SkillEID = AgentData->AbilityID_E;
		LoadAbilityData(SkillEID);
	}
	
	if (AgentData->AbilityID_X > 0)
	{
		SkillXID = AgentData->AbilityID_X;
		LoadAbilityData(SkillXID);
	}
}

// 어빌리티 UI 업데이트
void UMatchMapHUD::UpdateAbilityUI()
{
	// 각 슬롯 별 어빌리티 정보 업데이트
	if (SkillCID > 0)
	{
		FHUDAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillCID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityC_Image && AbilityInfo->AbilityIcon)
			{
				AbilityC_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_C, *AbilityInfo);
		}
	}
	
	if (SkillQID > 0)
	{
		FHUDAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillQID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityQ_Image && AbilityInfo->AbilityIcon)
			{
				AbilityQ_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_Q, *AbilityInfo);
		}
	}
	
	if (SkillEID > 0)
	{
		FHUDAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillEID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityE_Image && AbilityInfo->AbilityIcon)
			{
				AbilityE_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_E, *AbilityInfo);
		}
	}
	
	if (SkillXID > 0)
	{
		FHUDAbilityInfo* AbilityInfo = AbilityInfoCache.Find(SkillXID);
		if (AbilityInfo)
		{
			// 이미지 UI 업데이트
			if (AbilityX_Image && AbilityInfo->AbilityIcon)
			{
				AbilityX_Image->SetBrushFromTexture(AbilityInfo->AbilityIcon);
			}
			
			// 블루프린트 이벤트 호출
			OnAbilityInfoUpdated(EAbilitySlotType::Slot_X, *AbilityInfo);
		}
	}
}

// 어빌리티 정보 가져오기
FHUDAbilityInfo UMatchMapHUD::GetAbilityInfo(int32 AbilityID) const
{
	const FHUDAbilityInfo* InfoPtr = AbilityInfoCache.Find(AbilityID);
	if (InfoPtr)
	{
		return *InfoPtr;
	}
	
	// 기본 빈 정보 반환
	return FHUDAbilityInfo();
}

// 슬롯별 어빌리티 정보 가져오기
FHUDAbilityInfo UMatchMapHUD::GetAbilityInfoBySlot(EAbilitySlotType SlotType) const
{
	int32 AbilityID = 0;
	
	switch (SlotType)
	{
	case EAbilitySlotType::Slot_C:
		AbilityID = SkillCID;
		break;
	case EAbilitySlotType::Slot_Q:
		AbilityID = SkillQID;
		break;
	case EAbilitySlotType::Slot_E:
		AbilityID = SkillEID;
		break;
	case EAbilitySlotType::Slot_X:
		AbilityID = SkillXID;
		break;
	default:
		break;
	}
	
	if (AbilityID > 0)
	{
		return GetAbilityInfo(AbilityID);
	}
	
	// 기본 빈 정보 반환
	return FHUDAbilityInfo();
}

void UMatchMapHUD::OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& KillFeedInfo)
{
	auto* GI = GetWorld()->GetGameInstance<UValorantGameInstance>();
	if (nullptr == GI)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, GameInstance is nullptr"), __FUNCTION__);
		return;
	}
	
	const auto* MyController = GetWorld()->GetFirstPlayerController<AAgentPlayerController>();
	if (nullptr == MyController)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MyController is nullptr"), __FUNCTION__);
		return;
	}

	const auto* MyPS = MyController->GetPlayerState<AAgentPlayerState>();
	if (nullptr == MyPS)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MyPS is nullptr"), __FUNCTION__);
		return;
	}

	FString InstigatorName = "";
	bool bInstigatorIsMyTeam = false;
	const UTexture2D* InstigatorIcon = nullptr;
	if (InstigatorAgent)
	{
		InstigatorName = InstigatorAgent->GetPlayerNickname();
		bInstigatorIsMyTeam = InstigatorAgent->IsBlueTeam() == MyPS->bIsBlueTeam;
		if (const FAgentData* InstigatorData = GI->GetAgentData(InstigatorAgent->GetAgentID()))
		{
			// NET_LOG(LogTemp, Warning, TEXT("%hs Called, AgentID is %d, KillFeedIconName is %s"), __FUNCTION__, InstigatorAgent->GetAgentID(), *InstigatorData->KillFeedIcon->GetName());
			InstigatorIcon = InstigatorData->KillFeedIcon;
		}
	}

	FString VictimName = "";
	bool bVictimIsMyTeam = false;
	const UTexture2D* VictimIcon = nullptr;
	if (VictimAgent)
	{
		VictimName = VictimAgent->GetPlayerNickname();
		bVictimIsMyTeam = VictimAgent->IsBlueTeam() == MyPS->bIsBlueTeam;
		if (const FAgentData* VictimData = GI->GetAgentData(VictimAgent->GetAgentID()))
		{
			VictimIcon = VictimData->KillFeedIcon;
		}
	}
	
	const UTexture2D* ReasonIcon = nullptr;
	const int WeaponID = KillFeedInfo.WeaponID;
	if (const auto* Data = GI->GetWeaponData(WeaponID))
	{
		ReasonIcon = Data->WeaponIcon;
	}
	
	DisplayKillFeed(InstigatorIcon, InstigatorName, bInstigatorIsMyTeam, VictimIcon, VictimName, bVictimIsMyTeam, KillFeedInfo, ReasonIcon);
}

void UMatchMapHUD::DisplayFollowUpInputUI(FGameplayTag slotTag, EFollowUpInputType inputType)
{
	if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.E")))
	{
		if (inputType == EFollowUpInputType::LeftClick)
		{
			DisplayedFollowUpInputUI = Left_E;
			Left_E->SetVisibility(ESlateVisibility::Visible);
		}
		else if (inputType == EFollowUpInputType::LeftOrRight)
		{
			DisplayedFollowUpInputUI = LeftOrRight_E;
			LeftOrRight_E->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.Q")))
	{
		if (inputType == EFollowUpInputType::LeftClick)
		{
			DisplayedFollowUpInputUI = Left_Q;
			Left_Q->SetVisibility(ESlateVisibility::Visible);
		}
		else if (inputType == EFollowUpInputType::LeftOrRight)
		{
			DisplayedFollowUpInputUI = LeftOrRight_Q;
			LeftOrRight_Q->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.C")))
	{
		if (inputType == EFollowUpInputType::LeftClick)
		{
			DisplayedFollowUpInputUI = Left_C;
			Left_C->SetVisibility(ESlateVisibility::Visible);
		}
		else if (inputType == EFollowUpInputType::LeftOrRight)
		{
			DisplayedFollowUpInputUI = LeftOrRight_C;
			LeftOrRight_C->SetVisibility(ESlateVisibility::Visible);
		}
	}
	// else if (slotTag == FGameplayTag::RequestGameplayTag(FName("Input.Skill.X")))
	// {
	// 	if (inputType == EFollowUpInputType::LeftClick)
	// 	{
	// 		Left_X->SetVisibility(ESlateVisibility::Visible);
	// 	}
	// 	else if (inputType == EFollowUpInputType::LeftOrRight)
	// 	{
	// 		LeftOrRight_X->SetVisibility(ESlateVisibility::Visible);
	// 	}
	// }
}

void UMatchMapHUD::HideFollowUpInputUI()
{
	if (DisplayedFollowUpInputUI)
	{
		DisplayedFollowUpInputUI->SetVisibility(ESlateVisibility::Hidden);
		DisplayedFollowUpInputUI = nullptr;
	}
}

int32 UMatchMapHUD::GetAbilityStack(int32 AbilityID) const
{
	// 캐시에서 스택 값 가져오기
	const int32* StackPtr = AbilityStacksCache.Find(AbilityID);
	if (StackPtr)
	{
		return *StackPtr;
	}
	
	// 캐시에 없으면 플레이어 스테이트에서 직접 가져오기
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return 0;
	}
	
	AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return 0;
	}
	
	return PS->GetAbilityStack(AbilityID);
}

int32 UMatchMapHUD::GetMaxAbilityStack(int32 AbilityID) const
{
	// 플레이어 스테이트에서 최대 스택 값 가져오기
	AAgentPlayerController* PC = Cast<AAgentPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		return 0;
	}
	
	AAgentPlayerState* PS = PC->GetPlayerState<AAgentPlayerState>();
	if (!PS)
	{
		return 0;
	}
	
	return PS->GetMaxAbilityStack(AbilityID);
}
