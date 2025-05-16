// Fill out your copyright notice in the Description page of Project Settings.


#include "KayoKnifeAnim.h"

#include "KayoKnife.h"

void UKayoKnifeAnim::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	Owner = Cast<AKayoKnife>(GetOwningActor());
	if (Owner)
	{
		Mesh = Owner->Mesh;
	}
}

void UKayoKnifeAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (Owner)
	{
		State = Owner->State;
		bIsThirdPerson = Owner->bIsThirdPerson;
	}
}
