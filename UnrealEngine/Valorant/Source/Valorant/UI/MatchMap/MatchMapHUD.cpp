// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchMapHUD.h"

#include "OnlineSubsystemUtils.h"
#include "Valorant.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Player/AgentPlayerController.h"
#include "Player/AgentPlayerState.h"
#include "GameManager/ValorantGameInstance.h"

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

	auto* GameState = Cast<AMatchGameState>(GetWorld()->GetGameState());
	GameState->OnRemainRoundStateTimeChanged.AddDynamic(this, &UMatchMapHUD::UpdateTime);
	GameState->OnTeamScoreChanged.AddDynamic(this, &UMatchMapHUD::UpdateScore);
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
	const FString TimeStr = FString::Printf(TEXT("%d:%02d"), Minute, Seconds);
	TextBlockTime->SetText(FText::FromString(TimeStr));
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

void UMatchMapHUD::UpdateScore(int TeamBlueScore, int TeamRedScore)
{
	TextBlockBlueScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamBlueScore)));
	TextBlockRedScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), TeamRedScore)));
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
	GetWorld()->GetTimerManager().SetTimer(AnnouncementTimerHandle, this, &UMatchMapHUD::HideAnnouncement, DisplayTime, false);
}

void UMatchMapHUD::HideAnnouncement()
{
	WidgetSwitcherAnnouncement->SetVisibility(ESlateVisibility::Hidden);
}

void UMatchMapHUD::UpdateDisplayHealth(const float health)
{
	txt_HP->SetText(FText::AsNumber(health));
}

void UMatchMapHUD::UpdateDisplayArmor(const float armor)
{
	txt_Armor->SetText(FText::AsNumber(armor));
}

void UMatchMapHUD::BindToDelegatePC(UAgentAbilitySystemComponent* asc, AAgentPlayerController* pc)
{
	pc->OnHealthChanged_PC.AddDynamic(this, &UMatchMapHUD::UpdateDisplayHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &UMatchMapHUD::UpdateDisplayArmor);

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
	TextBlockRoundSubStateDbg->SetText(FText::FromString(TEXT("RoundSubState: ") + RoundSubStateStr));
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
