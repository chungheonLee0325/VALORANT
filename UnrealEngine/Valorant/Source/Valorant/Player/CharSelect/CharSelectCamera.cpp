// Fill out your copyright notice in the Description page of Project Settings.


#include "CharSelectCamera.h"

#include "CharSelectCharacterActor.h"
#include "Valorant.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameManager/SubsystemSteamManager.h"
#include "GameManager/ValorantGameInstance.h"


ACharSelectCamera::ACharSelectCamera()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(Root);
	
	CameraMesh = CreateDefaultSubobject<USkeletalMeshComponent>("CameraMesh");
	CameraMesh->SetupAttachment(RootComponent);
	static const ConstructorHelpers::FObjectFinder<USkeletalMesh> CameraMeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/Agent/CharSelect/ABCS_Core_Camera_Skelmesh.ABCS_Core_Camera_Skelmesh'"));
	if (CameraMeshAsset.Succeeded())
	{
		CameraMesh->SetSkeletalMeshAsset(CameraMeshAsset.Object);
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> CameraAnimClass(TEXT("/Script/Engine.AnimBlueprint'/Game/Resource/Agent/CharSelect/ABP_ABCS_Core_Camera.ABP_ABCS_Core_Camera_C'"));
	if (CameraAnimClass.Succeeded())
	{
		CameraMesh->SetAnimInstanceClass(CameraAnimClass.Class);
	}
	
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(CameraMesh, TEXT("CameraSocket"));
	Camera->SetRelativeRotation(FRotator(0, 180, 0));
	Camera->SetFieldOfView(45.f);
}

void ACharSelectCamera::OnSelectedAgent(const int SelectedAgentID)
{
	auto* GameInstance = GetGameInstance<UValorantGameInstance>();
	if (nullptr == GameInstance)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, GameInstance is nullptr"), __FUNCTION__);
		return;
	}

	const auto* AgentData = GameInstance->GetAgentData(SelectedAgentID);
	if (nullptr == AgentData)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, AgentData is nullptr"), __FUNCTION__);
		return;
	}

	const auto CharacterActorClass = AgentData->CharSelectCharacterActorClass;
	if (nullptr == CharacterActorClass)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, CharacterActorClass is nullptr"), __FUNCTION__);
		return;
	}

	if (CharacterActor)
	{
		CharacterActor->Destroy();
		CharacterActor = nullptr;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CharacterActor = GetWorld()->SpawnActor<ACharSelectCharacterActor>(CharacterActorClass, SpawnParameters);
	CharacterActor->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

	const auto* CharacterMontage = AgentData->CharSelectCharacterMontage;
	if (nullptr == CharacterMontage)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, CharacterMontage is nullptr"), __FUNCTION__);
		return;
	}
	CharacterActor->PlaySelectAnimation(CharacterMontage);
	
	const auto* CameraMontage = AgentData->CharSelectCameraMontage;
	if (nullptr == CameraMontage)
	{
		NET_LOG(LogTemp, Error, TEXT("%hs Called, CameraMontage is nullptr"), __FUNCTION__);
		return;
	}
	PlaySelectAnimation(CameraMontage);
}

void ACharSelectCamera::BeginPlay()
{
	Super::BeginPlay();
}
