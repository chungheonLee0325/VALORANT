// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MatchPlayerController.h"
#include "Component/ShopComponent.h"
#include "AgentPlayerController.generated.h"

class UBaseAttributeSet;
class UAgentAbilitySystemComponent;
class UMatchMapHUD;
class UMiniMapWidget;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged_PC, float, newHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChanged_PC, float, newMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmorChanged_PC, float, newArmor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectSpeedChanged_PC, float, newSpeed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnServerPurchaseResult, bool, bSuccess, int32, ItemID, EShopItemType, ItemType, const FString&, FailureReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChangedAmmo, bool, bDisplayWidget, int, MagazineAmmo, int, SpareAmmo);

UCLASS()
class VALORANT_API AAgentPlayerController : public AMatchPlayerController
{
	GENERATED_BODY()

public:
	AAgentPlayerController();
	
	UMatchMapHUD* GetAgentWidget() const { return AgentWidget; }
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Widget")
	TSubclassOf<UUserWidget> AgentWidgetClass;

	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged_PC OnHealthChanged_PC;
	UPROPERTY(BlueprintAssignable)
	FOnMaxHealthChanged_PC OnMaxHealthChanged_PC;
	UPROPERTY(BlueprintAssignable)
	FOnArmorChanged_PC OnArmorChanged_PC;
	UPROPERTY(BlueprintAssignable)
	FOnEffectSpeedChanged_PC OnEffectSpeedChanged_PC;
	UPROPERTY()
	UShopComponent* ShopComponent;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FOnServerPurchaseResult OnServerPurchaseResult;
	
	UPROPERTY(BlueprintAssignable)
	FOnChangedAmmo OnChangedAmmo;
	void NotifyChangedAmmo(const bool bDisplayWidget, const int MagazineAmmo, const int SpareAmmo) const;
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RequestShopUI();

	// 상점 UI 열기 요청 
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RequestOpenShopUI();

	// 상점 UI 닫기 요청
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RequestCloseShopUI();

	// 무기 구매 요청
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestPurchaseWeapon(int32 WeaponID);

	// 서버에서 실행될 무기 구매 함수
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPurchaseWeapon(int32 WeaponID);

	// 클라이언트에서 호출 -> 서버로 스킬 구매 요청
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestPurchaseAbility(int AbilityID);

	// 서버에서 실행될 실제 구매 요청 함수 (위 함수 내부에서 호출)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPurchaseAbility(int AbilityID);

	// 방어구 구매 요청
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestPurchaseArmor(int32 ArmorLevel);

	// 서버에서 실행될 방어구 구매 함수 
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPurchaseArmor(int32 ArmorID);

	UFUNCTION(Client, Reliable)
	void Client_EnterSpectatorMode();

	UFUNCTION()
	UAgentAbilitySystemComponent* GetCacehdASC() { return CachedASC; }
	UFUNCTION()
	UBaseAttributeSet* GetCachedABS() { return CachedABS; }

	// 상점 UI 관련 기능
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UMatchMapShopUI> ShopUIClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UMatchMapShopUI* ShopUI;

	// 상점 UI 열기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenShopUI();

	// 상점 UI 닫기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseShopUI();

	// 상점 가용성 변경 이벤트 핸들러
	UFUNCTION()
	void OnShopAvailabilityChanged();

	UFUNCTION(Client, Reliable)
	void Client_ReceivePurchaseResult(bool bSuccess, int32 ItemID, EShopItemType ItemType, const FString& FailureReason);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UValorantGameInstance* m_GameInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAgentAbilitySystemComponent* CachedASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBaseAttributeSet* CachedABS = nullptr ;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMatchMapHUD* AgentWidget;
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void InitGAS();
	UFUNCTION()
	void InitAgentWidget();
	
	UFUNCTION()
	void HandleHealthChanged(float NewHealth);
	UFUNCTION()
	void HandleMaxHealthChanged(float NewMaxHealth);
	UFUNCTION(BlueprintCallable)
	void HandleArmorChanged(float NewArmor);
	UFUNCTION()
	void HandleEffectSpeedChanged(float NewSpeed);
	
	// 크레딧 관련 위젯 바인딩
	UFUNCTION()
	void BindCreditWidgetDelegate();



	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
	//             CYT             ♣
	//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
public:
	// 미니맵 위젯 클래스
	// 에디터에서 기본값만 편집 가능하고 블루프린트에서 읽기만 가능한 속성
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	// 사용할 미니맵 위젯의 클래스 (블루프린트에서 설정)
	TSubclassOf<UMiniMapWidget> MinimapWidgetClass;
    
	// 미니맵 위젯 인스턴스
	// 블루프린트에서 읽기만 가능한 속성
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "UI")
	UMiniMapWidget* MinimapWidget;
    
	// 게임 시작 시 미니맵 초기화
	// 미니맵을 초기화하는 함수
	void InitializeMinimap();

	UMiniMapWidget* GetMinimapWidget() const { return MinimapWidget; }
	virtual void OnRep_Pawn() override;

	UFUNCTION()
	void OnMatchEnd(const bool bBlueWin);
};
