#pragma once

#include <Engine/DataTable.h>
#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "UObject/ObjectMacros.h"
#include "GameplayTagContainer.h"
#include "ValorantGameType.generated.h"

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
enum class EWeaponCategory : uint8
{
	None UMETA(DisplayName = "None"),

	// 무기 카테고리
	Sidearm UMETA(DisplayName = "Sidearm"),
	SMG UMETA(DisplayName = "SMG"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Rifle UMETA(DisplayName = "Rifle"),
	Sniper UMETA(DisplayName = "Sniper"),
	Heavy UMETA(DisplayName = "Heavy Weapon")
};


// 무기 타입 
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "None"),

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

UENUM(BlueprintType)
enum class EAbilitySlotType : uint8
{
	None UMETA(DisplayName = "None"),
	Slot_C UMETA(DisplayName = "Slot C"),
	Slot_Q UMETA(DisplayName = "Slot Q"),
	Slot_E UMETA(DisplayName = "Slot E"),
	Slot_X UMETA(DisplayName = "Slot X"),
};

// Struct

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
	EAgentRole AgentRole = EAgentRole::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseArmor = 0;

	// 고유 패시브 효과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayEffect> PassiveEffectClass = nullptr;

	// 기본 어빌리티
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<TSubclassOf<UGameplayAbility>> BasicAbilities;

	// 스킬 C
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_C = nullptr;

	// 스킬 Q
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_Q = nullptr;

	// 스킬 E
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_E = nullptr;

	// 궁극기 X
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_X = nullptr;

	// 에이전트 특성 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGameplayTag> AgentTags;
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 WeaponID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EWeaponType WeaponName = EWeaponType::None;

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

	// 크레딧 비용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 Cost = 0;

	// 무기 대미지 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;

	// 무기 관련 태그들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGameplayTag> WeaponTags;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic")
	EAbilitySlotType AbilitySlot = EAbilitySlotType::None;

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

	// 충전 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charges")
	bool UsesCharges = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charges", meta = (EditCondition = "UsesCharges"))
	int32 MaxCharges = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charges", meta = (EditCondition = "UsesCharges"))
	float ChargeRegenTime = 0.0f;

	// 비용 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
	bool HasUltimateCost = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (EditCondition = "HasUltimateCost"))
	int32 UltimatePointCost = 0;

	// 효과 관련
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TArray<int> AppliedEffectIDs; // GameplayEffectData 테이블의 행 ID를 참조

	// 기타 속성들 (스킬별 특성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Custom")
	TMap<FString, float> CustomParameters;

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
};
