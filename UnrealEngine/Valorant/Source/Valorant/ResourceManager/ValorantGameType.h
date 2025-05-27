#pragma once

#include <Engine/DataTable.h>
#include "CoreMinimal.h"
#include "GameplayAbilitySet.h"
#include "UObject/ObjectMacros.h"
#include "GameplayTagContainer.h"
#include "ValorantGameType.generated.h"

class ACharSelectCharacterActor;
class UNiagaraSystem;
class UBaseWeaponAnim;
class ABaseWeapon;
class ABaseAgent;
struct FGameplayTag;
class UGameplayEffect;
class UGameplayAbility;

// Enum
// 언리얼 리플렉션 시스템과 통합하기 위해 UENUM() 매크로를 사용

UENUM(BlueprintType)
enum class EValorantMap : uint8
{
	None UMETA(DisplayName = "None"),

	Ascent UMETA(DisplayName = "Ascent"),
	Bind UMETA(DisplayName = "Bind"),
	Haven UMETA(DisplayName = "Haven"),
	Split UMETA(DisplayName = "Split"),
	Icebox UMETA(DisplayName = "Icebox"),
	Breeze UMETA(DisplayName = "Breeze"),
	Fracture UMETA(DisplayName = "Fracture"),
	Pearl UMETA(DisplayName = "Pearl")
};

UENUM(BlueprintType)
enum class EAgentRole : uint8
{
	None UMETA(DisplayName = "None"),

	// 에이전트 역할
	Duelist UMETA(DisplayName = "Duelist"),
	Initiator UMETA(DisplayName = "Initiator"),
	Sentinel UMETA(DisplayName = "Sentinel"),
	Controller UMETA(DisplayName = "Controller")
};

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	None UMETA(DisplayName = "None"),

	EntryFragger UMETA(DisplayName = "Entry Fragger"),
	Support UMETA(DisplayName = "Support"),
	Lurker UMETA(DisplayName = "Lurker"),
	IGL UMETA(DisplayName = "In-Game Leader")
};

UENUM(BlueprintType)
enum class EInteractorType : uint8
{
	None UMETA(DisplayName = "None"),
	MainWeapon UMETA(DisplayName = "MainWeapon"),
	SubWeapon UMETA(DisplayName = "SubWeapon"),
	Melee UMETA(DisplayName = "Melee"),
	Spike UMETA(DisplayName = "Spike"),
	Ability UMETA(DisplayName = "Ability"),
};

// 무기 카테고리
UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	None UMETA(DisplayName = "None"),
	
	// Melee 
	Melee UMETA(DisplayName = "Melee"),
	// SubWeapon 
	Sidearm UMETA(DisplayName = "Sidearm"),
	// MainWeapon
	SMG UMETA(DisplayName = "SMG"),
	// MainWeapon
	Shotgun UMETA(DisplayName = "Shotgun"),
	// MainWeapon
	Rifle UMETA(DisplayName = "Rifle"),
	// MainWeapon
	Sniper UMETA(DisplayName = "Sniper"),
	// MainWeapon
	Heavy UMETA(DisplayName = "Heavy Weapon")
};

// 무기 타입 
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),

	// Melee
	Melee UMETA(DisplayName = "Melee"),
	
	// Sidearms
	Classic UMETA(DisplayName = "Classic"),
	Shorty UMETA(DisplayName = "Shorty"),
	Frenzy UMETA(DisplayName = "Frenzy"),
	Ghost UMETA(DisplayName = "Ghost"),
	Sheriff UMETA(DisplayName = "Sheriff"),

	// SMGs
	Stinger UMETA(DisplayName = "Stinger"),
	Spectre UMETA(DisplayName = "Spectre"),

	// Shotguns
	Bucky UMETA(DisplayName = "Bucky"),
	Judge UMETA(DisplayName = "Judge"),

	// Rifles
	Bulldog UMETA(DisplayName = "Bulldog"),
	Guardian UMETA(DisplayName = "Guardian"),
	Phantom UMETA(DisplayName = "Phantom"),
	Vandal UMETA(DisplayName = "Vandal"),

	// Snipers
	Marshal UMETA(DisplayName = "Marshal"),
	Outlaw UMETA(DisplayName = "Outlaw"),
	Operator UMETA(DisplayName = "Operator"),

	// Heavy Weapons
	Ares UMETA(DisplayName = "Ares"),
	Odin UMETA(DisplayName = "Odin")
};

// 스탯 유형 정의
UENUM(BlueprintType)
enum class EValorantStatType : uint8
{
	None UMETA(DisplayName = "None"),

	// 기본 전투 스탯
	Kills UMETA(DisplayName = "Kills"),
	Deaths UMETA(DisplayName = "Deaths"),
	Assists UMETA(DisplayName = "Assists"),
	HeadshotPercentage UMETA(DisplayName = "Headshot Percentage"),
	AverageDamage UMETA(DisplayName = "Average Damage per Round"),
	CombatScore UMETA(DisplayName = "Combat Score"),

	// 전략적 스탯
	FirstKills UMETA(DisplayName = "First Kills"),
	ClutchWins UMETA(DisplayName = "Clutch Wins"),
	MultiKills UMETA(DisplayName = "Multi-Kills"),
	EconManagement UMETA(DisplayName = "Economy Management"),

	// 기술적 스탯
	Accuracy UMETA(DisplayName = "Accuracy"),
	SprayControl UMETA(DisplayName = "Spray Control"),
	UtilityUsage UMETA(DisplayName = "Utility Usage")
};

// 발로란트 능력 유형 (기본, 시그니처, 궁극기)
UENUM(BlueprintType)
enum class EValorantAbilityType : uint8
{
	None UMETA(DisplayName = "None"),
	// 기본 능력 (크레딧으로 구매)
	Basic UMETA(DisplayName = "Basic"),
	// 시그니처 능력 (라운드마다 충전)
	/*
	*고유 스킬(Signature abilities)은 구매하지 않아도 반드시 한 번은 사용할 수 있으며, 2회 이상 사용하기 위해 특정 조건을 만족하여 충전하거나 별도로 구매할 수 있다. '고유 스킬'인 만큼 그 캐릭터의 전반적인 컨셉이 해당 스킬에 담겨있다.
레이나, 브림스톤, 스카이, 아스트라, 오멘, 요루, 클로브, 피닉스의 여덟 요원은 2회 이상의 충전량을 구매할 수 있다.
게코, 데드록, 바이스, 브리치, 사이퍼, 세이지, 소바, 오멘, 체임버, 케이/오, 클로브, 킬조이, 테호, 페이드, 하버의 열다섯 요원은 고유 스킬을 사용한 후 재사용 대기시간을 거칠 때마다 1회씩 충전된다. 이 중 오멘과 클로브는 2회 이상의 충전량을 구매하지 않으면 해당 충전량의 재사용 대기시간이 적용되지 않으며 영구히 충전되지 않는다.
네온, 레이즈, 요루, 웨이레이, 제트, 피닉스의 여섯 요원은 적 두 명을 처치하여 사용한 스킬을 재충전할 수 있다.
레이나, 브림스톤, 스카이, 아스트라, 아이소의 다섯 요원은 사용한 스킬이 영구히 충전되지 않는다. 또한 바이퍼는 연료를 소모하는 특수한 매커니즘으로 인해 스킬이 별도로 충전되지 않는다.
	 */
	Signature UMETA(DisplayName = "Signature"),
	// 궁극기 (포인트로 충전)
	/*
궁극기(Ultimate abilities)는 궁극기 포인트를 모두 모아야 사용할 수 있다. 궁극기 포인트는 적을 사살, 적에게 사망[1], 스파이크 설치/해체, 매 라운드마다 맵에 배치되는 궁극기 구슬을 습득하여 1개씩 얻을 수 있다. 요원에 따라 6 포인트에서 9 포인트를 획득하면 사용할 수 있다. 궁극기 포인트를 모두 모으면 사용 후 효과가 종료될 때까지 여분의 포인트를 모을 수 없다. 즉, 궁극기를 사용해 적을 처치하면 궁극기 포인트를 얻을 수 없다.
6 포인트 : 레이나, 피닉스
7 포인트 : 데드록, 사이퍼, 세이지, 아스트라, 아이소, 오멘, 요루, 하버
8 포인트 : 레이즈, 바이스, 브림스톤, 스카이, 소바, 웨이레이, 제트, 체임버, 게코, 클로브, 케이/오, 테호, 페이드, 네온
9 포인트 : 바이퍼, 브리치, 킬조이
	 */
	Ultimate UMETA(DisplayName = "Ultimate"),
};

UENUM(BlueprintType)
enum class EAbilitySlotType : uint8
{
	None UMETA(DisplayName = "None"),
	Slot_C UMETA(DisplayName = "Slot C"),
	Slot_Q UMETA(DisplayName = "Slot Q"),
	Slot_E UMETA(DisplayName = "Slot E"),
	Slot_X UMETA(DisplayName = "Slot X"),
};

USTRUCT(BlueprintType)
struct FGunDamageFalloffData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int RangeStart = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float DamageMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FGunRecoilData : public FTableRowBase
{
	GENERATED_BODY()
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	// int32 RecoilID = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float OffsetPitch = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float OffsetYaw = 0.0f;
};

// AgentData
USTRUCT(BlueprintType)
struct FAgentData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 AgentID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString AgentName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<ABaseAgent> AgentAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EAgentRole AgentRole = EAgentRole::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 MaxHealth = 100;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 MaxArmor = 1000;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 Speed = 600;

	// 고유 패시브 효과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayEffect> PassiveEffectClass = nullptr;

	// 기본 어빌리티
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<TSubclassOf<UGameplayAbility>> BasicAbilities;

	// 스킬 C
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 AbilityID_C = 0;

	// 스킬 Q
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 AbilityID_Q = 0;

	// 스킬 E
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 AbilityID_E = 0;

	// 궁극기 X
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 AbilityID_X = 0;

	// 에이전트 특성 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGameplayTag> AgentTags;

	// Agent Attribute Set
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbilitySet> AttributeSetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString LocalName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString Description;

	// 캐릭터 선택 연출
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<ACharSelectCharacterActor> CharSelectCharacterActorClass = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UAnimMontage* CharSelectCharacterMontage = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UAnimMontage* CharSelectCameraMontage = nullptr;
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 WeaponID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EWeaponCategory WeaponCategory = EWeaponCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EWeaponType WeaponName = EWeaponType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString LocalName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<ABaseWeapon> WeaponClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float FireRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 MagazineSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float ReloadTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float HeadshotMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float LegshotMultiplier = 1.5f;

	// 크레딧 비용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 Cost = 0;

	// 무기 메쉬
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	USkeletalMesh* WeaponMesh = nullptr;
	
	// 무기 대미지 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;

	// 무기 관련 태그들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGameplayTag> WeaponTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGunDamageFalloffData> GunDamageFalloffArray;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGunRecoilData> GunRecoilMap;

	// 무기 관련 애니
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UAnimMontage* FireAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	UAnimMontage* ReloadAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UBaseWeaponAnim> GunABPClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	UNiagaraSystem* MuzzleFlashEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	UNiagaraSystem* TracerEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	UNiagaraSystem* ImpactEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	FName MuzzleSocketName = "MuzzlePoint";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
	USoundBase* FireSound = nullptr;
};

// GameplayEffectData
USTRUCT(BlueprintType)
struct FGameplayEffectData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 EffectID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString EffectName = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Duration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Magnitude = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FGameplayTag> EffectTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, float> SetByCallerMagnitudes;
};

// 스킬 데이터 테이블
USTRUCT(BlueprintType)
struct FAbilityData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic")
	int32 AbilityID = 0;

	// 기본 정보
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic")
	FString AbilityName = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic")
	FText AbilityDescription;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic")
	// EAbilitySlotType AbilitySlot = EAbilitySlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic")
	UTexture2D* AbilityIcon = nullptr;

	// 어빌리티 클래스 및 동작 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSubclassOf<UGameplayAbility> AbilityClass = nullptr;

	// 어빌리티 레벨 (강도 조절용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int32 AbilityLevel = 1;

	// 쿨다운 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	float CooldownDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass = nullptr;

	// 능력 종류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Valorant|Ability")
	//EValorantAbilityType AbilityType = EValorantAbilityType::None;
	FGameplayTag AbilityKindTag;
    
	// 능력 충전 코스트 (시그니처는 라운드당 충전 수, 궁극기는 필요 포인트, 구매 비용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Valorant|Ability")
	int32 ChargeCost = 0;

	// 충전 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charges")
	bool UsesCharges = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charges", meta = (EditCondition = "UsesCharges"))
	int32 MaxCharges = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charges", meta = (EditCondition = "UsesCharges"))
	float ChargeRegenTime = 0.0f;

	// // 비용 관련
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	// bool HasUltimateCost = false;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (EditCondition = "HasUltimateCost"))
	// int32 UltimatePointCost = 0;

	// 효과 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TArray<int> AppliedEffectIDs; // GameplayEffectData 테이블의 행 ID를 참조

	// 기타 속성들 (스킬별 특성)
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom")
	// TMap<FString, float> CustomParameters;

	// 태그 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	TArray<FGameplayTag> AbilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	TArray<FGameplayTag> ActivationBlockedTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	TArray<FGameplayTag> CancelAbilitiesWithTags;

	// 네트워크 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network")
	bool ReplicateAbility = true;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// int32 MaxStack = 2;   // 최대 스택 수 (기본값 2)
};

