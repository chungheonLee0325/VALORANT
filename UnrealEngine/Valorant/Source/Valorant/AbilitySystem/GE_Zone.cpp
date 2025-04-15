// Fill out your copyright notice in the Description page of Project Settings.


#include "GE_Zone.h"

#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Valorant/Player/AgentPlayerState.h"
#include "Valorant/Player/Agent/BaseAgent.h"


// Sets default values
AGE_Zone::AGE_Zone()
{
	PrimaryActorTick.bCanEverTick = true;
	BoxComp = CreateDefaultSubobject<UBoxComponent>("Box");
	SetRootComponent(BoxComp);

	Text = CreateDefaultSubobject<UTextRenderComponent>("EffectText");
	Text->SetupAttachment(BoxComp);
	Text->SetVisibility(false);
}

// Called when the game starts or when spawned
void AGE_Zone::BeginPlay()
{
	Super::BeginPlay();
	BoxComp->OnComponentBeginOverlap.AddDynamic(this,&AGE_Zone::OnActorxBeginOverlap);
}

void AGE_Zone::OnActorxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Text->SetVisibility(true);
	
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		Text->SetVisibility(false);
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

	FGameplayEffectContextHandle Context = asc->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = asc->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Effect적용"));
		asc->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

// Called every frame
void AGE_Zone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

