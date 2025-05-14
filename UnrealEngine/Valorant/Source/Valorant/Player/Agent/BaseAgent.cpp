// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "MapTestAgent.h"
#include "Valorant.h"
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
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "ValorantObject/BaseInteractor.h"
#include "ValorantObject/Spike/Spike.h"
#include "Player/Component/CreditComponent.h"

/* static */
EAgentDamagedPart ABaseAgent::GetHitDamagedPart(const FName& BoneName)
{
	const FString& NameStr = BoneName.ToString();
	if (NameStr.Contains(TEXT("Neck"), ESearchCase::IgnoreCase))
		return EAgentDamagedPart::Head;
	if (NameStr.Contains(TEXT("Clavicle"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Shoulder"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Elbow"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Hand"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Spine"), ESearchCase::IgnoreCase))
		return EAgentDamagedPart::Body;
	if (NameStr.Contains(TEXT("Hip"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Knee"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Foot"), ESearchCase::IgnoreCase) ||
		NameStr.Contains(TEXT("Toe"), ESearchCase::IgnoreCase))
		return EAgentDamagedPart::Legs;
	return EAgentDamagedPart::None;
}



ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(-10, 0, 60));
	SpringArm->TargetArmLength = 0;
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);

	GetMesh()->SetupAttachment(SpringArm);
	GetMesh()->SetRelativeScale3D(FVector(.34f));
	GetMesh()->SetRelativeLocation(FVector(10, 0, -155));

	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ThirdPersonMesh");
	ThirdPersonMesh->SetupAttachment(GetRootComponent());
	ThirdPersonMesh->SetRelativeScale3D(FVector(.34f));
	ThirdPersonMesh->SetRelativeLocation(FVector(.0f, .0f, -90.f));

	ThirdPersonMesh->AlwaysLoadOnClient = true;
	ThirdPersonMesh->AlwaysLoadOnServer = true;
	ThirdPersonMesh->bOwnerNoSee = false;
	ThirdPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	ThirdPersonMesh->bCastDynamicShadow = true;
	ThirdPersonMesh->bAffectDynamicIndirectLighting = true;
	ThirdPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ThirdPersonMesh->SetGenerateOverlapEvents(true);
	ThirdPersonMesh->SetCollisionProfileName(TEXT("Agent"));
	ThirdPersonMesh->SetCanEverAffectNavigation(false);

	GetCapsuleComponent()->SetCapsuleHalfHeight(72.0f);
	BaseCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 330.0f;
	GetCharacterMovement()->SetCrouchedHalfHeight(BaseCapsuleHalfHeight);

	ThirdPersonMesh->SetOwnerNoSee(true);
	GetMesh()->SetOnlyOwnerSee(true);

	AgentInputComponent = CreateDefaultSubobject<UAgentInputComponent>("InputComponent");

	InteractionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionCapsule"));
	InteractionCapsule->SetupAttachment(Camera);
	InteractionCapsule->SetRelativeLocation(FVector(150, 0, 0));
	InteractionCapsule->SetRelativeRotation(FRotator(-90, 0, 0));
	InteractionCapsule->SetCapsuleHalfHeight(150);
	InteractionCapsule->SetCapsuleRadius(35);

	TL_Crouch = CreateDefaultSubobject<UTimelineComponent>("TL_Crouch");
	TL_DieCamera = CreateDefaultSubobject<UTimelineComponent>("TL_DieCamera");

	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

	// 기본값 설정
	// 시야 체크 간격을 0.1초로 설정 (초당 10회)
	VisibilityCheckInterval = 0.1f;
	// 물음표 상태 지속 시간을 3초 설정
	QuestionMarkDuration = 3.0f;
	// 마지막 시야 체크 시간 초기화 
	LastVisibilityCheckTime = 0.0f;
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
}

void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();

	ABP_1P = Cast<UAgentAnimInstance>(GetMesh()->GetAnimInstance());
	ABP_3P = Cast<UAgentAnimInstance>(ThirdPersonMesh->GetAnimInstance());

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

	// // 죽음 카메라 타임라인에 델리게이트 걸기
	// if (IsLocallyControlled())
	// {
	// 	FOnTimelineEvent finishDieCamera;
	// 	finishDieCamera.BindUFunction(this, FName("OnDieCameraFinished"));
	// 	TL_DieCamera->SetTimelineFinishedFunc(finishDieCamera);
	// }

	if (HasAuthority() == false && IsLocallyControlled())
	{
		NET_LOG(LogTemp, Error, TEXT("%hs, HasAuthority() == false && IsLocallyControlled()"), __FUNCTION__);
		InteractionCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABaseAgent::OnFindInteraction);
		InteractionCapsule->OnComponentEndOverlap.AddDynamic(this, &ABaseAgent::OnInteractionCapsuleEndOverlap);
	}

	//TODO: 기본 총, 칼 스폰해서 붙여주기


	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	
	// 서버에서만 시야 체크 처리 -현재 액터가 서버에서 실행 중인지 확인 (권한 있음)
    if (HasAuthority()) 
    {
        // 자동 시야 체크 설정 (시야 체크 함수 호출하여 초기 상태 설정)
        PerformVisibilityChecks();
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


	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

	// 서버에서만 시야 체크 처리
	// 현재 액터가 서버에서 실행 중인지 확인 (권한 있음)
	if (HasAuthority())
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
}

void ABaseAgent::InitAgentAbility()
{
	AAgentPlayerState* ps = GetPlayerState<AAgentPlayerState>();
	if (ps == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerState를 AAgentPlayerState를 상속받는 녀석으로 교체 부탁"));
		return;
	}

	ASC = Cast<UAgentAbilitySystemComponent>(ps->GetAbilitySystemComponent());
	ASC->InitAbilityActorInfo(ps, this);

	if (HasAuthority())
	{
		// 스킬 등록 및 값 초기화는 서버에서만 진행
		ASC->InitializeByAgentData(ps->GetAgentID());

		// UE_LOG(LogTemp, Warning, TEXT("=== ASC 등록된 GA 목록 ==="));
		// for (const FGameplayAbilitySpec& spec : ASC->GetActivatableAbilities())
		// {
		// 	if (spec.Ability)
		// 	{
		// 		UE_LOG(LogTemp, Warning, TEXT("GA: %s"), *spec.Ability->GetName());
		//
		// 		FString TagString;
		// 		TArray<FGameplayTag> tags = spec.GetDynamicSpecSourceTags().GetGameplayTagArray();
		// 		for (const FGameplayTag& Tag : tags)
		// 		{
		// 			TagString += Tag.ToString() + TEXT(" ");
		// 		}
		//
		// 		UE_LOG(LogTemp, Warning, TEXT("태그 목록: %s"), *TagString);
		// 	}
		// }
	}
}

void ABaseAgent::BindToDelegatePC(AAgentPlayerController* pc)
{
	pc->OnHealthChanged_PC.AddDynamic(this, &ABaseAgent::UpdateHealth);
	pc->OnMaxHealthChanged_PC.AddDynamic(this, &ABaseAgent::UpdateMaxHealth);
	pc->OnArmorChanged_PC.AddDynamic(this, &ABaseAgent::UpdateArmor);
	pc->OnEffectSpeedChanged_PC.AddDynamic(this, &ABaseAgent::UpdateEffectSpeed);

	PC = pc;
}

void ABaseAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AgentInputComponent->BindInput(PlayerInputComponent);
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
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	//NET_LOG(LogTemp,Warning,TEXT("OnStartCrouch"));
	TL_Crouch->PlayFromStart();
}

void ABaseAgent::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	//NET_LOG(LogTemp,Warning,TEXT("OnEndCrouch"));
	TL_Crouch->Reverse();
}

void ABaseAgent::StartFire()
{
	if (CurrentInteractor == nullptr)
	{
		NET_LOG(LogTemp, Warning, TEXT("현재 인터랙터 없음"));
		return;
	}

	// TODO: 무기를 발사하다가 교체하였을 때, EndFire() 호출?
	if (auto* weapon = Cast<ABaseWeapon>(CurrentInteractor))
	{
		weapon->StartFire();
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
		weapon->StartReload();
		ABP_3P->Montage_Stop(0.1f);
		if (AM_Reload)
		{
			ABP_3P->Montage_Play(AM_Reload, 1.0f);
		}
	}
}

void ABaseAgent::Interact()
{
	if (FindInteractActor)
	{
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
		if (CurrentInteractor == MainWeapon)
		{
			MainWeapon = nullptr;
		}
		else if (CurrentInteractor == SubWeapon)
		{
			SubWeapon = nullptr;
		}
		CurrentInteractor->ServerRPC_Drop();
		ServerRPC_SetCurrentInteractor(nullptr);

		EquipInteractor(CurrentInteractor);
	}
}

void ABaseAgent::ServerRPC_SetCurrentInteractor_Implementation(ABaseInteractor* interactor)
{
	CurrentInteractor = interactor;
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
	NET_LOG(LogTemp, Warning, TEXT("%hs Called"), __FUNCTION__);
	if (!HasAuthority())
	{
		Server_AcquireInteractor(Interactor);
		return;
	}

	// 스파이크일 경우 처리
	auto* spike = Cast<ASpike>(Interactor);
	if (spike)
	{
		Spike = spike;
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
			// ToDO : 기본 권총을 버려야할지
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
	SwitchInteractor(Interactor->GetInteractorType());
}

void ABaseAgent::SwitchInteractor(EInteractorType InteractorType)
{
	if (HasAuthority())
	{
		if (CurrentInteractor)
		{
			CurrentInteractor->SetActive(false);
		}

		if (InteractorType == EInteractorType::MainWeapon)
		{
			PoseIdxOffset = -11;
			EquipInteractor(MainWeapon);
			UpdateEquipSpeedMultiplier();
		}
		else if (InteractorType == EInteractorType::SubWeapon)
		{
			PoseIdxOffset = -2;
			EquipInteractor(SubWeapon);
			UpdateEquipSpeedMultiplier();
		}
		else if (InteractorType == EInteractorType::Melee)
		{
			PoseIdx = 0;
			EquipInteractor(MeleeKnife);
			UpdateEquipSpeedMultiplier();
		}
		else if (InteractorType == EInteractorType::Spike)
		{
			EquipInteractor(Spike);
			UpdateEquipSpeedMultiplier();
		}
	}
	else
	{
		ServerRPC_SwitchInteractor(InteractorType);
	}
}

void ABaseAgent::ActivateSpike()
{
	if (IsInPlantZone)
	{
		// 스파이크 소지자이고, 설치 상태이면 설치
		if (Spike && Spike->GetSpikeState() == ESpikeState::Carried)
		{
			ServerRPC_Interact(Spike);
		}
		// 스파이크 해제 가능 상태이면 스파이크 해제
		else if (Cast<ASpike>(FindInteractActor))
		{
			ServerRPC_Interact(FindInteractActor);
		}
	}
	else
	{
		if (Spike && Spike->GetSpikeState() == ESpikeState::Planted)
		{
			return;
		}
		SwitchInteractor(EInteractorType::Spike);
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

void ABaseAgent::OnRep_ChangeInteractorState()
{
	if (ABP_1P)
	{
		ABP_1P->InteractorState = CurrentInteractorState;
	}
	if (ABP_3P)
	{
		ABP_3P->InteractorState = CurrentInteractorState;
	}
}

void ABaseAgent::OnRep_ChangePoseIdx()
{
	if (ABP_1P)
	{
		ABP_1P->InteractorPoseIdx = PoseIdx;
	}
	if (ABP_3P)
	{
		ABP_3P->InteractorPoseIdx = PoseIdx;
	}
}

void ABaseAgent::Server_AcquireInteractor_Implementation(ABaseInteractor* Interactor)
{
	AcquireInteractor(Interactor);
}

void ABaseAgent::ServerRPC_SwitchInteractor_Implementation(EInteractorType InteractorType)
{
	SwitchInteractor(InteractorType);
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

	if (CurrentInteractor == nullptr)
	{
		CurrentInteractorState = EInteractorType::None;
		ABP_1P->InteractorState = EInteractorType::None;
		ABP_3P->InteractorState = EInteractorType::None;

		NET_LOG(LogTemp, Warning, TEXT("빈손이네요"));
		return;
	}
	CurrentInteractorState = CurrentInteractor->GetInteractorType();
	if (CurrentInteractorState == EInteractorType::Spike)
	{
		// ToDo : 위치 맞게 수정
		CurrentInteractor->SetActorLocation(GetActorLocation() + GetActorUpVector() * -200);
	}

	CurrentInteractor->SetActive(true);

	if (ABP_1P)
	{
		ABP_1P->InteractorState = CurrentInteractorState;
	}
	if (ABP_3P)
	{
		ABP_3P->InteractorState = CurrentInteractorState;
	}

	//TODO: 인터랙터에도 적용할지 말지
	if (auto* weapon = Cast<ABaseWeapon>(CurrentInteractor))
	{
		PoseIdx = weapon->GetWeaponID() + PoseIdxOffset;
		ABP_1P->InteractorPoseIdx = PoseIdx;
		ABP_3P->InteractorPoseIdx = PoseIdx;
	}

	NET_LOG(LogTemp, Warning, TEXT("현재 들고 있는 인터랙터: %s"), *CurrentInteractor->GetActorNameOrLabel());
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
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, 이미 감지된 Interactor가 있음"), __FUNCTION__);
			return;
		}
	}

	if (auto* Interactor = Cast<ABaseInteractor>(OtherActor))
	{
		if (CurrentInteractor == Interactor)
		{
			NET_LOG(LogTemp, Error, TEXT("%hs Called, 현재 들고 있는 Interactor와 동일함"), __FUNCTION__);
			return;
		}
		if (Interactor->HasOwnerAgent())
		{
			NET_LOG(LogTemp, Warning, TEXT("%hs Called, 이미 주인이 있는 Interactor"), __FUNCTION__);
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
		
		NET_LOG(LogTemp, Warning, TEXT("%hs Called, Interactor Name is %s"), __FUNCTION__, *Interactor->GetName());
		FindInteractActor = Interactor;
		FindInteractActor->OnDetect(true);
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
	float newHalfHeight = BaseCapsuleHalfHeight - Value;
	//NET_LOG(LogTemp,Warning,TEXT("HandleCrouchProgress %f"), newHalfHeight);
	GetCapsuleComponent()->SetCapsuleHalfHeight(newHalfHeight, true);
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
	if (MainWeapon)
	{
		MainWeapon->ServerRPC_Drop();
	}
	if (SubWeapon)
	{
		SubWeapon->ServerRPC_Drop();
	}
	MeleeKnife->Destroy();
	
	Net_Die();
	// 킬러 플레이어 컨트롤러 찾기
	AMatchPlayerController* KillerPC = nullptr;

	// Instigator 로그 추가
	AActor* InstigatorActor = GetInstigator();
	if (InstigatorActor)
	{
		NET_LOG(LogTemp, Warning, TEXT("Die() - Instigator 정보: %s"), *InstigatorActor->GetName());
	}
	else
	{
		NET_LOG(LogTemp, Warning, TEXT("Die() - Instigator가 없습니다. 데미지 적용 시 Instigator가 제대로 설정되지 않았습니다."));
	}

	if (GetInstigator() && GetInstigator() != this)
	{
		KillerPC = Cast<AMatchPlayerController>(GetInstigator()->GetController());
		// 키의 유효성 검사
		NET_LOG(LogTemp, Warning, TEXT("죽음 처리: 킬러 컨트롤러 - %s"), KillerPC ? *KillerPC->GetName() : TEXT("없음"));

		if (KillerPC)
		{
			GetWorld()->GetAuthGameMode<AMatchGameMode>()->OnKill(KillerPC, PC);
			// AAgentPlayerState* KillerPS = KillerPC->GetPlayerState<AAgentPlayerState>();
			// if (KillerPS)
			// {
			// 	UCreditComponent* CreditComp = KillerPS->FindComponentByClass<UCreditComponent>();
			// 	if (CreditComp)
			// 	{
			// 		// 헤드샷 여부 체크 (데미지 시스템에서 구현 필요)
			// 		bool bIsHeadshot = false; // 임시로 false 설정
			// 		CreditComp->AwardKillCredits(bIsHeadshot);
			// 		
			// 		NET_LOG(LogTemp, Warning, TEXT("%s가 %s를 처치하여 크레딧 보상을 받았습니다."), 
			// 			*KillerPC->GetPlayerState<APlayerState>()->GetPlayerName(), 
			// 			*GetPlayerState<APlayerState>()->GetPlayerName());
			// 		
			// 	}
			// }
		}
	}
	
	FTimerHandle deadTimerHandle;
	GetWorldTimerManager().SetTimer(deadTimerHandle, FTimerDelegate::CreateLambda([this]()
	{
		OnDieCameraFinished();
	}), DieCameraTimeRange, false);

	ThirdPersonMesh->SetOwnerNoSee(false);
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

void ABaseAgent::Net_Die_Implementation()
{
	if (IsLocallyControlled())
	{
		DisableInput(Cast<APlayerController>(GetController()));

		ThirdPersonMesh->SetOwnerNoSee(false);
		GetMesh()->SetVisibility(false);

		TL_DieCamera->PlayFromStart();
	}
	
	bIsDead = true;

	ABP_3P->Montage_Stop(0.1f);
	ABP_3P->Montage_Play(AM_Die, 1.0f);
	ABP_3P->bIsDead = true;
}

void ABaseAgent::ServerApplyGE_Implementation(TSubclassOf<UGameplayEffect> geClass)
{
	if (!geClass)
	{
		NET_LOG(LogTemp, Error, TEXT("올바른 게임이펙트를 넣어주세요."));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(geClass, 1.f, Context);

	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ABaseAgent::ServerApplyHitScanGE_Implementation(TSubclassOf<UGameplayEffect> GEClass, const int Damage,
                                                     ABaseAgent* DamageInstigator)
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

void ABaseAgent::UpdateHealth(float newHealth)
{
	if (newHealth <= 0.f && bIsDead == false)
	{
		Die();
	}
}

void ABaseAgent::UpdateMaxHealth(float newMaxHealth)
{
}

void ABaseAgent::UpdateArmor(float newArmor)
{
}

void ABaseAgent::UpdateEffectSpeed(float newSpeed)
{
	NET_LOG(LogTemp, Warning, TEXT("%f dp"), newSpeed);
	EffectSpeedMultiplier = newSpeed;
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

//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//             CYT             ♣
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

// 미니맵 아이콘 설정 함수
void ABaseAgent::SetMinimapIcon(UTexture2D* NewIcon)
{
	// 새 아이콘으로 갱신
	MinimapIcon = NewIcon;
}

// 가시성 상태 조회 함수 - TMap 대신 TArray에서 검색하도록 수정
EVisibilityState ABaseAgent::GetVisibilityStateForAgent(ABaseAgent* Observer)
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




void ABaseAgent::OnRep_VisibilityStateArray()
{
	// 클라이언트에서 배열이 업데이트될 때 호출됨 
	// UI 업데이트나 시각 효과 처리를 여기서 할 수 있음 
}

// 네트워크 복제 설정
void ABaseAgent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseAgent, VisibilityStateArray);
	DOREPLIFETIME(ABaseAgent, bIsRun);
	DOREPLIFETIME(ABaseAgent, MeleeKnife);
	DOREPLIFETIME(ABaseAgent, MainWeapon);
	DOREPLIFETIME(ABaseAgent, SubWeapon);
	DOREPLIFETIME(ABaseAgent, Spike);
	DOREPLIFETIME(ABaseAgent, CurrentInteractor);
	DOREPLIFETIME(ABaseAgent, CurrentInteractorState);
	DOREPLIFETIME(ABaseAgent, PoseIdx);
	DOREPLIFETIME(ABaseAgent, IsInPlantZone);
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
