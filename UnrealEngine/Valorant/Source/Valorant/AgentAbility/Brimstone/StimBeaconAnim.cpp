#include "StimBeaconAnim.h"

#include "StimBeacon.h"

void UStimBeaconAnim::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	Owner = Cast<AStimBeacon>(GetOwningActor());
	if (Owner)
	{
		Mesh = Owner->Mesh;
	}
}

void UStimBeaconAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (Owner)
	{
		State = Owner->State;
	}
}

void UStimBeaconAnim::AnimNotify_OnDeployEnded() const
{
	OnDeployEnded.Broadcast();
}

void UStimBeaconAnim::AnimNotify_OnOutroEnded() const
{
	OnOutroEnded.Broadcast();
}
