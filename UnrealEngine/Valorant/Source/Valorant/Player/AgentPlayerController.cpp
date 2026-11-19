// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentPlayerController.h"

#include <GameManager/SubsystemSteamManager.h>

#include "AgentPlayerState.h"
#include "Valorant.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "Agent/BaseAgent.h"
#include "UI/MatchMap/MatchMapHUD.h"
#include "Widget/AgentBaseWidget.h"
#include "Valorant/Player/Widget/MiniMapWidget.h"
#include "Component/ShopComponent.h"
#include "Valorant/GameManager/ValorantGameInstance.h"
#include "Components/InputComponent.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/MatchMap/ShopLayer.h"
#include "Player/Component/AgentInputComponent.h"


AAgentPlayerController::AAgentPlayerController()
{
	ShopComponent = CreateDefaultSubobject<UShopComponent>(TEXT("ShopComponent"));
}

void AAgentPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// UE_LOG(LogTemp, Warning, TEXT("PC, BeginPlay → %s, LocalRole=%d, IsLocal=%d"),
	// *GetName(), (int32)GetLocalRole(), IsLocalController());

	// ShopComponent 초기화
	if (ShopComponent)
	{
		// 에이전트가 가진 스킬 ID를 가져와서 상점 초기화
		// 아직 에이전트가 연결되지 않았으므로 나중에 처리
		UE_LOG(LogTemp, Display, TEXT("PC, ShopComponent found"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PC, ShopComponent not found"));
	}
	
	// GameState의 상점 닫기 이벤트 구독
	if (AMatchGameState* GameState = GetWorld()->GetGameState<AMatchGameState>())
	{
		GameState->OnShopClosed.AddDynamic(this, &AAgentPlayerController::CloseShopUI);
		GameState->OnMatchEnd.AddDynamic(this, &AAgentPlayerController::OnMatchEnd);
		GameState->OnSpikePlanted.AddDynamic(this, &AAgentPlayerController::OnSpikePlanted);
	}
}

void AAgentPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	OnRep_Pawn();
	
	InitGAS();

	if (IsLocalController())
	{
		m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
	}

}

void AAgentPlayerController::OnUnPossess()
{
	CloseShopUI();
	Super::OnUnPossess();
	if (ShopViewModel)
	{
		ShopViewModel->Bind(*this);
	}
}

void AAgentPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseShopUI();
	if (ShopLayer)
	{
		ShopLayer->RemoveFromParent();
	}
	if (ShopViewModel)
	{
		ShopViewModel->Unbind();
	}
	UnbindCachedAttributes();
	Super::EndPlay(EndPlayReason);
}

void AAgentPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &AAgentPlayerController::RequestShopUI);
	}
}

void AAgentPlayerController::OnRep_PlayerState()
{
	CloseShopUI();
	Super::OnRep_PlayerState();
	if (ShopViewModel)
	{
		ShopViewModel->Bind(*this);
	}

	InitGAS();

	m_GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
	if (auto* matchHud = Cast<UMatchMapHUD>(GetMatchMapHud()))
	{
		matchHud->RefreshPlayerSources();
	}
}

void AAgentPlayerController::Client_EnterSpectatorMode_Implementation()
{
	StartSpectatingOnly();
}

void AAgentPlayerController::InitGAS()
{
	auto* state = GetPlayerState<AAgentPlayerState>();
	auto* attributes = state ? state->GetBaseAttributeSet() : nullptr;
	CachedASC = state ? state->GetAbilitySystemComponent() : nullptr;
	if (CachedABS == attributes)
	{
		return;
	}

	UnbindCachedAttributes();
	CachedABS = attributes;
	if (!CachedABS)
	{
		return;
	}

	if (HasAuthority())
	{
		CachedABS->ResetAttributeData();
	}
	CachedABS->OnHealthChanged.AddUniqueDynamic(this, &AAgentPlayerController::HandleHealthChanged);
	CachedABS->OnMaxHealthChanged.AddUniqueDynamic(this, &AAgentPlayerController::HandleMaxHealthChanged);
	CachedABS->OnArmorChanged.AddUniqueDynamic(this, &AAgentPlayerController::HandleArmorChanged);
	CachedABS->OnEffectSpeedChanged.AddUniqueDynamic(this, &AAgentPlayerController::HandleEffectSpeedChanged);
}

void AAgentPlayerController::UnbindCachedAttributes()
{
	if (IsValid(CachedABS))
	{
		CachedABS->OnHealthChanged.RemoveDynamic(this, &AAgentPlayerController::HandleHealthChanged);
		CachedABS->OnMaxHealthChanged.RemoveDynamic(this, &AAgentPlayerController::HandleMaxHealthChanged);
		CachedABS->OnArmorChanged.RemoveDynamic(this, &AAgentPlayerController::HandleArmorChanged);
		CachedABS->OnEffectSpeedChanged.RemoveDynamic(this, &AAgentPlayerController::HandleEffectSpeedChanged);
	}
	CachedABS = nullptr;
}

void AAgentPlayerController::InitAgentWidget()
{
	if (AgentWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerController에 AgentWidget 좀 넣어주세요."));
		return;
	}

	if (!AgentWidget)
	{
		AgentWidget = CreateWidget<UMatchMapHUD>(this, AgentWidgetClass);
		AgentWidget->AddToViewport();
		AgentWidget->BindToDelegatePC(CachedASC, this);
	}

	// 크레딧 UI 바인딩
	BindCreditWidgetDelegate();

	if (AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>())
	{
		AgentWidget->InitUI(ps);
	}
}

void AAgentPlayerController::HandleHealthChanged(float NewHealth, bool bIsDamage)
{
	// NET_LOG(LogTemp,Display,TEXT("PC, Health Changed 데미지 여부: %d"), bIsDamage);
	if (NewHealth <= 0.0f)
	{
		CloseShopUI();
	}
	OnHealthChanged_PC.Broadcast(NewHealth, bIsDamage);
}

void AAgentPlayerController::OnKillEvent(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent,
	const FKillFeedInfo& Info)
{
	//NET_LOG(LogTemp, Warning, TEXT("%hs Called, InstigatorAgentName: %s"), __FUNCTION__, *InstigatorAgent->GetName());
	OnKillEvent_PC.Broadcast(InstigatorAgent, VictimAgent, Info);
}

void AAgentPlayerController::HandleMaxHealthChanged(float NewMaxHealth)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, MaxHealth Changed"));
	OnMaxHealthChanged_PC.Broadcast(NewMaxHealth);
}

void AAgentPlayerController::HandleArmorChanged(float NewArmor)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, Armor Changed"));
	OnArmorChanged_PC.Broadcast(NewArmor);
}

void AAgentPlayerController::HandleEffectSpeedChanged(float NewSpeed)
{
	//UE_LOG(LogTemp,Display,TEXT("PC, MoveSpeed Changed"));
	OnEffectSpeedChanged_PC.Broadcast(NewSpeed);
}

void AAgentPlayerController::NotifyChangedAmmo(const bool bDisplayWidget, const int MagazineAmmo, const int SpareAmmo) const
{
	OnChangedAmmo.Broadcast(bDisplayWidget, MagazineAmmo, SpareAmmo);
}

void AAgentPlayerController::RequestPurchaseAbility(int AbilityID)
{
	Server_RequestPurchaseAbility(AbilityID);
}

void AAgentPlayerController::Server_RequestPurchaseAbility_Implementation(int AbilityID)
{
	ShopComponent->PurchaseAbility(AbilityID);
}

void AAgentPlayerController::RequestShopUI()
{
	if (IsShopOpen())
	{
		CloseShopUI();
	}
	else
	{
		OpenShopUI();
	}
}

void AAgentPlayerController::RequestOpenShopUI()
{
	OpenShopUI();
}

void AAgentPlayerController::RequestCloseShopUI()
{
	CloseShopUI();
}

void AAgentPlayerController::ClientRPC_SaveMatchResult_Implementation(const FMatchDTO& MatchDto,
	const TArray<FPlayerMatchDTO>& PlayerMatchDtoArray)
{
	if (auto* GI = GetGameInstance<UValorantGameInstance>())
	{
		GI->SaveMatchResult(MatchDto, PlayerMatchDtoArray);
	}
	
	if (auto* SubsystemManager = GetGameInstance()->GetSubsystem<USubsystemSteamManager>())
	{
		SubsystemManager->DestroySession();
	}

	if (false == HasAuthority())
	{
		ClientTravel("/Game/Maps/MainMap", TRAVEL_Absolute);
	}
}

void AAgentPlayerController::OpenShopUI()
{
	if (!GetLocalPlayer() || IsShopOpen() || !IsValid(ShopComponent) || !ShopComponent->IsShopAvailable())
	{
		return;
	}

	if (!ShopUIClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopUIClass is not configured on the local controller."));
		return;
	}

	auto* state = GetPlayerState<AAgentPlayerState>();
	auto* match = GetWorld()->GetGameState<AMatchGameState>();
	if (!state || !state->GetCreditComponent() || !match)
	{
		return;
	}

	if (!ShopLayer)
	{
		ShopLayer = CreateWidget<UShopLayer>(this);
		if (!ShopLayer)
		{
			return;
		}
		// The match HUD is added to the viewport's global widget layer.
		ShopLayer->AddToViewport(10);
	}
	if (!ShopViewModel)
	{
		ShopViewModel = NewObject<UShopViewModel>(this);
	}

	if (auto* input = GetPawn<ABaseAgent>()->FindComponentByClass<UAgentInputComponent>())
	{
		input->ReleaseGameplayInput();
	}
	ShopComponent->RefreshAbilityItems();
	ShopViewModel->Bind(*this);
	ShopViewModel->BeginSession();
	ShopComponent->SetShopActive(true);
	ShopLayer->Open(*ShopViewModel, ShopUIClass);
}

bool AAgentPlayerController::IsShopOpen() const
{
	return ShopLayer && ShopLayer->IsOpen();
}

void AAgentPlayerController::CloseShopUI()
{
	if (IsShopOpen())
	{
		ShopLayer->Close();
	}
}

void AAgentPlayerController::RestoreShopGameplayInput()
{
	if (!IsValid(ShopComponent) || !ShopComponent->IsShopActive())
	{
		return;
	}

	ShopComponent->SetShopActive(false);
	if (ShopViewModel)
	{
		ShopViewModel->EndSession();
	}
	if (auto* router = UCommonUIActionRouterBase::Get(*ShopLayer))
	{
		router->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown));
	}
	SetInputMode(FInputModeGameOnly());
	FlushPressedKeys();
	bShowMouseCursor = false;
}

void AAgentPlayerController::RequestPurchaseWeapon(int32 WeaponID)
{
	Server_RequestPurchaseWeapon(WeaponID);
}

void AAgentPlayerController::Server_RequestPurchaseWeapon_Implementation(int32 WeaponID)
{
	ShopComponent->PurchaseWeapon(WeaponID);
}

void AAgentPlayerController::RequestPurchaseArmor(int32 ArmorLevel)
{
	Server_RequestPurchaseArmor(ArmorLevel);
}

void AAgentPlayerController::Server_RequestPurchaseArmor_Implementation(int32 ArmorID)
{
	ShopComponent->PurchaseArmor(ArmorID);
}

void AAgentPlayerController::BindCreditWidgetDelegate()
{
	// AgentBaseWidget에는 크레딧 기능 필요 없음
	// 기존 크레딧 관련 바인딩 제거

	// AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>();
	// if (!PS)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("PlayerState가 없습니다."));
	// 	return;
	// }
	// 
	// // 크레딧 UI 바인딩 - PlayerState의 델리게이트 사용
	// PS->OnCreditChangedDelegate.AddDynamic(AgentWidget, &UMatchMapHUD::UpdateCreditDisplay);
	// 
	// // 초기 크레딧 값으로 UI 업데이트
	// AgentWidget->UpdateCreditDisplay(PS->GetCurrentCredit());

	// 이 함수는 더 이상 크레딧 바인딩에 사용되지 않음
	// 필요한 경우 다른 UI 요소 바인딩에 활용
}

void AAgentPlayerController::Client_ReceivePurchaseResult_Implementation(
	bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason)
{
	Client_ReceiveShopPurchase_Implementation(0, bSuccess, ItemID, ItemType, FailureReason);
}

uint32 AAgentPlayerController::AllocateShopRequestID()
{
	if (++LastShopRequestID == 0)
	{
		++LastShopRequestID;
	}
	return LastShopRequestID;
}

void AAgentPlayerController::Server_RequestShopPurchase_Implementation(int32 ItemID, EShopItemType ItemType, uint32 RequestID)
{
	if (IsValid(ShopComponent))
	{
		ShopComponent->PurchaseItem(ItemID, ItemType, RequestID);
	}
	else
	{
		Client_ReceiveShopPurchase(RequestID, false, ItemID, ItemType, TEXT("상점을 찾을 수 없습니다."));
	}
}

void AAgentPlayerController::Client_ReceiveShopPurchase_Implementation(
	uint32 RequestID, bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason)
{
	UE_LOG(LogTemp, Display, TEXT("Shop result (%s) item=%d type=%d success=%d request=%u reason=%s"),
		HasAuthority() ? TEXT("ListenHost") : TEXT("Client"), ItemID, static_cast<int32>(ItemType), bSuccess, RequestID, *FailureReason);
	OnShopPurchaseResponse.Broadcast(RequestID, bSuccess, ItemID, ItemType, FailureReason);
	OnServerPurchaseResult.Broadcast(bSuccess, ItemID, ItemType, FailureReason);
}

void AAgentPlayerController::HideFollowUpInputUI()
{
	Client_HideFollowUpInputUI();
}

void AAgentPlayerController::Client_HideFollowUpInputUI_Implementation()
{
	if (auto* MatchMapHud = Cast<UMatchMapHUD>(GetMatchMapHud()))
	{
		MatchMapHud->HideFollowUpInputUI();
	}
}

//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//             CYT             ♣
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

// 미니맵 초기화 함수
void AAgentPlayerController::InitializeMinimap()
{
	// 미니맵 위젯 클래스가 설정되어 있는지 확인
	if (MinimapWidgetClass)
	{
		// 미니맵 위젯 생성
		MinimapWidget = CreateWidget<UMiniMapWidget>(this, MinimapWidgetClass);
		if (MinimapWidget)
		{
			// 뷰포트에 위젯 추가
			MinimapWidget->AddToViewport(1); // Z-Order 1로 설정 (UI 레이어)
            
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, 미니맵 위젯이 생성되었습니다."), __FUNCTION__);
            
			// 미니맵 위젯 생성 후 에이전트 스캔은 위젯 내부에서 자동으로 수행됨
		}
		else
		{
			NET_LOG(LogTemp, Error, TEXT("%hs Called, 미니맵 위젯 생성 실패!"), __FUNCTION__);
		}
	}
	else
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, MinimapWidgetClass가 설정되지 않았습니다!"), __FUNCTION__);
	}
}

void AAgentPlayerController::OnRep_Pawn()
{
	CloseShopUI();
	Super::OnRep_Pawn();
	if (ShopViewModel)
	{
		ShopViewModel->Bind(*this);
	}
	
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	
	// 로컬 플레이어만 미니맵 생성 (멀티플레이어 최적화)
	// 현재 컨트롤러가 로컬 플레이어의 것인지 확인
	if (IsLocalPlayerController() && nullptr == MinimapWidget)
	{
		// 미니맵 초기화 함수 호출
		InitializeMinimap();
	}

	if (nullptr != GetPawn())
	{
		if (auto* Agent = Cast<ABaseAgent>(GetPawn()))
		{
			Agent->OnAgentDamaged.RemoveDynamic(this, &AAgentPlayerController::OnDamaged);
			Agent->OnAgentDamaged.AddDynamic(this, &AAgentPlayerController::OnDamaged);
			Agent->OnAgentHealed.RemoveDynamic(this,&AAgentPlayerController::OnHealed);
			Agent->OnAgentHealed.AddDynamic(this,&AAgentPlayerController::OnHealed);
			Agent->OnSwitchEquipment.RemoveDynamic(this,&AAgentPlayerController::OnSwitchWeapon);
			Agent->OnSwitchEquipment.AddDynamic(this,&AAgentPlayerController::OnSwitchWeapon);
			Agent->OnSpikeOwnChanged.RemoveDynamic(this,&AAgentPlayerController::OnSpikeOwnChanged);
			Agent->OnSpikeOwnChanged.AddDynamic(this,&AAgentPlayerController::OnSpikeOwnChanged);
		}
	}
}

void AAgentPlayerController::OnMatchEnd(const bool bBlueWin)
{
	if (auto* MatchMapHud = Cast<UMatchMapHUD>(GetMatchMapHud()))
	{
		const auto* PS = GetPlayerState<AAgentPlayerState>();
		if (PS)
		{
			const bool bWin = !(PS->bIsBlueTeam ^ bBlueWin);
			// NOT(XOR) 하면 아래 결과가 나옴
			// True, True = Win
			// True, False = False
			// False, True = False
			// False, False = True
			MatchMapHud->OnMatchEnd(bWin);
		}
		else
		{
			NET_LOG(LogTemp, Error, TEXT("%hs Called, PS is nullptr"), __FUNCTION__);
		}
	}
	else
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, AgentWidget is nullptr"), __FUNCTION__);
	}
}

void AAgentPlayerController::OnDamaged(const FVector& HitOrg, const EAgentDamagedPart AgentDamagedPart,
	const EAgentDamagedDirection AgentDamagedDirection, const bool bArg, const bool bCond, const bool bLowState)
{
	OnDamaged_PC.Broadcast(HitOrg, AgentDamagedPart, AgentDamagedDirection, bArg, bCond, bLowState);
}

void AAgentPlayerController::OnHealed(const bool bHighState)
{
	OnHealed_PC.Broadcast(bHighState);
}

void AAgentPlayerController::OnSpikePlanted(AMatchPlayerController* Planter)
{
	if (auto* MatchMapHud = Cast<UMatchMapHUD>(GetMatchMapHud()))
	{
		MatchMapHud->SpikePlanted();
	}
}

void AAgentPlayerController::OnSwitchWeapon(const EInteractorType EquipmentState)
{
	if (auto* MatchMapHud = Cast<UMatchMapHUD>(GetMatchMapHud()))
	{
		MatchMapHud->OnSwitchWeapon(EquipmentState);
	}
}

void AAgentPlayerController::OnSpikeOwnChanged(const bool bOwnSpike)
{
	if (auto* MatchMapHud = Cast<UMatchMapHUD>(GetMatchMapHud()))
	{
		MatchMapHud->OnSpikeOwnChanged(bOwnSpike);
	}
}
