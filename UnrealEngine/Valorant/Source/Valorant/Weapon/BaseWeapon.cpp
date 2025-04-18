// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseWeapon.h"

#include "GameManager/ValorantGameInstance.h"
#include "Kismet/GameplayStatics.h"

ABaseWeapon::ABaseWeapon()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	auto* GameInstance = Cast<UValorantGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (nullptr == GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: GameInstance Is Null"));
		return;
	}

	WeaponData = GameInstance->GetWeaponData(WeaponID);
	if (nullptr == WeaponData)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: WeaponData Load Fail (WeaponID : %d)"), WeaponID);
		return;
	}

	// TODO: WeaponID에 맞는 SkeletalMesh 불러오기
	FStringAssetReference MeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/Resource/FPWeapon/Mesh/SK_FPGun.SK_FPGun'"));
	auto* WeaponMeshAsset = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *MeshRef.ToString()));
	if (nullptr == WeaponMeshAsset || nullptr == WeaponMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ABaseWeapon::BeginPlay: WeaponMeshAsset Load Fail (WeaponID : %d)"), WeaponID);
		return;
	}
	WeaponMesh->SetSkeletalMeshAsset(WeaponMeshAsset);
	
	MagazineSize = WeaponData->MagazineSize;
	MagazineAmmo = MagazineSize;
	// TODO: 총기별 여분탄약 데이터 추가 필요
	SpareAmmo = MagazineSize * 3;
	FireInterval = 1.0f / WeaponData->FireRate;
	for (auto Element : WeaponData->GunRecoilMap)
	{
		RecoilData.Add(Element);
	}
}

void ABaseWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
