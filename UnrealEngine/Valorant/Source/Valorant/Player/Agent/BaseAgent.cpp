// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "Valorant.h"
#include "AbilitySystem/Abilities/BaseGameplayAbility.h"
#include "AbilitySystem/Attributes/BaseAttributeSet.h"
#include "AbilitySystem/Context/HitScanGameplayEffectContext.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameManager/MatchGameMode.h"
#include "GameManager/MatchGameState.h"
#include "GameManager/SubsystemSteamManager.h"
#include "GameManager/ValorantGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/Animaiton/AgentAnimInstance.h"
#include "Player/Component/AgentInputComponent.h"
#include "Player/Widget/MiniMapWidget.h"
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "ValorantObject/BaseInteractor.h"
#include "ValorantObject/Spike/Spike.h"
#include "Player/Component/CreditComponent.h"
#include "Player/Component/FlashComponent.h"
#include "Player/Component/FlashPostProcessComponent.h"
#include "UI/FlashWidget.h"
#include "UI/MatchMap/MatchMapHUD.h"
#include "Weapon/BaseWeapon.h"
#include "NiagaraFunctionLibrary.h"
#include "AgentAbility/FlashProjectile.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

/* static */ EAgentDamagedPart ABaseAgent::GetHitDamagedPart(const FName& BoneName)
{
	const FString& NameStr = BoneName.ToString();
	if (NameStr.Contains(TEXT("Neck"), ESearchCase::IgnoreCase))
	{
		return EAgentDamagedPart::Head;
	}
		
	if (NameStr.Contains(TEXT("Clavicle"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Shoulder"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Elbow"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Hand"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Spine"), ESearchCase::IgnoreCase))
	{
		return EAgentDamagedPart::Body;
	}
		
	if (NameStr.Contains(TEXT("Hip"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Knee"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Foot"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Toe"), ESearchCase::IgnoreCase))
	{
		return EAgentDamagedPart::Legs;
	}
		
	return EAgentDamagedPart::None;
}

ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	// Root Capsule
	BaseCapsuleHalfHeight = 72.0f;
	CrouchCapsuleHalfHeight = 68.0f;
	GetCapsuleComponent()->SetCapsuleHalfHeight(BaseCapsuleHalfHeight);
	GetCapsuleComponent()->bReturnMaterialOnMove = true;
	
	// SpringArm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(0, 0, 60));
	SpringArm->TargetArmLength = 0;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 20.0f;
	SpringArm->bUsePawnControlRotation = true;
	BaseSpringArmHeight = SpringArm->GetRelativeLocation().Z;
	CrouchSpringArmHeight = BaseSpringArmHeight - 28.0f;

	// TP Mesh
	auto* TpMesh = GetMesh();
	TpMesh->SetupAttachment(GetRootComponent());
	TpMesh->SetRelativeScale3D(FVector(.34f));
	TpMesh->SetRelativeLocation(FVector(.0f, .0f, -90.f));
	TpMesh->SetCollisionProfileName(TEXT("Agent"));
	TpMesh->SetGenerateOverlapEvents(true);
	TpMesh->SetOwnerNoSee(true);
	TpMesh->SetCastShadow(false);
	
	// FP Mesh
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>("FirstPersonMesh");
	FirstPersonMesh->SetupAttachment(SpringArm);
	FirstPersonMesh->SetRelativeScale3D(FVector(.34f));
	FirstPersonMesh->SetRelativeLocation(FVector(0, 0, -120));
	FirstPersonMesh->AlwaysLoadOnClient = true;
	FirstPersonMesh->AlwaysLoadOnServer = true;
	FirstPersonMesh->bOwnerNoSee = false;
	FirstPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	FirstPersonMesh->bCastDynamicShadow = true;
	FirstPersonMesh->bAffectDynamicIndirectLighting = true;
	FirstPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	FirstPersonMesh->SetGenerateOverlapEvents(true);
	FirstPersonMesh->SetCollisionProfileName(TEXT("NoCollision"));
	FirstPersonMesh->SetCanEverAffectNavigation(false);
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetCastShadow(false);

	// Defusal Mesh
	DefusalMesh = CreateDefaultSubobject<USkeletalMeshComponent>("DefusalMesh");
	DefusalMesh->SetupAttachment(GetRootComponent());
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh> defusal(TEXT("'/Game/Resource/Props/Defuser/Defusal.Defusal'"));
	DefusalMesh->SetSkeletalMesh(defusal.Object);
	
	DefusalMesh->SetCollisionProfileName(TEXT("NoCollision"));
	DefusalMesh->SetVisibility(false);
	DefusalMesh->SetCastShadow(false);
	
	// Camera
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(FirstPersonMesh, TEXT("CameraSocket"));
	Camera->SetFieldOfView(90.f);
	Camera->SetRelativeLocation(FVector(15, 0, 0));

	// Interaction Capsule
	InteractionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionCapsule"));
	InteractionCapsule->SetupAttachment(Camera);
	InteractionCapsule->SetRelativeLocation(FVector(150, 0, 0));
	InteractionCapsule->SetRelativeRotation(FRotator(-90, 0, 0));
	InteractionCapsule->SetCapsuleHalfHeight(150);
	InteractionCapsule->SetCapsuleRadius(35);
	InteractionCapsule->SetCollisionProfileName(TEXT("Interactable"));
	
	// Character Movement Component
	auto* Movement = GetCharacterMovement();
	Movement->GravityScale = 0.9f; // Set Global Gravity Z -2100.f in Level - World Settings
	Movement->MaxAcceleration = 1800.f;
	Movement->GroundFriction = 3.3f;
	Movement->MaxWalkSpeed = BaseRunSpeed;
	Movement->BrakingDecelerationWalking = 225.f;
	Movement->bIgnoreBaseRotation = true;
	Movement->JumpZVelocity = 600.f;
	Movement->BrakingDecelerationFalling = 1200.f;
	Movement->AirControl = 0.45f;
	Movement->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
	
	Movement->SetCrouchedHalfHeight(BaseCapsuleHalfHeight);
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->MaxWalkSpeedCrouched = 330.0f;

	// Agent Input Component
	AgentInputComponent = CreateDefaultSubobject<UAgentInputComponent>("InputComponent");
	
	TL_Crouch = CreateDefaultSubobject<UTimelineComponent>("TL_Crouch");
	TL_DieCamera = CreateDefaultSubobject<UTimelineComponent>("TL_DieCamera");

	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             LCH             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	
	// 섬광 컴포넌트 생성
	FlashComponent = CreateDefaultSubobject<UFlashComponent>(TEXT("FlashComponent"));
    
	// 포스트 프로세스 컴포넌트 생성
	PostProcessComponent = CreateDefaultSubobject<UFlashPostProcessComponent>(TEXT("PostProcessComponent"));

	// 적/아군 테두리 외곽선 머터리얼 로딩
	static const ConstructorHelpers::FObjectFinder<UMaterialInterface> EnemyMaterialFinder(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Resource/CommonMaterials/MI_Highlight_Enemy.MI_Highlight_Enemy'"));
	if (EnemyMaterialFinder.Succeeded())
	{
		EnemyOverlayMaterial = EnemyMaterialFinder.Object;
	}
	static const ConstructorHelpers::FObjectFinder<UMaterialInterface> TeamMaterialFinder(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/Resource/CommonMaterials/MI_Highlight_Team.MI_Highlight_Team'"));
	if (TeamMaterialFinder.Succeeded())
	{
		TeamOverlayMaterial = TeamMaterialFinder.Object;
	}
}

void ABaseAgent::OnRep_CurrentInteractorState()
{
	if (CurrentInteractor && CurrentEquipmentState != EInteractorType::None)
	{
		CurrentInteractor->PlayEquipAnimation();
		OnSwitchEquipment.Broadcast(CurrentEquipmentState);
	}
}

// 서버 전용. 캐릭터를 Possess할 때 호출됨. 게임 첫 시작시, BeginPlay 보다 먼저 호출됩니다.
void ABaseAgent::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//국룰 위치
	InitAgentAbility();

	AAgentPlayerController* pc = Cast<AAgentPlayerController>(NewController);
	if (pc)
	{
		BindToDelegatePC(pc);
	}

	if (IsLocallyControlled())
	{
		InteractionCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABaseAgent::OnFindInteraction);
		InteractionCapsule->OnComponentEndOverlap.AddDynamic(this, &ABaseAgent::OnInteractionCapsuleEndOverlap);
	}

	CreateFlashWidget();

	/*
	 *	실루엣 관련
	 */
	if (const auto* OtherPS = GetPlayerState<AAgentPlayerState>())
	{
		if (IsLocallyControlled())
		{
			return;
		}
		
		const auto* MyPC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>();
		const auto* MyPS = MyPC->GetPlayerState<AAgentPlayerState>();
		if (MyPS->bIsBlueTeam == OtherPS->bIsBlueTeam)
		{
			SetHighlight(true, false);
		}
		else
		{
			SetHighlight(true, true);
		}
	}

	DevCameraMode(true);
}

// 클라이언트 전용. 서버로부터 PlayerState를 최초로 받을 때 호출됨
void ABaseAgent::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//국룰 위치
	InitAgentAbility();

	AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetController());
	if (pc)
	{
		// UE_LOG(LogTemp, Warning, TEXT("클라, 델리게이트 바인딩"));
		BindToDelegatePC(pc);
	}

	// 로컬 플레이어에게만 UI 생성
	CreateFlashWidget();

	/*
	 *	실루엣 관련
	 */
	if (const auto* OtherPS = GetPlayerState<AAgentPlayerState>())
	{
		if (IsLocallyControlled())
		{
			return;
		}
		
		const auto* MyPC = GetWorld()->GetFirstPlayerController<AAgentPlayerController>();
		const auto* MyPS = MyPC->GetPlayerState<AAgentPlayerState>();
		if (MyPS)
		{
			if (MyPS->bIsBlueTeam == OtherPS->bIsBlueTeam)
			{
				SetHighlight(true, false);
			}
			else
			{
				SetHighlight(true, true);
			}
		}
	}

	DevCameraMode(true);
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();
	
	// 어빌리티 초기화 플래그 리셋 (리스폰 시)
	bAbilityInitialized = false;

	ABP_1P = Cast<UAgentAnimInstance>(FirstPersonMesh->GetAnimInstance());
	ABP_3P = Cast<UAgentAnimInstance>(GetMesh()->GetAnimInstance());

	if (IsLocallyControlled())
	{
		DefusalMesh->AttachToComponent(GetMesh1P(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,FName(TEXT("L_SpikeSocket")));
	}
	else
	{
		DefusalMesh->AttachToComponent(GetMesh(),FAttachmentTransformRules::SnapToTargetNotIncludingScale,FName(TEXT("L_SpikeSocket")));
	}
	DefusalMesh->SetRelativeLocation(FVector::ZeroVector);
	DefusalMesh->SetRelativeRotation(FRotator::ZeroRotator);
	DefusalMesh->SetRelativeScale3D(FVector(.34f));

	if (CrouchCurve)
	{
		FOnTimelineFloat CrouchOffset;
		CrouchOffset.BindUFunction(this, FName("HandleCrouchProgress"));
		TL_Crouch->AddInterpFloat(CrouchCurve, CrouchOffset);
	}

	TL_Crouch->SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);

	if (DieCameraCurve && DieCameraPitchCurve)
	{
		FOnTimelineVector CamOffset;
		FOnTimelineFloat CamPitch;
		CamOffset.BindUFunction(this, FName("HandleDieCamera"));
		CamPitch.BindUFunction(this, FName("HandleDieCameraPitch"));
		TL_DieCamera->AddInterpVector(DieCameraCurve, CamOffset);
		TL_DieCamera->AddInterpFloat(DieCameraPitchCurve, CamPitch);
	}

	TL_DieCamera->SetTimelineLength(DieCameraTimeRange);
	TL_DieCamera->SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);

	// 기본 무기 쥐어주기
	if (HasAuthority()) 
	{
		FTimerHandle SpawnWeaponTimerHandle;
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate TimerDel;
			TimerDel.BindLambda([this, World]()
			{
				m_GameMode = World->GetAuthGameMode<AMatchGameMode>();
				if (m_GameMode)
				{
					m_GameMode->SpawnDefaultWeapon(this);
					m_GameMode->OnStartInRound.AddDynamic(this,&ABaseAgent::StartLogging);
					m_GameMode->OnEndRound.AddDynamic(this,&ABaseAgent::StopLogging);
				}
			});
            
			World->GetTimerManager().SetTimer(SpawnWeaponTimerHandle,TimerDel,1.0f,false);
		}
	}
	
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             LCH             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
    
	// 섬광 강도 변경 델리게이트 바인딩
	if (FlashComponent)
	{
		FlashComponent->OnFlashIntensityChanged.AddDynamic(this, &ABaseAgent::OnFlashIntensityChanged);
	}
	
}

void ABaseAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float baseSpeed = BaseRunSpeed;

	if (!bIsRun)
	{
		baseSpeed = BaseWalkSpeed;
	}

	GetCharacterMovement()->MaxWalkSpeed = baseSpeed * EffectSpeedMultiplier * EquipSpeedMultiplier;

	if (HasAuthority() && Controller)
	{
		ReplicatedControlRotation = Controller->GetControlRotation();
	}
	
	if (HasAuthority())
	{
		CheckMinimapVisibility(DeltaTime);
	}
}

void ABaseAgent::InitAgentAbility()
{
	AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>();
	if (ps == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerState를 AAgentPlayerState를 상속받는 녀석으로 교체 부탁"));
		return;
	}

	// 이미 초기화되었는지 확인
	if (bAbilityInitialized)
	{
		NET_LOG(LogTemp, Warning, TEXT("어빌리티가 이미 초기화되었습니다. 중복 초기화 방지"));
		return;
	}

	ps->OnKillDelegate.AddDynamic(this, &ABaseAgent::OnKill);

	ASC = ps->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(ps, this);

	if (HasAuthority())
	{
		// 스킬 등록 및 값 초기화는 서버에서만 진행
		ASC->InitializeByAgentData(ps->GetAgentID());

		UE_LOG(LogTemp, Warning, TEXT("=== ASC 등록된 GA 목록 ==="));
		for (const FGameplayAbilitySpec& spec : ASC->GetActivatableAbilities())
		{
			if (spec.Ability)
			{
				UE_LOG(LogTemp, Warning, TEXT("GA: %s"), *spec.Ability->GetName());
        
				FString TagString;
				TArray<FGameplayTag> tags = spec.GetDynamicSpecSourceTags().GetGameplayTagArray();
				for (const FGameplayTag& Tag : tags)
				{
					TagString += Tag.ToString() + TEXT(" ");
				}
        
				UE_LOG(LogTemp, Warning, TEXT("태그 목록: %s"), *TagString);
			}
		}
	}

	bAbilityInitialized = true;
}

void ABaseAgent::BindToDelegatePC(AAgentPlayerController* pc)
{
	pc->OnHealthChanged_PC.AddDynamic(this, &ABaseAgent::UpdateHealth);
	pc->OnMaxHealthChanged_PC.AddDynamic(this, &ABaseAgent::UpdateMaxHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &ABaseAgent::UpdateArmor);
	pc->OnEffectSpeedChanged_PC.AddDynamic(this, &ABaseAgent::UpdateEffectSpeed);

	PC = pc;
	m_Hud = Cast<UMatchMapHUD>(pc->GetMatchMapHud());
}

void ABaseAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AgentInputComponent->BindInput(PlayerInputComponent);
}

FString ABaseAgent::GetPlayerNickname() const
{
	if (const AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>())
	{
		return PS->DisplayName;
	}
	return FString(TEXT("INVALID_NAME"));
}

void ABaseAgent::SetHighlight(bool bEnable, bool bIsEnemy)
{
	if (bEnable)
	{
		if (bIsEnemy)
		{
			GetMesh()->SetOverlayMaterial(EnemyOverlayMaterial);
		}
		else
		{
			GetMesh()->SetRenderCustomDepth(true);
			GetMesh()->SetOverlayMaterial(TeamOverlayMaterial);
		}
	}
	else
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}
}

void ABaseAgent::SetIsRun(const bool _bIsRun)
{
	if (HasAuthority())
	{
		bIsRun = _bIsRun;
	}
	else
	{
		Server_SetIsRun(_bIsRun);
	}
}

void ABaseAgent::Server_SetIsRun_Implementation(const bool _bIsRun)
{
	bIsRun = _bIsRun;
}

void ABaseAgent::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	// Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	//NET_LOG(LogTemp,Warning,TEXT("OnStartCrouch"));
	TL_Crouch->PlayFromStart();
}

void ABaseAgent::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	// Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	//NET_LOG(LogTemp,Warning,TEXT("OnEndCrouch"));
	TL_Crouch->Reverse();
}

void ABaseAgent::StartFire()
{
	// 어빌리티 활성화중이라면 무기 강제 전환 X
	if (IsAbilityActivating()) return;
	
	if (CurrentInteractor == nullptr)
	{
		// NET_LOG(LogTemp, Warning, TEXT("%hs Called, CurrentInteractor is nullptr"), __FUNCTION__);
		if (CurrentEquipmentState == EInteractorType::Ability)
		{
			if (MainWeapon)
			{
				SwitchEquipment(EInteractorType::MainWeapon);
			}
			else if (SubWeapon)
			{
				SwitchEquipment(EInteractorType::SubWeapon);
			}
			else
			{
				SwitchEquipment(EInteractorType::Melee);
			}
		}
		else
		{
			return;
		}
	}
	
	if (auto* weapon = Cast<ABaseWeapon>(CurrentInteractor))
	{
		weapon->StartFire();
		return;
	}

	if (Spike && Spike->GetSpikeState() == ESpikeState::Carried)
	{
		Spike->ServerRPC_Interact(this);
	}
}

void ABaseAgent::EndFire()
{
	if (CurrentInteractor == nullptr)
	{
		return;
	}
	
	if (auto* weapon = Cast<ABaseWeapon>(CurrentInteractor))
	{
		weapon->EndFire();

		if (bIsInRound)
		{
			if (HasAuthority())
			{
				m_GameMode->SubmitShotLog(PC,CachedFireCount,CachedHitCount,CachedHeadshotCount,CachedDamage);
				InitLog();
			}
			else
			{
				ServerRPC_SubmitLog();
			}
		}
		
		return;
	}

	if (Spike && Spike->GetSpikeState() == ESpikeState::Planting)
	{
		CancelSpike(Spike);
	}	
}

void ABaseAgent::Reload()
{
	if (CurrentInteractor == nullptr)
	{
		return;
	}

	if (ABaseWeapon* weapon = Cast<ABaseWeapon>(CurrentInteractor))
	{
		weapon->ServerRPC_StartReload();
	}
}

void ABaseAgent::Interact()
{
	if (FindInteractActor)
	{
		ASpike* spike = Cast<ASpike>(FindInteractActor);
		if (spike && spike->GetSpikeState() == ESpikeState::Planted)
		{
			return;
		}
		
		if (ABaseInteractor* Interactor = Cast<ABaseInteractor>(FindInteractActor))
		{
			ServerRPC_Interact(Interactor);
			FindInteractActor = nullptr;
		}
	}
}

void ABaseAgent::ServerRPC_Interact_Implementation(ABaseInteractor* Interactor)
{
	if (nullptr == Interactor)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, Interactor is nullptr"), __FUNCTION__);
		return;
	}
	
	Interactor->ServerRPC_Interact(this);
}

void ABaseAgent::ServerRPC_DropCurrentInteractor_Implementation()
{
	if (CurrentInteractor && CurrentInteractor->ServerOnly_CanDrop())
	{
		CurrentInteractor->ServerRPC_Drop();
		ResetCurrentInteractor();

		if (MainWeapon != nullptr)
		{
			// NET_LOG(LogTemp,Warning,TEXT("Main있음. %s"),*MainWeapon->GetActorNameOrLabel());
			SwitchEquipment(EInteractorType::MainWeapon);
		}
		else if (SubWeapon != nullptr)
		{
			// NET_LOG(LogTemp,Warning,TEXT("Main 없고, Sub있음"));
			SwitchEquipment(EInteractorType::SubWeapon);
		}
		else
		{
			// NET_LOG(LogTemp,Warning,TEXT("Main 없고, Sub 없음"));
			SwitchEquipment(EInteractorType::Melee);
		}
	}
}

void ABaseAgent::ServerRPC_SetCurrentInteractor_Implementation(ABaseInteractor* interactor)
{
	if (CurrentInteractor)
	{
		CurrentInteractor->SetActive(false);
	}
	
	CurrentInteractor = interactor;
	CurrentEquipmentState = CurrentInteractor ? CurrentInteractor->GetInteractorType() : EInteractorType::None;
	OnRep_CurrentInteractorState();
	
	if (CurrentInteractor)
	{
		CurrentInteractor->SetActive(true);
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, 현재 장착 중인 Interactor: %s"), __FUNCTION__, *CurrentInteractor->GetActorNameOrLabel());
	}
}

void ABaseAgent::ResetCurrentInteractor()
{
	if (CurrentInteractor == MainWeapon)
	{
		NET_LOG(LogTemp,Warning,TEXT("버린 물건이 Main"));
		MainWeapon = nullptr;
	}
	else if (CurrentInteractor == SubWeapon)
	{
		NET_LOG(LogTemp,Warning,TEXT("버린 물건이 Sub"));
		SubWeapon = nullptr;
	}
	else if (CurrentInteractor == Spike)
	{
		NET_LOG(LogTemp,Warning,TEXT("버린 물건이 Spike"));
		Spike = nullptr;
	}
	
	CurrentInteractor = nullptr;
}

ABaseWeapon* ABaseAgent::GetMainWeapon() const
{
	return MainWeapon;
}

ABaseWeapon* ABaseAgent::GetSubWeapon() const
{
	return SubWeapon;
}

ABaseWeapon* ABaseAgent::GetMeleeWeapon() const
{
	return MeleeKnife;
}

void ABaseAgent::ResetOwnSpike()
{
	Spike = nullptr;
}

void ABaseAgent::AcquireInteractor(ABaseInteractor* Interactor)
{
	if (Interactor == nullptr)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, Interactor is nullptr"), __FUNCTION__);
		return;
	}
	if (!HasAuthority())
	{
		Server_AcquireInteractor(Interactor);
		return;
	}
	
	auto* spike = Cast<ASpike>(Interactor);
	if (spike)
	{
		Spike = spike;

		Multicast_OnSpikeOwnChanged(true);
		return;
	}

	auto* weapon = Cast<ABaseWeapon>(Interactor);
	if (!weapon)
	{
		return;
	}

	if (weapon->GetWeaponCategory() == EWeaponCategory::Sidearm)
	{
		if (SubWeapon)
		{
			SubWeapon->ServerRPC_Drop();
		}
		SubWeapon = weapon;
	}
	else if (weapon->GetWeaponCategory() == EWeaponCategory::Melee)
	{
		MeleeKnife = weapon;
	}
	else
	{
		if (MainWeapon)
		{
			MainWeapon->ServerRPC_Drop();
		}
		MainWeapon = weapon;
	}
	
	// 무기를 얻으면, 해당 무기의 타입의 슬롯으로 전환해 바로 장착하도록
	SwitchEquipment(Interactor->GetInteractorType());
}

void ABaseAgent::SwitchEquipment(EInteractorType EquipmentType)
{
	if (!HasAuthority())
	{
		ServerRPC_SwitchEquipment(EquipmentType);
		return;
	}
    
	// 동일한 장비로 전환 시도 시 무시
	if (EquipmentType == CurrentEquipmentState)
	{
		return;
	}
    
	// 어빌리티 상태 확인
	if (ASC.IsValid())
	{
		// 무기 전환 차단 태그 체크
		if (ASC->HasMatchingGameplayTag(FValorantGameplayTags::Get().Block_WeaponSwitch))
		{
			NET_LOG(LogTemp, Warning, TEXT("어빌리티 사용 중 무기 전환 차단됨"));
			return;
		}

		// 어빌리티 강제 취소
		NET_LOG(LogTemp, Display, TEXT("무기 전환을 위해 활성 어빌리티 취소"));
		ASC->ForceCleanupAllAbilities();
		PC->HideFollowUpInputUI();
		
		// 약간의 딜레이 후 무기 전환
		FTimerHandle DelayedSwitchTimer;
		GetWorld()->GetTimerManager().SetTimer(DelayedSwitchTimer, [this, EquipmentType]()
		{
			PerformWeaponSwitch(EquipmentType);
		}, 0.1f, false);
		return;
	}
    
	// 정상적인 무기 전환
	PerformWeaponSwitch(EquipmentType);
}

void ABaseAgent::PerformWeaponSwitch(EInteractorType EquipmentType)
{
	// 이전 상태 저장
	if (CurrentInteractor)
	{
		PrevEquipmentState = CurrentEquipmentState;
	}
    
	// 새 장비로 전환
	switch (EquipmentType)
	{
	case EInteractorType::Ability:
		EquipInteractor(nullptr);
		CurrentEquipmentState = EInteractorType::Ability;
		break;
        
	case EInteractorType::MainWeapon:
		if (MainWeapon)
		{
			EquipInteractor(MainWeapon);
		}
		break;
        
	case EInteractorType::SubWeapon:
		if (SubWeapon)
		{
			EquipInteractor(SubWeapon);
		}
		break;
        
	case EInteractorType::Melee:
		if (MeleeKnife)
		{
			EquipInteractor(MeleeKnife);
		}
		break;
        
	case EInteractorType::Spike:
		if (Spike)
		{
			EquipInteractor(Spike);
		}
		break;
	}
    
	UpdateEquipSpeedMultiplier();
}


void ABaseAgent::ActivateSpike()
{
	if (IsInPlantZone)
	{
		// 스파이크 소지자이고, 설치 상태이면 설치
		if (Spike && Spike->GetSpikeState() == ESpikeState::Carried)
		{
			if (CurrentInteractor)
			{
				CurrentInteractor->SetActive(false);
			}
			
			// 스파이크를 들지 않은 상태에서 설치하려 할 경우, 장착 로직 따로 실행
			if (CurrentInteractor != Spike)
			{
				CurrentInteractor = Spike;
			}
			ServerRPC_Interact(Spike);
		}
		// 스파이크 해제 가능 상태이면 스파이크 해제
		else if (auto* spike = Cast<ASpike>(FindInteractActor))
		{
			if (CurrentInteractor)
			{
				CurrentInteractor->SetActive(false);
			}
			Spike = spike;
			ServerRPC_Interact(spike);
		}
	}
	else
	{
		if (Spike && Spike->GetSpikeState() == ESpikeState::Planted)
		{
			return;
		}
		SwitchEquipment(EInteractorType::Spike);
	}
}

void ABaseAgent::CancelSpike(ASpike* CancelObject)
{
	if (CancelObject == nullptr)
	{
		if (Spike)
		{
			CancelObject = Spike;
		}
		else if (ASpike* spike = Cast<ASpike>(FindInteractActor))
		{
			CancelObject = spike;
		}
		else
		{
			return;
		}
	}

	if (HasAuthority() && CancelObject)
	{
		CancelObject->ServerRPC_Cancel(this);
	}
	else
	{
		ServerRPC_CancelSpike(CancelObject);
	}
}

void ABaseAgent::ServerRPC_CancelSpike_Implementation(ASpike* CancelObject)
{
	CancelSpike(CancelObject);
}

void ABaseAgent::Server_AcquireInteractor_Implementation(ABaseInteractor* Interactor)
{
	AcquireInteractor(Interactor);
}

void ABaseAgent::ServerRPC_SwitchEquipment_Implementation(EInteractorType InteractorType)
{
	SwitchEquipment(InteractorType);
}

void ABaseAgent::SetShopUI()
{
	if (IsLocallyControlled())
	{
		// 현재 라운드 상태 확인
		// GameMode 대신 GameState를 사용 (클라이언트에서 접근 가능)
		AMatchGameState* GameState = GetWorld()->GetGameState<AMatchGameState>();
		if (GameState)
		{
			// 구매 페이즈인지 확인
			if (GameState->CanOpenShop())
			{
				// 구매 페이즈일 때만 상점 UI 열기
				PC->RequestShopUI();
			}
			else
			{
				// 구매 페이즈가 아닐 때는 알림 메시지 표시
				FString Message = TEXT("상점은 구매 페이즈에서만 이용할 수 있습니다.");

				// 알림 메시지 표시 (이미 열려있는 상점이 있으면 닫음)
				if (PC)
				{
					PC->Client_ReceivePurchaseResult(false, 0, EShopItemType::None, Message);
					PC->CloseShopUI();
				}
			}
		}
		else
		{
			// GameState를 찾을 수 없는 경우 기존 동작 유지
			PC->RequestShopUI();
		}
	}
}

/** 실 장착관련 로직 */
void ABaseAgent::EquipInteractor(ABaseInteractor* interactor)
{
	ServerRPC_SetCurrentInteractor(interactor);
}

void ABaseAgent::OnFindInteraction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	// 이미 바라보고 있는 총이 있으면 리턴
	if (FindInteractActor)
	{
		if (FindInteractActor->HasOwnerAgent())
		{
			FindInteractActor = nullptr;
		}
		else
		{
			// NET_LOG(LogTemp, Warning, TEXT("%hs Called, 이미 감지된 Interactor가 있음"), __FUNCTION__);
			return;
		}
	}

	if (auto* Interactor = Cast<ABaseInteractor>(OtherActor))
	{
		if (CurrentInteractor == Interactor)
		{
			// NET_LOG(LogTemp, Error, TEXT("%hs Called, 현재 들고 있는 Interactor와 동일함"), __FUNCTION__);
			return;
		}
		if (Interactor->HasOwnerAgent())
		{
			// NET_LOG(LogTemp, Warning, TEXT("%hs Called, 이미 주인이 있는 Interactor"), __FUNCTION__);
			return;
		}
		if (const auto* DetectedSpike = Cast<ASpike>(Interactor))
		{
			if (const auto* PS = GetPlayerState<AMatchPlayerState>())
			{
				if (PS->bIsAttacker)
				{
					// 공격팀인데 스파이크가 이미 설치된 상태라면 감지 X
					if (DetectedSpike->GetSpikeState() == ESpikeState::Planted)
					{
						return;
					}
				}
				else
				{
					// 수비팀인데 스파이크가 설치된 상태가 아니라면 감지 X
					if (DetectedSpike->GetSpikeState() != ESpikeState::Planted)
					{
						return;
					}
				}
			}
		}
		
		// NET_LOG(LogTemp, Warning, TEXT("%hs Called, Interactor Name is %s"), __FUNCTION__, *Interactor->GetName());
		FindInteractActor = Interactor;
		FindInteractActor->OnDetect(true);
	}
	else
	{
		// NET_LOG(LogTemp, Error, TEXT("%hs Called, OtherActor is nullptr or not interactor, OtherActor Name is %s"), __FUNCTION__, *OtherActor->GetName());
	}
}

void ABaseAgent::OnInteractionCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const auto* interactor = Cast<ABaseInteractor>(OtherActor))
	{
		if (interactor == FindInteractActor)
		{
			FindInteractActor->OnDetect(false);
			FindInteractActor = nullptr;
		}
	}
}

void ABaseAgent::HandleCrouchProgress(float Value)
{
	float newHalfHeight = FMath::Lerp(
	   BaseCapsuleHalfHeight,
	   CrouchCapsuleHalfHeight,
	   Value
   );
	GetCapsuleComponent()->SetCapsuleHalfHeight(newHalfHeight, true);

	float newSprinArmHeight = FMath::Lerp(
	   BaseSpringArmHeight,
	   CrouchSpringArmHeight,
	   Value
   );
	SpringArm->SetRelativeLocation(FVector(SpringArm->GetRelativeLocation().X,SpringArm->GetRelativeLocation().Y, newSprinArmHeight));
}

void ABaseAgent::HandleDieCamera(FVector newPos)
{
	Camera->SetRelativeLocation(newPos);
	//UE_LOG(LogTemp,Warning,TEXT("pos: %f"),newPos.Z);
}


void ABaseAgent::HandleDieCameraPitch(float newPitch)
{
	Camera->SetRelativeRotation(FRotator(newPitch, 0, 0));
	//UE_LOG(LogTemp,Warning,TEXT("pitch %f"),newPitch);
}

/** 서버에서만 호출됨*/
void ABaseAgent::Die()
{
	NET_LOG(LogTemp, Display, TEXT("%s 사망 처리 시작"), *GetName());

	DevCameraMode(false);
	// 1. 어빌리티 정리를 가장 먼저 수행
	CancelActiveAbilities();

	// 어빌리티 완전 제거 
	if (ASC.IsValid() && HasAuthority())
	{
		ASC->ResetAgentAbilities();
	}

	// 초기화 플래그 리셋
	bAbilityInitialized = false;
    
	// 2. 장비 상태 초기화
	CurrentEquipmentState = EInteractorType::None;
	PrevEquipmentState = EInteractorType::None;
    
	// 3. 모든 장비 드롭
	if (MainWeapon)
	{
		MainWeapon->ServerRPC_Drop();
		MainWeapon = nullptr;
	}
	if (SubWeapon)
	{
		SubWeapon->ServerRPC_Drop();
		SubWeapon = nullptr;
	}
	if (MeleeKnife)
	{
		MeleeKnife->Destroy();
		MeleeKnife = nullptr;
	}
	if (Spike)
	{
		Spike->ServerRPC_Drop();
		Spike = nullptr;
	}
    
	// 4. 게임모드에 죽음 알림
	if (AMatchGameMode* GameMode = GetWorld()->GetAuthGameMode<AMatchGameMode>())
	{
		GameMode->OnDie(PC);
	}
    
	// 5. 킬 피드 처리
	ABaseAgent* InstigatorAgent = Cast<ABaseAgent>(GetInstigator());
	if (InstigatorAgent && InstigatorAgent != this)
	{
		if (AMatchPlayerController* KillerPC = Cast<AMatchPlayerController>(InstigatorAgent->GetController()))
		{
			if (AMatchGameMode* GameMode = GetWorld()->GetAuthGameMode<AMatchGameMode>())
			{
				GameMode->OnKill(Cast<AAgentPlayerController>(KillerPC), PC);
			}
		}
	}
    
	// 6. 클라이언트 동기화
	MulticastRPC_Die(InstigatorAgent, this, LastKillFeedInfo);
    
	// 7. 카메라 처리 타이머
	GetWorldTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda([this]()
	{
		OnDieCameraFinished();
	}), DieCameraTimeRange, false);
    
	// 8. 메시 가시성 설정
	GetMesh()->SetOwnerNoSee(false);
}

void ABaseAgent::OnDieCameraFinished()
{
	AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetController());
	if (pc)
	{
		pc->StartSpectatingOnly();
		Destroy();

		pc->Client_EnterSpectatorMode();
	}
	else
	{
		NET_LOG(LogTemp, Error, TEXT("OnDieCameraFinished: Controller가 없습니다!"));
	}
}

void ABaseAgent::MulticastRPC_Die_Implementation(ABaseAgent* InstigatorAgent, ABaseAgent* VictimAgent, const FKillFeedInfo& Info)
{
	if (IsLocallyControlled())
	{
		GetMesh()->SetOwnerNoSee(false);
		
		DisableInput(Cast<APlayerController>(GetController()));

		FirstPersonMesh->SetOwnerNoSee(false);
		FirstPersonMesh->SetVisibility(false);

		TL_DieCamera->PlayFromStart();
	}

	// NET_LOG(LogTemp, Warning, TEXT("%hs Called, InstigatorAgentName: %s"), __FUNCTION__, *InstigatorAgent->GetName());
	OnAgentDie.Broadcast(InstigatorAgent, VictimAgent, Info);
	if (auto* LocalController = GetWorld()->GetFirstPlayerController<AAgentPlayerController>())
	{
		LocalController->OnKillEvent(InstigatorAgent, VictimAgent, Info);
	}
	bIsDead = true;
}

void ABaseAgent::ServerApplyGE_Implementation(TSubclassOf<UGameplayEffect> geClass, ABaseAgent* DamageInstigator)
{
	if (!geClass)
	{
		NET_LOG(LogTemp, Error, TEXT("올바른 게임이펙트를 넣어주세요."));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(geClass, 1.f, Context);

	if (DamageInstigator)
	{
		SetInstigator(DamageInstigator);
		LastDamagedOrg = DamageInstigator->GetActorLocation();
	}

	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ABaseAgent::ServerApplyHitScanGE_Implementation(TSubclassOf<UGameplayEffect> GEClass, const int Damage,
	ABaseAgent* DamageInstigator, const int WeaponID, const EAgentDamagedPart DamagedPart, const EAgentDamagedDirection DamagedDirection)
{
	if (!GEClass)
	{
		NET_LOG(LogTemp, Error, TEXT("올바른 게임이펙트를 넣어주세요."));
		return;
	}

	FGameplayEffectContextHandle Context = FGameplayEffectContextHandle(new FHitScanGameplayEffectContext());
	FHitScanGameplayEffectContext* HitScanContext = static_cast<FHitScanGameplayEffectContext*>(Context.Get());
	HitScanContext->Damage = Damage;

	// Instigator 설정
	if (DamageInstigator)
	{
		// GAS에서 Instigator를 설정하고 Die() 함수에서 GetInstigator()로 확인
		SetInstigator(DamageInstigator);
		LastDamagedOrg = DamageInstigator->GetActorLocation();
		LastDamagedPart = DamagedPart;
		LastDamagedDirection = DamagedDirection;
		FKillFeedInfo KillFeedInfo;
		KillFeedInfo.Reason = EKillFeedReason::EKPR_Weapon;
		KillFeedInfo.SubReason = (LastDamagedPart == EAgentDamagedPart::Head) ? EKillFeedSubReason::EKPSR_Headshot : EKillFeedSubReason::EKPSR_None;
		KillFeedInfo.WeaponID = WeaponID;
		LastKillFeedInfo = KillFeedInfo;

		// 디버깅 로그
		NET_LOG(LogTemp, Warning, TEXT("데미지 적용: %s가 %s에게 %d 데미지를 입혔습니다."),
				*DamageInstigator->GetName(), *GetName(), Damage);
	}

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ABaseAgent::UpdateHealth(float newHealth, bool bIsDamage)
{
	if (!bIsDamage)
	{
		if (HasAuthority() && bIsInRound)
		{
			MulticastRPC_OnHealed(true);
		}
		return;
	}
	
	if (newHealth <= 0.f && bIsDead == false)
	{
		if (HasAuthority())
		{
			MulticastRPC_OnDamaged(LastDamagedOrg, LastDamagedPart, LastDamagedDirection, true, false);
		}
		Die();
	}
	else
	{
		if (HasAuthority())
		{
			if (newHealth < 34.0f)
			{
				MulticastRPC_OnDamaged(LastDamagedOrg, LastDamagedPart, LastDamagedDirection, false, false, true);
			}
			else
			{
				MulticastRPC_OnDamaged(LastDamagedOrg, LastDamagedPart, LastDamagedDirection, false, false);
			}
		}
	}

	// 스킬 데미지일 경우에는 슬로우 효과 적용 X
	if (GetLocalRole() == ROLE_AutonomousProxy && IsGunDamage())
	{
		ServerApplyGE(GE_HitSlow, nullptr);
	}
    
	LastDamagedOrg = FVector::ZeroVector;
	LastDamagedPart = EAgentDamagedPart::None;
	LastDamagedDirection = EAgentDamagedDirection::Front;
}

void ABaseAgent::UpdateMaxHealth(float newMaxHealth)
{
}

void ABaseAgent::UpdateArmor(float newArmor)
{
}

void ABaseAgent::UpdateEffectSpeed(float newSpeed)
{
	// NET_LOG(LogTemp, Warning, TEXT("%f dp"), newSpeed);
	EffectSpeedMultiplier = newSpeed;
}

void ABaseAgent::MulticastRPC_OnDamaged_Implementation(const FVector& HitOrg, const EAgentDamagedPart DamagedPart,
	const EAgentDamagedDirection DamagedDirection, const bool bDie, const bool bLarge, const bool bLowState)
{
	// NET_LOG(LogTemp, Warning, TEXT("%hs Called, DamagedPart: %s, DamagedDir: %s, Die: %hs, Large: %hs"),
		// __FUNCTION__, *EnumToString(DamagedPart), *EnumToString(DamagedDirection), bDie ? "True" : "False", bLarge ? "True" : "False");
	OnAgentDamaged.Broadcast(HitOrg, DamagedPart, DamagedDirection, bDie, bLarge, bLowState);
}

void ABaseAgent::MulticastRPC_OnHealed_Implementation(const bool bHighState)
{
	OnAgentHealed.Broadcast(bHighState);
}

// 무기 카테고리에 따른 이동 속도 멀티플라이어 업데이트
void ABaseAgent::UpdateEquipSpeedMultiplier()
{
	if (HasAuthority())
	{
		// 기본값으로 리셋
		EquipSpeedMultiplier = 1.0f;

		// 무기가 있으면 카테고리에 따라 속도 설정
		if (CurrentInteractor)
		{
			ABaseWeapon* CurrentWeapon = Cast<ABaseWeapon>(CurrentInteractor);
			if (CurrentWeapon)
			{
				// GameInstance에서 무기 데이터 가져오기
				UValorantGameInstance* GameInstance = Cast<UValorantGameInstance>(GetGameInstance());
				if (GameInstance)
				{
					FWeaponData* WeaponData = GameInstance->GetWeaponData(CurrentWeapon->GetWeaponID());
					if (WeaponData)
					{
						// 무기 종류에 따른 이동 속도 조정
						switch (WeaponData->WeaponCategory)
						{
						case EWeaponCategory::Sidearm:
							EquipSpeedMultiplier = 1.0f; // 기본 속도
							break;
						case EWeaponCategory::SMG:
							EquipSpeedMultiplier = 0.95f; // 약간 감소
							break;
						case EWeaponCategory::Rifle:
						case EWeaponCategory::Shotgun:
							EquipSpeedMultiplier = 0.9f; // 더 감소
							break;
						case EWeaponCategory::Sniper:
							EquipSpeedMultiplier = 0.85f; // 많이 감소
							break;
						case EWeaponCategory::Heavy:
							EquipSpeedMultiplier = 0.8f; // 가장 많이 감소
							break;
						default:
							EquipSpeedMultiplier = 1.0f; // 기본값
							break;
						}
					}
				}
			}
		}
	}
}

// 데미지 방향이 제대로 입력되었다면, 무기 데미지인 것으로 판정
bool ABaseAgent::IsGunDamage() const
{
	return LastDamagedOrg == FVector::ZeroVector ? false : true;
}

// 네트워크 복제 설정
void ABaseAgent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseAgent, m_AgentID);
	DOREPLIFETIME(ABaseAgent, VisibilityStateArray);
	DOREPLIFETIME(ABaseAgent, bIsRun);
	DOREPLIFETIME(ABaseAgent, bIsDead);
	DOREPLIFETIME(ABaseAgent, MeleeKnife);
	DOREPLIFETIME(ABaseAgent, MainWeapon);
	DOREPLIFETIME(ABaseAgent, SubWeapon);
	DOREPLIFETIME(ABaseAgent, Spike);
	DOREPLIFETIME(ABaseAgent, CurrentInteractor);
	DOREPLIFETIME(ABaseAgent, CurrentEquipmentState);
	DOREPLIFETIME(ABaseAgent, PrevEquipmentState);
	DOREPLIFETIME(ABaseAgent, PoseIdx);
	DOREPLIFETIME(ABaseAgent, IsInPlantZone);
	DOREPLIFETIME(ABaseAgent, ReplicatedControlRotation);
	DOREPLIFETIME(ABaseAgent, bIsInRound);
}


// 크레딧 보상 함수 구현
void ABaseAgent::AddCredits(int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}

	AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>();
	if (PS)
	{
		UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
		if (CreditComp)
		{
			CreditComp->AddCredits(Amount);
			NET_LOG(LogTemp, Warning, TEXT("%s에게 %d 크레딧 지급. 현재 총 크레딧: %d"),
			        *GetName(), Amount, CreditComp->GetCurrentCredit());
		}
	}
}

void ABaseAgent::RewardKill()
{
	if (HasAuthority())
	{
		AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>();
		if (PS)
		{
			UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
			if (CreditComp)
			{
				CreditComp->AwardKillCredits();
			}
		}
	}
}

void ABaseAgent::RewardSpikeInstall()
{
	if (HasAuthority())
	{
		AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>();
		if (PS)
		{
			UCreditComponent* CreditComp = PS->FindComponentByClass<UCreditComponent>();
			if (CreditComp)
			{
				CreditComp->AwardSpikeActionCredits(true);
				NET_LOG(LogTemp, Warning, TEXT("%s가 스파이크를 설치하여 보상을 받았습니다."),
				        *GetPlayerState<APlayerState>()->GetPlayerName());
			}
		}
	}
}

void ABaseAgent::OnEquip()
{
	if (ABP_1P)
	{
		ABP_1P->UpdateState();
	}
	if (ABP_3P)
	{
		ABP_3P->UpdateState();
	}
	OnAgentEquip.Broadcast();
}

void ABaseAgent::OnFire()
{
	OnAgentFire.Broadcast();
}

void ABaseAgent::OnReload()
{
	OnAgentReload.Broadcast();
}

void ABaseAgent::OnSpikeStartPlant()
{
	// NET_LOG(LogTemp,Warning,TEXT("baseAgent:: OnSpikeStartPlant"));

	bCanMove = false;
	OnSpikeActive.Broadcast();

	if (PC != nullptr)
	{
		auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		if (hud)
		{
			hud->SetSpikeProgressTextToPlant();
			hud->DisplaySpikeProgress();
		}
	}
}

void ABaseAgent::OnSpikeCancelInteract()
{
	// NET_LOG(LogTemp,Warning,TEXT("baseAgent:: OnSpikeCancelPlant"));
	DefusalMesh->SetVisibility(false);
	bCanMove = true;
	OnSpikeCancel.Broadcast();

	if (MainWeapon)
	{
		EquipInteractor(MainWeapon);
	}
	else if (SubWeapon)
	{
		EquipInteractor(SubWeapon);
	}
	else
	{
		EquipInteractor(MeleeKnife);
	}

	if (PC != nullptr)
	{
		auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		if (hud)
		{
			hud->HideSpikeProgress();
		}
	}
}

void ABaseAgent::OnSpikeProgressBarUpdate(const float ratio)
{
	//TODO: hud 멤버 변수로 변경
	if (PC != nullptr)
	{
		auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		if (hud)
		{
			hud->OnSpikeProgressBarUpdate(ratio);
		}
	}
}

void ABaseAgent::Multicast_OnSpikeOwnChanged_Implementation(bool bOwnSpike)
{
	OnSpikeOwnChanged.Broadcast(bOwnSpike);
}

void ABaseAgent::OnSpikeFinishPlant()
{
	bCanMove = true;
	OnSpikeOwnChanged.Broadcast(false);
	
	if (CurrentInteractor == Spike)
	{
		CurrentInteractor = nullptr;
	}

	Spike = nullptr;
	
	if (MainWeapon)
	{
		EquipInteractor(MainWeapon);
	}
	else if (SubWeapon)
	{
		EquipInteractor(SubWeapon);
	}
	else
	{
		EquipInteractor(MeleeKnife);
	}

	if (PC != nullptr)
	{
		auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		if (hud)
		{
			hud->HideSpikeProgress();
		}
	}
}

void ABaseAgent::OnSpikeStartDefuse()
{
	DefusalMesh->SetVisibility(true);
	
	bCanMove = false;
	OnSpikeDeactive.Broadcast();

	if (PC != nullptr)
	{
		auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		if (hud)
		{
			hud->SetSpikeProgressTextToDefuse();
			hud->DisplaySpikeProgress();
		}
	}
}

void ABaseAgent::OnSpikeFinishDefuse()
{
	DefusalMesh->SetVisibility(false);
	
	bCanMove = true;
	OnSpikeDefuseFinish.Broadcast();

	if (MainWeapon)
	{
		EquipInteractor(MainWeapon);
	}
	else if (SubWeapon)
	{
		EquipInteractor(SubWeapon);
	}
	else
	{
		EquipInteractor(MeleeKnife);
	}

	if (PC != nullptr)
	{
		auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		if (hud)
		{
			hud->HideSpikeProgress();
		}
	}
}

void ABaseAgent::OnRep_Controller()
{
	Super::OnRep_Controller();
	
	// 클라이언트 입장에서 Possess가 되었는지 알 수 있는 곳
	// InteractionCapsule AddDynamic을 여기서 처리
	if (false == bInteractionCapsuleInit && nullptr != Controller && false == HasAuthority() && IsLocallyControlled())
	{
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, InteractionCapsule AddDynamic"), __FUNCTION__);
		bInteractionCapsuleInit = true;
		InteractionCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABaseAgent::OnFindInteraction);
		InteractionCapsule->OnComponentEndOverlap.AddDynamic(this, &ABaseAgent::OnInteractionCapsuleEndOverlap);
	}
}

bool ABaseAgent::IsBlueTeam() const
{
	if (const AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>())
	{
		return PS->bIsBlueTeam;
	}
	NET_LOG(LogTemp, Error, TEXT("%hs Called, PS is nullptr"), __FUNCTION__);
	return false;
}

bool ABaseAgent::IsAttacker() const
{
	if (const AAgentPlayerState* PS = GetPlayerState<AAgentPlayerState>())
	{
		return PS->bIsAttacker;
	}
	NET_LOG(LogTemp, Error, TEXT("%hs Called, PS is nullptr"), __FUNCTION__);
	return false;
}

bool ABaseAgent::IsInFrustum(const AActor* Actor) const
{
	// Ref: https://forums.unrealengine.com/t/perform-frustum-check/287524/10
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer != nullptr && LocalPlayer->ViewportClient != nullptr && LocalPlayer->ViewportClient->Viewport)
	{
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			LocalPlayer->ViewportClient->Viewport,
			GetWorld()->Scene,
			LocalPlayer->ViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(true));

		FVector ViewLocation;
		FRotator ViewRotation;
		FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, LocalPlayer->ViewportClient->Viewport);
		if (SceneView != nullptr)
		{
			bool bIsInFrustum = SceneView->ViewFrustum.IntersectSphere(Actor->GetActorLocation(), Actor->GetSimpleCollisionRadius());
			// 절두체 안에 있으면서 최근에 렌더링 된 적 있는 경우에만 true 반환
			return bIsInFrustum && Actor->WasRecentlyRendered(0.01f);
		}
	}
	return false;
}

EInteractorType ABaseAgent::GetPrevEquipmentType() const
{
	return PrevEquipmentState;
}

void ABaseAgent::Client_PlayFirstPersonMontage_Implementation(UAnimMontage* MontageToPlay, float PlayRate,
	FName StartSectionName)
{
	if (ABP_1P && MontageToPlay)
	{
		ABP_1P->Montage_Play(MontageToPlay, PlayRate);
		if (StartSectionName != NAME_None)
		{
			ABP_1P->Montage_JumpToSection(StartSectionName, MontageToPlay);
		}
	}
}

void ABaseAgent::NetMulti_PlayThirdPersonMontage_Implementation(UAnimMontage* MontageToPlay, float PlayRate,
	FName StartSectionName)
{
	if (ABP_3P && MontageToPlay)
	{
		ABP_3P->Montage_Play(MontageToPlay, PlayRate);
		if (StartSectionName != NAME_None)
		{
			ABP_3P->Montage_JumpToSection(StartSectionName, MontageToPlay);
		}
	}
}

void ABaseAgent::StopFirstPersonMontage(float BlendOutTime)
{
	if (ABP_1P)
	{
		ABP_1P->Montage_Stop(BlendOutTime);
	}
}

void ABaseAgent::StopThirdPersonMontage(float BlendOutTime)
{
	if (ABP_3P)
	{
		ABP_3P->Montage_Stop(BlendOutTime);
	}
}

float ABaseAgent::GetCurrentHealth() const
{
	if (ASC.IsValid())
	{
		const UBaseAttributeSet* Attributes = Cast<UBaseAttributeSet>(ASC->GetSet<UBaseAttributeSet>());
		if (Attributes)
		{
			return Attributes->GetHealth();
		}
	}
	return 0.f;
}

float ABaseAgent::GetMaxHealth() const
{
	if (ASC.IsValid())
	{
		const UBaseAttributeSet* Attributes = Cast<UBaseAttributeSet>(ASC->GetSet<UBaseAttributeSet>());
		if (Attributes)
		{
			return Attributes->GetMaxHealth();
		}
	}
	return 0.f;
}

bool ABaseAgent::IsFullHealth() const
{
	return FMath::IsNearlyEqual(GetCurrentHealth(), GetMaxHealth());
}

void ABaseAgent::CancelActiveAbilities()
{
	if (!ASC.IsValid())
	{
		return;
	}
    
	NET_LOG(LogTemp, Display, TEXT("모든 활성 어빌리티 취소"));
    
	// ASC의 통합 정리 함수 호출
	ASC->ForceCleanupAllAbilities();
}

bool ABaseAgent::HasGameplayTag(const FGameplayTag& TagToCheck) const
{
	if (ASC.IsValid())
	{
		return ASC->HasMatchingGameplayTag(TagToCheck);
	}
	return false;
}

void ABaseAgent::OnAbilityPrepare(FGameplayTag slotTag, EFollowUpInputType inputType)
{
	auto* pc = Cast<AMatchPlayerController>(GetController());
	if (pc)
	{
		if (auto* MatchMapHud = Cast<UMatchMapHUD>(pc->GetMatchMapHud()))
		{
			MatchMapHud->DisplayFollowUpInputUI(slotTag, inputType);
		}
	}
	// NET_LOG(LogTemp,Display,TEXT("어빌리티 준비"));
}

void ABaseAgent::OnEndAbility(EFollowUpInputType inputType)
{
	auto* pc = Cast<AMatchPlayerController>(GetController());
	if (pc)
	{
		if (auto* MatchMapHud = Cast<UMatchMapHUD>(pc->GetMatchMapHud()))
		{
			MatchMapHud->HideFollowUpInputUI();
		}
	}
}

void ABaseAgent::OnCancelAbility(EFollowUpInputType inputType)
{
}

void ABaseAgent::OnAbilityFollowupInput()
{
	auto* pc = Cast<AMatchPlayerController>(GetController());
	if (pc)
	{
		if (auto* MatchMapHud = Cast<UMatchMapHUD>(pc->GetMatchMapHud()))
		{
			MatchMapHud->HideFollowUpInputUI();
		}
	}
}

void ABaseAgent::OnFlashIntensityChanged(float NewIntensity, FVector FlashSourceLocation)
{
	// 로컬 플레이어에게만 시각 효과 적용
	if (!IsLocallyControlled())
		return;

	if (FlashWidget)
	{
		// 섬광 시작 시 타입 설정
		if (NewIntensity > 0.01f && FlashComponent)
		{
			EFlashType FlashType = FlashComponent->GetFlashType();
			FlashWidget->StartFlashEffect(0.0f, FlashType);
		}
		
		// 섬광 위치 정보와 함께 업데이트
		FlashWidget->UpdateFlashIntensity(NewIntensity, FlashSourceLocation);
		
		// 섬광 종료 시 위젯 정리
		if (NewIntensity <= 0.01f)
		{
			FlashWidget->StopFlashEffect();
		}
	}
    
	if (PostProcessComponent)
	{
		PostProcessComponent->UpdateFlashPostProcess(NewIntensity);
	}
    
	// 디버그 로그 (완전 실명/회복 상태 표시)
	if (UFlashComponent* FlashComp = FindComponentByClass<UFlashComponent>())
	{
		EFlashState State = FlashComp->GetFlashState();
		FString StateStr = (State == EFlashState::CompleteBlind) ? TEXT("완전실명") :
						  (State == EFlashState::Recovery) ? TEXT("회복중") : TEXT("정상");
		
		EFlashType FlashType = FlashComp->GetFlashType();
		FString FlashTypeStr;
		switch(FlashType)
		{
		case EFlashType::Phoenix: FlashTypeStr = TEXT("Phoenix"); break;
		case EFlashType::KayO: FlashTypeStr = TEXT("Kay/O"); break;
		default: FlashTypeStr = TEXT("Default"); break;
		}
        
		UE_LOG(LogTemp, VeryVerbose, TEXT("섬광 강도: %.2f, 상태: %s, 타입: %s, 위치: %s"), 
			NewIntensity, *StateStr, *FlashTypeStr, *FlashSourceLocation.ToString());
	}
}



void ABaseAgent::CreateFlashWidget()
{
	if (FlashWidgetClass && !FlashWidget)
	{
		FlashWidget = CreateWidget<UFlashWidget>(GetWorld(), FlashWidgetClass);
		if (FlashWidget)
		{
			FlashWidget->AddToViewport(100); // 높은 Z-Order로 최상단에 표시
			//FlashWidget->SetVisibility(ESlateVisibility::Collapsed); // 처음에는 숨김
		}
	}
}

#pragma region "Minimap"
void ABaseAgent::OnRep_VisibilityStateArray()
{
	// 클라이언트에서 배열이 업데이트될 때 호출됨 
	// UI 업데이트나 시각 효과 처리를 여기서 할 수 있음 
}

void ABaseAgent::InitMinimap()
{
	// 서버에서만 시야 체크 처리 -현재 액터가 서버에서 실행 중인지 확인 (권한 있음)
	if (HasAuthority())
	{
		// 자동 시야 체크 설정 (시야 체크 함수 호출하여 초기 상태 설정)
		PerformVisibilityChecks();
	}
	
    // 에이전트 ID에 따라 다른 아이콘 설정
    if (MinimapIcon == nullptr && GetWorld())
    {
        // 게임 인스턴스에서 에이전트 데이터 가져오기
        if (UValorantGameInstance* GameInstance = Cast<UValorantGameInstance>(GetGameInstance()))
        {
            // 에이전트 ID로 에이전트 데이터 얻기
            FAgentData* AgentData = GameInstance->GetAgentData(m_AgentID);
            if (AgentData)
            {
                // 데이터에서 아이콘 정보 가져오기
                FString IconPath = AgentData->AgentName;
                if (!IconPath.IsEmpty())
                {
                    // 경로에서 아이콘 로드
                    MinimapIcon = LoadObject<UTexture2D>(nullptr, *IconPath);
                }
                
                // 물음표 아이콘 설정 (모든 에이전트 공통 또는 각자 다른 물음표)
                FString QuestionPath = AgentData->AgentName;
                if (!QuestionPath.IsEmpty())
                {
                    QuestionMarkIcon = LoadObject<UTexture2D>(nullptr, *QuestionPath);
                }
            }
        }
        
        // 여전히 아이콘이 없다면 기본 아이콘 설정
        if (MinimapIcon == nullptr)
        {
            // 기본 아이콘 로드
            MinimapIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Resource/MapObject/Images"));
        }
        
        if (QuestionMarkIcon == nullptr)
        {
            // 기본 물음표 아이콘 로드
            QuestionMarkIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Resource/MapObject/Images"));
        }
    }
    
    // 자신을 로컬 플레이어의 미니맵에 등록하기
    // 약간의 지연을 두고 실행하여 모든 컨트롤러가 초기화될 시간을 확보
    FTimerHandle RegisterTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(RegisterTimerHandle, [this]()
    {
        // 모든 플레이어 컨트롤러 탐색
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            AAgentPlayerController* PC = Cast<AAgentPlayerController>(It->Get());
        	if (IsValid(PC) && PC->IsLocalController() && PC->GetMinimapWidget())
            {
        		PC->GetMinimapWidget()->AddPlayerToMinimap(GetPlayerState<AAgentPlayerState>());
                UE_LOG(LogTemp, Warning, TEXT("에이전트(%s)가 자신을 미니맵에 등록함"), *GetName());
            }
        }
    }, 1.0f, false);
}

void ABaseAgent::SetMinimapIcon(UTexture2D* NewIcon)
{
	MinimapIcon = NewIcon;
}

// 가시성 상태 조회 함수 - TMap 대신 TArray에서 검색하도록 수정
EVisibilityState ABaseAgent::GetVisibilityStateForAgent(const ABaseAgent* Observer) const
{
	// 자기 자신은 항상 보임
	if (Observer == this)
		// 항상 보이는 상태 반환
			return EVisibilityState::Visible;

	// TArray에서 Observer에 해당하는 정보 찾기
	for (const FAgentVisibilityInfo& Info : VisibilityStateArray)
	{
		if (Info.Observer == Observer)
			return Info.VisibilityState;
	}
	// 기본값은 숨김 상태
	return EVisibilityState::Hidden;
}

// 서버 함수 유효성 검사
bool ABaseAgent::Server_UpdateVisibilityState_Validate(ABaseAgent* Observer, EVisibilityState NewState)
{
	// 항상 유효함 (필요시 추가 검증 로직 구현 가능)
	return true;
}

// 서버 함수 - 중앙화된 헬퍼 함수를 사용하도록 수정
void ABaseAgent::Server_UpdateVisibilityState_Implementation(ABaseAgent* Observer, EVisibilityState NewState)
{
	// 헬퍼 함수 사용 - 코드 중복 방지
	UpdateVisibilityState(Observer, NewState);
}

// 멀티캐스트 함수 - 모든 클라이언트에 상태 변경 알림
void ABaseAgent::Multicast_OnVisibilityStateChanged_Implementation(ABaseAgent* Observer, EVisibilityState NewState)
{
	// 클라이언트에서 상태 업데이트 (UI 갱신용)
	// OnRep 함수를 통해 대부분 처리 가능하지만, 추가 처리가 필요한 경우를 위해 유지
    
	// TArray 방식으로 업데이트 - 기존 정보가 있으면 업데이트, 없으면 추가
	FAgentVisibilityInfo Info;
	int32 Index;
	bool bFound = FindVisibilityInfo(Observer, Info, Index);
    
	if (bFound)
	{
		// 기존 항목 업데이트
		VisibilityStateArray[Index].VisibilityState = NewState;
	}
	else
	{
		// 새 항목 추가
		FAgentVisibilityInfo NewInfo;
		NewInfo.Observer = Observer;
		NewInfo.VisibilityState = NewState;
		VisibilityStateArray.Add(NewInfo);
	}
    
	//************* 이 함수에서 변경한 값으로 인해 OnRep_VisibilityStateArray가 클라이언트에서 호출됨
}

// 시야 체크 수행 함수
void ABaseAgent::PerformVisibilityChecks()
{
	// 서버가 아닌 경우
	if (!HasAuthority())
		// 함수 종료 (서버에서만 실행)
		return;

	// 게임의 모든 에이전트 가져오기
	// 모든 에이전트 배열
	TArray<AActor*> AllAgents;
	// BaseAgent 클래스의 모든 인스턴스 가져오기
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseAgent::StaticClass(), AllAgents);

	// 각 에이전트에 대해 시야 체크 수행
	// 모든 에이전트에 대해 반복
	for (AActor* ActorAgent : AllAgents)
	{
		// Actor를 BaseAgent로 형변환
		ABaseAgent* Observer = Cast<ABaseAgent>(ActorAgent);
		// 관찰자가 유효하지 않거나 자기 자신인 경우
		if (!IsValid(Observer) || Observer == this)
			// 다음 에이전트로 넘어감
			continue;

		// 라인 트레이스로 시야 체크
		// 충돌 결과 저장 구조체
		FHitResult HitResult;
		// 충돌 쿼리 매개변수
		FCollisionQueryParams QueryParams;
		// 관찰자 자신은 충돌 검사에서 제외
		QueryParams.AddIgnoredActor(Observer);

		// 관찰자의 눈 위치
		FVector ObserverEyeLocation;
		// 관찰자의 눈 회전
		FRotator ObserverEyeRotation;
		// 관찰자의 시점 정보 가져오기
		Observer->GetActorEyesViewPoint(ObserverEyeLocation, ObserverEyeRotation);

		// 대상(this)의 위치
		FVector TargetLocation = GetActorLocation();
		// 관찰자로부터 대상까지의 방향 벡터 (정규화)
		FVector DirectionToTarget = (TargetLocation - ObserverEyeLocation).GetSafeNormal();

		// 시야 각도 체크 (제한이 없으니 생략하지만, 필요하면 여기 추가)

		// 시야 라인 트레이스
		bool bHasLineOfSight = !GetWorld()->LineTraceSingleByChannel( // 라인 트레이스 수행하여 시야 확인
			HitResult, // 충돌 결과
			ObserverEyeLocation, // 시작점 (관찰자 눈)
			TargetLocation, // 끝점 (타겟 위치)
			ECC_Visibility, // 가시성 충돌 채널 사용
			QueryParams // 쿼리 매개변수
		); // 충돌이 없으면(true 반환) 시야 있음, 충돌이 있으면(false 반환) 시야 없음


		// 이전 상태 찾기 - TMap 대신 헬퍼 함수 사용
		FAgentVisibilityInfo Info;
		int32 Index;
		bool bFound = FindVisibilityInfo(Observer, Info, Index);
		EVisibilityState OldState = bFound ? Info.VisibilityState : EVisibilityState::Hidden;

		
		// 새 상태 결정
		// 새로운 가시성 상태
		EVisibilityState NewState;
		// 시야가 확보된 경우
		if (bHasLineOfSight)
		{
			// 보이는 상태로 설정
			NewState = EVisibilityState::Visible;
		}
		// 시야가 차단된 경우
		else
		{
			// 이전에 보였다면 질문표로, 아니면 숨김 유지
			// 이전에 보이던 상태였다면
			if (OldState == EVisibilityState::Visible)
			{
				// 물음표 상태로 설정
				NewState = EVisibilityState::QuestionMark;
			}
			// 이전에 보이지 않던 상태였다면
			else
			{
				// 상태 유지 (변경 없음)
				NewState = OldState;
			}
		}

		// 상태가 변경된 경우만 업데이트
		// 새 상태가 이전 상태와 다른 경우에만
		if (NewState != OldState)
		{
			// 서버 함수 호출하여 상태 업데이트
			Server_UpdateVisibilityState(Observer, NewState);
		}
	}
}

// 상태 업데이트 헬퍼 함수 - 상태 업데이트 로직을 중앙화하여 코드 중복 방지
void ABaseAgent::UpdateVisibilityState(ABaseAgent* Observer, EVisibilityState NewState)
{
	if (!IsValid(Observer))
		return;
        
	// 기존 정보 찾기
	FAgentVisibilityInfo Info;
	int32 Index = -1;
	bool bFound = FindVisibilityInfo(Observer, Info, Index);
    
	// 상태가 변경된 경우에만 처리 - 불필요한 업데이트 방지
	if (!bFound || Info.VisibilityState != NewState)
	{
		// 이전 상태 저장 - 상태 전환 로직을 위해 필요
		EVisibilityState OldState = bFound ? Info.VisibilityState : EVisibilityState::Hidden;
        
		// 배열 업데이트
		if (bFound)
		{
			// 기존 항목 업데이트
			VisibilityStateArray[Index].VisibilityState = NewState;
            
			// 질문표 타이머 설정 (Visible -> QuestionMark 전환 시)
			if (OldState == EVisibilityState::Visible && NewState == EVisibilityState::QuestionMark)
			{
				VisibilityStateArray[Index].QuestionMarkTimer = QuestionMarkDuration;
			}
		}
		else
		{
			// 새 정보 생성 및 추가
			FAgentVisibilityInfo NewInfo;
			NewInfo.Observer = Observer;
			NewInfo.VisibilityState = NewState;
            
			// 질문표 상태인 경우 타이머 설정
			if (NewState == EVisibilityState::QuestionMark)
			{
				NewInfo.QuestionMarkTimer = QuestionMarkDuration;
			}
            
			VisibilityStateArray.Add(NewInfo);  // 배열에 추가
		}
        
		// 모든 클라이언트에 상태 변경 알림
		Multicast_OnVisibilityStateChanged(Observer, NewState);
	}
}

// 헬퍼 함수 - TArray에서 특정 관찰자의 가시성 정보를 찾는 유틸리티 함수
bool ABaseAgent::FindVisibilityInfo(ABaseAgent* Observer, FAgentVisibilityInfo& OutInfo, int32& OutIndex)
{
	OutIndex = -1;  // 기본값 -1 (찾지 못한 경우)
    
	// 배열 전체를 순회하며 일치하는 관찰자 찾기
	for (int32 i = 0; i < VisibilityStateArray.Num(); i++)
	{
		if (VisibilityStateArray[i].Observer == Observer)
		{
			// 정보 반환
			OutInfo = VisibilityStateArray[i];
			// 인덱스 반환
			OutIndex = i;
			// 찾았음
			return true;
		}
	}
    
	// 찾지 못함
	return false;
}

void ABaseAgent::CheckMinimapVisibility(const float DeltaTime)
{
	// 주기적으로 시야 체크
	// 마지막 체크 이후 지정된 간격이 지났는지 확인
	if (GetWorld()->GetTimeSeconds() - LastVisibilityCheckTime > VisibilityCheckInterval)
	{
		// 시야 체크 함수 호출
		PerformVisibilityChecks();
		// 마지막 체크 시간 업데이트
		LastVisibilityCheckTime = GetWorld()->GetTimeSeconds();
	}

	// 물음표 타이머 관리 - TMap 대신 TArray 사용 방식으로 수정
	// 업데이트가 필요한 항목의 인덱스 저장
	TArray<int32> IndicesNeedingUpdate;
        
	// 모든 가시성 정보 순회하며 타이머 업데이트
	for (int32 i = 0; i < VisibilityStateArray.Num(); i++)
	{
		FAgentVisibilityInfo& Info = VisibilityStateArray[i];
            
		// 물음표 상태이고 타이머가 동작 중인 경우에만 처리
		if (Info.VisibilityState == EVisibilityState::QuestionMark && Info.QuestionMarkTimer > 0)
		{
			// 타이머 감소
			Info.QuestionMarkTimer -= DeltaTime;
			if (Info.QuestionMarkTimer <= 0)
			{
				// 타이머 만료 시 Hidden으로 변경할 항목 표시
				IndicesNeedingUpdate.Add(i);
			}
		}
	}
        
	// 타이머가 끝난 항목들을 Hidden으로 변경
	// 배열의 끝에서부터 처리하여 인덱스 변경을 방지
	for (int32 i = IndicesNeedingUpdate.Num() - 1; i >= 0; i--)
	{
		int32 Index = IndicesNeedingUpdate[i];
		ABaseAgent* Observer = VisibilityStateArray[Index].Observer;
		// 상태 업데이트
		UpdateVisibilityState(Observer, EVisibilityState::Hidden);
	}
}

void ABaseAgent::DevCameraMode_Implementation(bool bIsActive)
{
	// if (!bIsActive)
	// {
	// 	if (PC && PC->GetMinimapWidget())
	// 	{
	// 		PC->GetMinimapWidget()->SetVisibility(ESlateVisibility::Hidden);
	// 	}
	// 	if (PC && PC->GetMatchMapHud())
	// 	{
	// 		PC->GetMatchMapHud()->SetVisibility(ESlateVisibility::Hidden);
	// 	}
	// }
	// else
	// {
	// 	if (PC && PC->GetMinimapWidget())
	// 	{
	// 		PC->GetMinimapWidget()->SetVisibility(ESlateVisibility::Visible);
	// 	}
	// 	if (PC && PC->GetMatchMapHud())
	// 	{
	// 		PC->GetMatchMapHud()->SetVisibility(ESlateVisibility::Visible);
	// 	}
	// }
}

#pragma endregion "Minimap"

void ABaseAgent::ServerApplyHealthGE_Implementation(TSubclassOf<UGameplayEffect> geClass, float Value, ABaseAgent* DamageInstigator)
{
    if (!geClass) return;

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(geClass, 1.f, Context);

    if (DamageInstigator)
    {
        SetInstigator(DamageInstigator);
        LastDamagedOrg = DamageInstigator->GetActorLocation();
    }

    if (SpecHandle.IsValid())
    {
        // Data.Health 태그로 고정
        SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Health")), Value);
        ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}

void ABaseAgent::AdjustFlashEffectDirect_Implementation(float BlindDuration, float RecoveryDuration)
{
	FlashComponent->FlashEffect(BlindDuration, RecoveryDuration);
}

void ABaseAgent::Multicast_PlayNiagaraEffectAttached_Implementation(AActor* AttachTarget, UNiagaraSystem* NiagaraEffect, float Duration)
{
	if (!AttachTarget || !NiagaraEffect)
	{	
		return;
	}

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraEffect,
		AttachTarget->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		true, // Auto Destroy
		ENCPoolMethod::AutoRelease,
		true // Auto Activate
	);

	if (NiagaraComp && Duration > 0.f)
	{
		FTimerHandle TimerHandle;
		// 타이머 람다에서 NiagaraComp를 안전하게 파괴
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [NiagaraComp]() {
			if (IsValid(NiagaraComp))
			{
				NiagaraComp->DestroyComponent();
			}
		}, Duration, false);
	}
}

void ABaseAgent::Multicast_PlayNiagaraEffectAtLocation_Implementation(FVector Location, UNiagaraSystem* NiagaraEffect,
	float Duration)
{
	if (!NiagaraEffect)
	{	
		return;
	}

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraEffect,
		Location
	);

	if (NiagaraComp && Duration > 0.f)
	{
		FTimerHandle TimerHandle;
		// 타이머 람다에서 NiagaraComp를 안전하게 파괴
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [NiagaraComp]() {
			if (IsValid(NiagaraComp))
			{
				NiagaraComp->DestroyComponent();
			}
		}, Duration, false);
	}
}

void ABaseAgent::Multicast_PlaySoundAtLocation_Implementation(FVector Location, USoundBase* SoundEffect)
{
	if (!SoundEffect)
	{	
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundEffect, Location);
}

void ABaseAgent::Multicast_PlaySound_Implementation(USoundBase* SoundEffect)
{
	if (!SoundEffect)
	{	
		return;
	}

	UGameplayStatics::PlaySound2D(GetWorld(), SoundEffect);
}

void ABaseAgent::StartLogging()
{
	bIsInRound = true;
}

void ABaseAgent::StopLogging()
{
	bIsInRound = false;
	if (HasAuthority())
	{
		m_GameMode->SubmitShotLog(PC,CachedFireCount,CachedHitCount,CachedHeadshotCount,CachedDamage);
		InitLog();
	}
	else
	{
		ServerRPC_SubmitLog();
	}
}

void ABaseAgent::InitLog()
{
	CachedFireCount = 0;
	CachedHitCount = 0;
	CachedHeadshotCount = 0;
	CachedDamage = 0;
}

void ABaseAgent::LogShotResult(const bool bHit)
{
	if (!bIsInRound)
	{
		return;
	}

	CachedFireCount++;
	
	if (bHit)
	{
		CachedHitCount++;
	}
}

void ABaseAgent::LogHeadshot()
{
	if (!bIsInRound)
	{
		return;
	}

	CachedHeadshotCount++;
}

void ABaseAgent::LogFinalDamage(const int damage)
{
	if (!bIsInRound)
	{
		return;
	}
	
	CachedDamage += damage;
}

void ABaseAgent::ServerRPC_SubmitLog_Implementation()
{
	m_GameMode->SubmitShotLog(PC,CachedFireCount,CachedHitCount,CachedHeadshotCount,CachedDamage);
	InitLog();
}

void ABaseAgent::LoadWavFileBinary(const FString& FilePath, TArray<uint8>& BinaryData)
{
	// WAV 파일을 로드
	if (!FFileHelper::LoadFileToArray(BinaryData, *FilePath))
	{
		// 로드 실패 시 로그 출력
		UE_LOG(LogTemp, Error, TEXT("Failed to load WAV file from path: %s"), *FilePath);
		return;
	}

	// 로드 성공 시 로그 출력
	UE_LOG(LogTemp, Log, TEXT("Successfully loaded WAV file: %s"), *FilePath);
}

void ABaseAgent::SendWavFileAsFormData(const TArray<uint8>& BinaryData)
{
	// TODO: 서버주소 및 엔드포인트
	const FString ServerBaseURL  = TEXT("http://192.168.20.142:8080");
	const FString PostEndpoint   = TEXT("/botresponse");
	
	// boundary 설정
	FString Boundary = "---------------------------boundary";
	FString FileName = TEXT("UserVoiceInput.wav"); // WAV 파일 이름

	// TODO: key 이름 설정
	FString Key = TEXT("file");

	// 요청 본문 생성
	FString Body;
	Body += TEXT("--") + Boundary + TEXT("\r\n");
	Body += TEXT("Content-Disposition: form-data; name=\"") + Key + TEXT("\"; filename=\"") + FileName + TEXT("\"\r\n");
	Body += TEXT("Content-Type: audio/wav\r\n\r\n");

	// WAV 파일 바이너리 데이터 추가
	TArray<uint8> FullData;
	FullData.Append(FStringToUint8(Body));
	FullData.Append(BinaryData);

	// boundary 끝 표시
	const FString EndBoundary = TEXT("\r\n--") + Boundary + TEXT("--\r\n");
	FullData.Append(FStringToUint8(EndBoundary));

	// 요청 생성
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ServerBaseURL + PostEndpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("multipart/form-data; boundary=") + Boundary);
	Request->SetContent(FullData);

	// 요청 완료 콜백 설정
	Request->OnProcessRequestComplete().BindLambda(
		[this, ServerBaseURL](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bWasSuccessful)
		{
			if (!bWasSuccessful || !Res.IsValid())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[Voice] POST 요청 실패 or 응답 무효. ResponseCode=%d"),
					Res.IsValid() ? Res->GetResponseCode() : -1
				);
				return;
			}

			// (6) POST 응답 JSON 문자열 읽기
			FString JsonString = Res->GetContentAsString();
			UE_LOG(LogTemp, Log, TEXT("[Voice] POST 응답 JSON: %s"), *JsonString);

			// (7) JSON 파싱: "audio_file" 필드 추출
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

			if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("[Voice] JSON 파싱 실패"));
				return;
			}
			
			// "answer" 필드만 꺼내기
		    if (JsonObject->HasField(TEXT("answer")))
		    {
		    	const FString AnswerText = JsonObject->GetStringField(TEXT("answer"));
		    	
		    	if (PC && PC->GetMatchMapHud())
		    	{
					auto* hud = Cast<UMatchMapHUD>(PC->GetMatchMapHud());
		    		hud->UpdateAIAnswer(AnswerText);
				}
		    }
		    else
		    {
		    	UE_LOG(LogTemp, Error, TEXT("[Voice] JSON에 'answer' 필드가 없습니다"));
		    }

			// JSON 안에 audio_file 필드가 존재하는지 확인
			if (!JsonObject->HasField(TEXT("audio_file")))
			{
				UE_LOG(LogTemp, Error, TEXT("[Voice] JSON에 'audio_file' 필드가 없습니다"));
				return;
			}

			const FString AudioFileName = JsonObject->GetStringField(TEXT("audio_file"));
			UE_LOG(LogTemp, Log, TEXT("[Voice] 다운로드할 오디오 파일명: %s"), *AudioFileName);

			// TODO: 엔드포인트 다시 확인
			// DownloadWavFile(ServerBaseURL + TEXT("/get_audio/answerspeech%5C") + AudioFileName);
			DownloadWavFile("http://192.168.20.142:8080/get_audio/answerspeech%5Canswer_speech.wav");
		}
	);

	// 요청 실행
	Request->ProcessRequest();
}

void ABaseAgent::SendWavFileDirectly()
{
	// Wav 파일 전송 로직
	UE_LOG(LogTemp, Log, TEXT("Wav 파일 전송 시작!"));

	FString BaseSavedDir = FPaths::ProjectSavedDir();

	FString SubFolder = TEXT("UserVoiceInput");
	FString FileName = TEXT("UserVoiceInput.wav");
			
	const FString SaveDir   = FPaths::Combine(FPaths::ProjectSavedDir(), SubFolder);

	IFileManager::Get().MakeDirectory(*SaveDir, true);
			
	const FString SavePath = FPaths::Combine(SaveDir,FileName);
	
	TArray<uint8> BinaryData;
	LoadWavFileBinary(SavePath, BinaryData);

	if (BinaryData.Num() > 0)
	{
		SendWavFileAsFormData(BinaryData);
	}
}

void ABaseAgent::DownloadWavFile(const FString& AudioFileURL)
{
	// (10) GET 요청 생성
	FHttpRequestRef GetRequest = FHttpModule::Get().CreateRequest();
	GetRequest->SetURL(AudioFileURL);
	GetRequest->SetVerb(TEXT("GET"));
	// WAV 파일을 받을 땐 따로 Content-Type 지정할 필요 없음. 서버가 application/octet-stream 등으로 보내 줄 것.

	// (11) GET 응답 콜백 바인딩
	GetRequest->OnProcessRequestComplete().BindLambda(
		[this, AudioFileURL](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bWasSuccessful)
		{
			if (!bWasSuccessful || !Res.IsValid())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[Voice] WAV GET 요청 실패 or 응답 무효. URL=%s, ResponseCode=%d"),
					*AudioFileURL, Res.IsValid() ? Res->GetResponseCode() : -1
				);
				return;
			}

			// (12) 응답 바디로부터 WAV 바이너리 받아서 파일로 저장
			const TArray<uint8>& WavData = Res->GetContent();
			if (WavData.Num() == 0)
			{
				UE_LOG(LogTemp, Error, TEXT("[Voice] WAV 데이터가 비어 있습니다."));
				return;
			}

			// 답변 파일 경로 및 파일명
			FString SubFolder = TEXT("AiVoiceOutput");
			FString FileName = TEXT("answer_speech.wav");
			
			const FString SaveDir   = FPaths::Combine(FPaths::ProjectSavedDir(), SubFolder);

			IFileManager::Get().MakeDirectory(*SaveDir, true);
			
			const FString SavePath = FPaths::Combine(SaveDir,FileName);
			
			if (FFileHelper::SaveArrayToFile(WavData, *SavePath))
			{
				UE_LOG(LogTemp, Log, TEXT("[Voice] WAV 파일 저장 성공: %s"), *SavePath);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[Voice] WAV 파일 저장 실패: %s"), *SavePath);
			}
			
			OnGetAIAnswer();
		}
	);

	// (14) GET 요청 실행
	GetRequest->ProcessRequest();
}

TArray<uint8> ABaseAgent::FStringToUint8(FString str)
{
	TArray<uint8> outBytes;

	FTCHARToUTF8 converted(*str);
	outBytes.Append(reinterpret_cast<const uint8*>(converted.Get()), converted.Length());

	return outBytes;
}
