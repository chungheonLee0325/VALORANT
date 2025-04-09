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
	None        UMETA(DisplayName = "None"),

	Ascent      UMETA(DisplayName = "Ascent"),
	Bind        UMETA(DisplayName = "Bind"),
	Haven       UMETA(DisplayName = "Haven"),
	Split       UMETA(DisplayName = "Split"),
	Icebox      UMETA(DisplayName = "Icebox"),
	Breeze      UMETA(DisplayName = "Breeze"),
	Fracture    UMETA(DisplayName = "Fracture"),
	Pearl       UMETA(DisplayName = "Pearl")
};

UENUM(BlueprintType)
enum class EAgentRole : uint8
{
	None        UMETA(DisplayName = "None"),

	// 에이전트 역할
	Duelist     UMETA(DisplayName = "Duelist"),
	Initiator   UMETA(DisplayName = "Initiator"),
	Sentinel    UMETA(DisplayName = "Sentinel"),
	Controller  UMETA(DisplayName = "Controller")
};

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	None        UMETA(DisplayName = "None"),

	EntryFragger   UMETA(DisplayName = "Entry Fragger"),
	Support        UMETA(DisplayName = "Support"),
	Lurker         UMETA(DisplayName = "Lurker"),
	IGL            UMETA(DisplayName = "In-Game Leader")
};



UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	None        UMETA(DisplayName = "None"),

	// 무기 카테고리
	Sidearm     UMETA(DisplayName = "Sidearm"),
	SMG         UMETA(DisplayName = "SMG"),
	Shotgun     UMETA(DisplayName = "Shotgun"),
	Rifle       UMETA(DisplayName = "Rifle"),
	Sniper      UMETA(DisplayName = "Sniper"),
	Heavy       UMETA(DisplayName = "Heavy Weapon")
};


// 무기 타입 
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None        UMETA(DisplayName = "None"),
    
	// Sidearms
	Classic     UMETA(DisplayName = "Classic"),
	Shorty      UMETA(DisplayName = "Shorty"),
	Frenzy      UMETA(DisplayName = "Frenzy"),
	Ghost       UMETA(DisplayName = "Ghost"),
	Sheriff     UMETA(DisplayName = "Sheriff"),
    
	// SMGs
	Stinger     UMETA(DisplayName = "Stinger"),
	Spectre     UMETA(DisplayName = "Spectre"),
    
	// Shotguns
	Bucky       UMETA(DisplayName = "Bucky"),
	Judge       UMETA(DisplayName = "Judge"),
    
	// Rifles
	Bulldog     UMETA(DisplayName = "Bulldog"),
	Guardian    UMETA(DisplayName = "Guardian"),
	Phantom     UMETA(DisplayName = "Phantom"),
	Vandal      UMETA(DisplayName = "Vandal"),
    
	// Snipers
	Marshal     UMETA(DisplayName = "Marshal"),
	Operator    UMETA(DisplayName = "Operator"),
    
	// Heavy Weapons
	Ares        UMETA(DisplayName = "Ares"),
	Odin        UMETA(DisplayName = "Odin")
};

// 스탯 유형 정의
UENUM(BlueprintType)
enum class EValorantStatType : uint8
{
	None                UMETA(DisplayName = "None"),
    
	// 기본 전투 스탯
	Kills               UMETA(DisplayName = "Kills"),
	Deaths              UMETA(DisplayName = "Deaths"),
	Assists             UMETA(DisplayName = "Assists"),
	HeadshotPercentage  UMETA(DisplayName = "Headshot Percentage"),
	AverageDamage       UMETA(DisplayName = "Average Damage per Round"),
	CombatScore         UMETA(DisplayName = "Combat Score"),

	// 전략적 스탯
	FirstKills          UMETA(DisplayName = "First Kills"),
	ClutchWins          UMETA(DisplayName = "Clutch Wins"),
	MultiKills          UMETA(DisplayName = "Multi-Kills"),
	EconManagement      UMETA(DisplayName = "Economy Management"),

	// 기술적 스탯
	Accuracy            UMETA(DisplayName = "Accuracy"),
	SprayControl        UMETA(DisplayName = "Spray Control"),
	UtilityUsage        UMETA(DisplayName = "Utility Usage")
};

// Struct

// AgentData
USTRUCT(BlueprintType)
struct FAgentData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 AgentID;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FString AgentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EAgentRole AgentRole;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseHealth;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseArmor;
    
	// 고유 패시브 효과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayEffect> PassiveEffectClass;
    
	// 기본 어빌리티
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<TSubclassOf<UGameplayAbility>> BasicAbilities;
    
	// 스킬 C
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_C;
    
	// 스킬 Q
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_Q;
    
	// 스킬 E
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_E;
    
	// 궁극기 X
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayAbility> Ability_X;
    
	// 에이전트 특성 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGameplayTag> AgentTags;
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 WeaponID;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	EWeaponType WeaponName;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 BaseDamage;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float FireRate;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 MagazineSize;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float ReloadTime;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	float HeadshotMultiplier;
    
	// 크레딧 비용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	int32 Cost;
    
	// 무기 대미지 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
    
	// 무기 관련 태그들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TArray<FGameplayTag> WeaponTags;
};
