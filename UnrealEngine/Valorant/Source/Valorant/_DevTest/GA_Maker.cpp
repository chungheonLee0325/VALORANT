

#include "GA_Maker.h"

#include "AbilitySystemComponent.h"
#include "Valorant/Player/Agent/BaseAgent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Valorant/Player/AgentPlayerState.h"


AGA_Maker::AGA_Maker()
{
	PrimaryActorTick.bCanEverTick = true;

	Box = CreateDefaultSubobject<UBoxComponent>("Box");
	SetRootComponent(Box);

	Widget = CreateDefaultSubobject<UWidgetComponent>("Widget");
	Widget->SetupAttachment(Box);
	Widget->SetVisibility(false);
}

void AGA_Maker::OnActorxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Widget->SetVisibility(true);
	
	 FTimerHandle TimerHandle;
	 GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	 {
	 	Widget->SetVisibility(false);
	 }, 2.0f, false);
	
	ABaseAgent* agent = Cast<ABaseAgent>(OtherActor);
	if (agent == nullptr)
	{
		return;
	}
	
	AAgentPlayerState* ps = Cast<AAgentPlayerState>(agent->GetPlayerState());
	if (ps == nullptr)
	{
		return;
	}
	UAbilitySystemComponent* asc = ps->GetAbilitySystemComponent();

	if (asc == nullptr)
	{
		return;
	}

	asc->TryActivateAbilityByClass(AbilityClass);
}

void AGA_Maker::BeginPlay()
{
	Super::BeginPlay();
	
	Box->OnComponentBeginOverlap.AddDynamic(this, &AGA_Maker::OnActorxBeginOverlap);
	AAgentPlayerState* ps =  Cast<AAgentPlayerState>(GetWorld()->GetFirstPlayerController()->GetPlayerState<APlayerState>());
	if (ps == nullptr)
	{
		return;
	}
	ps->GetAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
}

void AGA_Maker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

