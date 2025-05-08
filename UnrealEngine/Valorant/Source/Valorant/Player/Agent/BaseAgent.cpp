// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAgent.h"
#include "AbilitySystemComponent.h"
#include "MapTestAgent.h"
#include "Valorant.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Valorant/AbilitySystem/AgentAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameManager/MatchGameMode.h"
#include "GameManager/SubsystemSteamManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/Animaiton/AgentAnimInstance.h"
#include "Player/Component/AgentInputComponent.h"
#include "Valorant/Player/AgentPlayerController.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "ValorantObject/BaseInteractor.h"
#include "ValorantObject/Spike/Spike.h"


class AAgentPlayerState;

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
	GetMesh()->SetRelativeLocation(FVector(10,0,-155));
	
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ThirdPersonMesh");
	ThirdPersonMesh->SetupAttachment(GetRootComponent());
	ThirdPersonMesh->SetRelativeScale3D(FVector(.34f));
	ThirdPersonMesh->SetRelativeLocation(FVector(.0f,.0f,-90.f));
	
	ThirdPersonMesh->AlwaysLoadOnClient = true;
	ThirdPersonMesh->AlwaysLoadOnServer = true;
	ThirdPersonMesh->bOwnerNoSee = false;
	ThirdPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	ThirdPersonMesh->bCastDynamicShadow = true;
	ThirdPersonMesh->bAffectDynamicIndirectLighting = true;
	ThirdPersonMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ThirdPersonMesh->SetGenerateOverlapEvents(false);
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
	// 상태 숨김 초기화
	MinimapVisibility = EAgentVisibility::Hidden;
	// 마지막 시야 시간 0으로 초기화
	LastVisibleTime = 0.0f;
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
		TL_DieCamera->AddInterpVector(DieCameraCurve,CamOffset);
		TL_DieCamera->AddInterpFloat(DieCameraPitchCurve,CamPitch);
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

	InteractionCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABaseAgent::OnFindInteraction);
	InteractionCapsule->OnComponentEndOverlap.AddDynamic(this, &ABaseAgent::OnInteractionCapsuleEndOverlap);

	//TODO: 기본 총, 칼 스폰해서 붙여주기
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

void ABaseAgent::HandleCrouchProgress(float Value)
{
	float newHalfHeight = BaseCapsuleHalfHeight - Value;
	//NET_LOG(LogTemp,Warning,TEXT("HandleCrouchProgress %f"), newHalfHeight);
	GetCapsuleComponent()->SetCapsuleHalfHeight(newHalfHeight,true);
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

void ABaseAgent::SetShopUI()
{
	if (IsLocallyControlled())
	{
		PC->RequestShopUI();
	}
	else
	{
		
	}
}

void ABaseAgent::EquipSpike(ASpike* spike)
{
	//TODO: 픽업 성공했는지 여부에 따라 다르게 처리
}

void ABaseAgent::EquipWeapon(ABaseWeapon* weapon)
{
	// UE_LOG(LogTemp,Warning,TEXT("이큅 웨폰"));
	if(weapon->GetWeaponCategory() == EWeaponCategory::Sidearm)
	{
		UE_LOG(LogTemp,Warning,TEXT("보조무기"));
		if (SecondWeapon)
		{
			// UE_LOG(LogTemp,Warning,TEXT("이미 들고 있음"));
			SecondWeapon->Drop();
		}
		
		SecondWeapon = weapon;
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("주무기"));
		if (PrimaryWeapon)
		{
			// UE_LOG(LogTemp,Warning,TEXT("이미 들고 있음"));
			PrimaryWeapon->Drop();
		}
            
		PrimaryWeapon = weapon;
	}

	weapon->PickUp(this);
	SetInteractorState(weapon->GetInteractorType());
}

void ABaseAgent::SetInteractorState(const EInteractorType newState)
{
	CurrentInteractorState = newState;
	
	if (newState == EInteractorType::MainWeapon)
	{
		SetCurrentInteractor(PrimaryWeapon);
	}
	else if (newState == EInteractorType::SubWeapon)
	{
		SetCurrentInteractor(SecondWeapon);
	}
	else if (newState == EInteractorType::Melee)
	{
		//TODO: 칼 넣어주기
		SetCurrentInteractor(nullptr);
	}
	else if (newState == EInteractorType::Spike)
	{
		//TODO: 스파이크 넣어주기
		SetCurrentInteractor(nullptr);
	}
}

void ABaseAgent::SetCurrentInteractor(ABaseInteractor* interactor)
{
	//TODO: 기존 들고 있던 물건 숨기고 새로운 인터랙터 활성화
	//TODO: EInteractorType 따른 애니메이션 재생

	CurrentInteractor = interactor;
	
	if (CurrentInteractor == nullptr)
	{
		CurrentInteractorState = EInteractorType::None;
		CurrentInteractorState = EInteractorType::None;
		return;
	}

	UE_LOG(LogTemp,Warning,TEXT("현재 들고 있는 인터랙터: %s"), *CurrentInteractor->GetActorNameOrLabel());
}

void ABaseAgent::Server_SetInteractorState_Implementation(EInteractorType newState)
{
	CurrentInteractorState = newState;
}

/** CurrentInteractorState 변경시 호출 */
void ABaseAgent::OnRep_InteractorState()
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

void ABaseAgent::Interact()
{
	if (FindInteractActor)
	{
		if (ABaseWeapon* weapon = Cast<ABaseWeapon> (FindInteractActor))
		{
			EquipWeapon(weapon);
		}
		else if (ASpike* spike = Cast<ASpike> (FindInteractActor))
		{
			EquipSpike(spike);			
		}
	}
}

void ABaseAgent::DropCurrentInteractor()
{
	if (CurrentInteractor && CurrentInteractor->CanDrop())
	{
		if (CurrentInteractor == PrimaryWeapon)
		{
			PrimaryWeapon = nullptr;
		}
		else if (CurrentInteractor == SecondWeapon)
		{
			SecondWeapon = nullptr;
		}
		
		CurrentInteractor->Drop();
		SetCurrentInteractor(nullptr);
	}
}

void ABaseAgent::OnFindInteraction(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const ECollisionChannel ObjType = OtherComp->GetCollisionObjectType();
	if (ObjType != ECC_GameTraceChannel1)
	{
		return;
	}
	
	// 이미 바라보고 있는 총이 있으면 리턴
	if (FindInteractActor)
	{
		if (auto* weapon = Cast<ABaseWeapon>(FindInteractActor))
		{
			return;
		}
	}
	
	if (auto* interactor = Cast<ABaseInteractor>(OtherActor))
	{
		FindInteractActor = interactor;
		FindInteractActor->InteractActive(true);
	}
}

void ABaseAgent::OnInteractionCapsuleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const auto* interactor = Cast<ABaseInteractor>(OtherActor))
	{
		if (interactor == FindInteractActor)
		{
			FindInteractActor->InteractActive(false);
			FindInteractActor = nullptr;
		}
	}
}

void ABaseAgent::HandleDieCamera(FVector newPos)
{
	Camera->SetRelativeLocation(newPos);
	//UE_LOG(LogTemp,Warning,TEXT("pos: %f"),newPos.Z);
}


void ABaseAgent::HandleDieCameraPitch(float newPitch)
{
	Camera->SetRelativeRotation(FRotator(newPitch,0,0));
	//UE_LOG(LogTemp,Warning,TEXT("pitch %f"),newPitch);
}

void ABaseAgent::Die()
{
	// NET_LOG(LogTemp,Warning,TEXT("죽음"));
	
	//TODO: 복제 어떻게 진행할지
	if (PrimaryWeapon)
	{
		PrimaryWeapon->Drop();
	}
	if (SecondWeapon)
	{
		SecondWeapon->Drop();
	}
	
	if (IsLocallyControlled())
	{
		DisableInput(Cast<APlayerController>(GetController()));

		ThirdPersonMesh->SetOwnerNoSee(false);
		GetMesh()->SetVisibility(false);
		
		TL_DieCamera->PlayFromStart();
	}
	
	if (HasAuthority())
	{
		// NET_LOG(LogTemp,Warning,TEXT("다이 캠 피니쉬 타이머 설정"));
		GetWorld()->GetAuthGameMode<AMatchGameMode>()->OnKill(Cast<AMatchPlayerController>(GetOwner()), Cast<AMatchPlayerController>(GetOwner()));
		FTimerHandle deadTimerHandle;
		GetWorldTimerManager().SetTimer(deadTimerHandle, FTimerDelegate::CreateLambda([this]()
		{
			OnDieCameraFinished();
		}), DieCameraTimeRange, false);

		ThirdPersonMesh->SetOwnerNoSee(false);
		Net_Die();
	}
}

/** 서버에서만 호출됨*/
void ABaseAgent::OnDieCameraFinished()
{
	// NET_LOG(LogTemp,Warning,TEXT("다이 캠 피니쉬 콜백"));
	
	AAgentPlayerController* pc = Cast<AAgentPlayerController>(GetController());
	if (pc)
	{
		pc->StartSpectatingOnly();
		Destroy();
		
		pc->Client_EnterSpectatorMode();
	}
	else
	{
		// NET_LOG(LogTemp, Error, TEXT("OnDieCameraFinished: Controller가 없습니다!"));
	}
}

void ABaseAgent::Net_Die_Implementation()
{
	bIsDead = true;
	
	ABP_3P->Montage_Stop(0.1f);
	ABP_3P->Montage_Play(AM_Die, 1.0f);
	ABP_3P->bIsDead = true;
}

void ABaseAgent::ServerApplyGE_Implementation(TSubclassOf<UGameplayEffect> geClass)
{
	if (!geClass)
	{
		NET_LOG(LogTemp,Error,TEXT("올바른 게임이펙트를 넣어주세요."));
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(geClass, 1.f, Context);

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
	NET_LOG(LogTemp,Warning,TEXT("%f dp"), newSpeed);
	EffectSpeedMultiplier = newSpeed;
}


//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
//             CYT             ♣
//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
// 네트워크 복제 설정
void ABaseAgent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 네트워크로 복제할 변수 등록 (가시성상태 , 마지막시야 확인시간 , 팀ID)
	DOREPLIFETIME(ABaseAgent, MinimapVisibility);
	DOREPLIFETIME(ABaseAgent, LastVisibleTime);
	DOREPLIFETIME(ABaseAgent, TeamID);
	DOREPLIFETIME(ABaseAgent, bIsRun);
	DOREPLIFETIME(ABaseAgent, CurrentInteractorState);
	DOREPLIFETIME(ABaseAgent, PrimaryWeapon);
	DOREPLIFETIME(ABaseAgent, SecondWeapon);
	DOREPLIFETIME(ABaseAgent, CurrentInteractor);
}

// 특정 플레이어에게 보이는지 체크
bool ABaseAgent::IsVisibleToTeam(int32 ViewerTeamID) const
{
	// 같은 팀은 항상 보이게
	if (TeamID == ViewerTeamID)
	{
		UE_LOG(LogTemp, Warning, TEXT("같은팀 "))
		return true;
	}
	// 적팀 시야 체크

	// 현재 월드의 모든 에이전트 가져오자
	//TArray<AActor*> AllAgents;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), AllAgents);
	// 각 플레이어 에이전트에 대해 체크
	// for (AActor* Agent : AllAgents)
	// {
	// 	ABaseAgent* PlayerAgent = Cast<ABaseAgent>(Agent);
		// 유효한 플레이어 지정된 팀ID 일치하는지 확인
		// if (PlayerAgent && PlayerAgent->TeamID == ViewerTeamID)
		// {
			// 이플레이어의 시야에 현재 에이전트가 있는지 //@@TODO YT
			// if (PlayerAgent->IsLineOfSightBlocked(this))
			// {
			// 	// 한명이라도 볼 수 있으면 보이는 것으로 판단
			// 	UE_LOG(LogTemp, Warning, TEXT("다른팀 보일랑말랑"));
			// 	return true;
			// }
	// 	}
	// }
	 UE_LOG(LogTemp, Warning, TEXT("다른팀 안보임"));
	return false;
}

// 미니맵 가시성 상태 업데이트
void ABaseAgent::UpdateMinimapVisibility()
{
	// 서버에서만 실행 (권한 있는 쪽에서만 상태 변경)
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	
	// 게임의 현재 시간 가져오기 
	float CurrentTime = UGameplayStatics::GetTimeSeconds(GetWorld());
	
	// 시야에서 사라진 후 경과 시간 계산
	float TimeSinceLastSeen = CurrentTime - LastVisibleTime;
	
	//현재 상태에 따른 업데이트 로직
	if (MinimapVisibility == EAgentVisibility::Visible)
	{
		// 현재 보이는 상태인테 시야에서 벗어났다면 물음표 상태로 변경
		if (!IsVisibleToOpponents())
		{
			MinimapVisibility = EAgentVisibility::LastKnown;
			// 마지막 본 시간 업데이트
			LastVisibleTime = CurrentTime;
		}
	}
	// 물음표 상태일때 업데이트 로직 
	else if (MinimapVisibility == EAgentVisibility::LastKnown)
	{
		// 물음표 상태인데 다시 시야에 들어왔다면 Visible 상태 변경
		if (IsVisibleToOpponents())
		{
			MinimapVisibility = EAgentVisibility::Visible;
		}
		// 물음표 표시 시간이 지났다면 Hidden 상태로 변경
		else if (TimeSinceLastSeen > QuestionMarkDuration)
		{
			MinimapVisibility = EAgentVisibility::Hidden;
		}
	}
	//숨김 상태인데 다시 시야에 들어왔다면 Visible로 상타로 변경
	else if (MinimapVisibility == EAgentVisibility::Hidden)
	{
		if (IsVisibleToOpponents())
		{
			MinimapVisibility = EAgentVisibility::Visible;
		}
	}
}

// 미니맵에 표시될 아이콘 가져오기
UTexture2D* ABaseAgent::GetAgentIcon(int32 ViewerTeamID) const
{
	// 같은 팀이면 항상 기본 아이콘 표시 (팀원은 항상 미니맵에 표시)
	if (TeamID == ViewerTeamID)
	{
		return AgentIcon;
	}

	// 다른팀에 대한 표시 규칙
	switch (MinimapVisibility)
	{
		// 기본시야에 보이는경우 에이전트 아이콘으로 표시
		case EAgentVisibility::Visible: return AgentIcon;
		// 마지막으로 본위치 물음표 아이콘으로 표시
		case EAgentVisibility::LastKnown: return QuestionMarkIcon;
		// 숨김 상태 아이콘 표시 없음
		case EAgentVisibility::Hidden: default: return nullptr;
	}
}

bool ABaseAgent::IsVisibleToOpponents() const
{
	// 모든 플레이어 컨트롤러 Actor 배열로 가져오기
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass
	(GetWorld(),APlayerController::StaticClass(),FoundActors);

	// 각 Actor를 APlayController로 캐스팅하여 검사 
	for (AActor* Actor : FoundActors)
	{
		APlayerController* Plc = Cast<APlayerController>(Actor);
		if (!Plc)
		{
			continue;
		}
		AMapTestAgent* PlayerAgent = Cast<AMapTestAgent>(Plc->GetPawn());
		// 같은 팀이면 진행
		if (!PlayerAgent || PlayerAgent->TeamID == TeamID)
		{
			continue;
		}
		// 플레이어가 이 에이전트 볼 수 있는지 체크
		if(!PlayerAgent->IsLineOfSightBlocked(this))
		{
			// 한명이라도 볼수 있다면 true 반환
			return true;
		}
	}
	// 아무도 볼수 없다면 false 반환 
	return false;
}
