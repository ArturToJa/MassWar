// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/MassWarPuppetAnimInstance.h"
#include "Characters/MassWarUnitVisualCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarPuppetAnimInstance)

void UMassWarPuppetAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Mass pools and reuses puppets, so the owner stays the same Actor for this instance's whole life.
	Puppet = Cast<AMassWarUnitVisualCharacter>(GetOwningActor());
}

void UMassWarPuppetAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Game thread on purpose (not NativeThreadSafeUpdateAnimation): the puppet's state is written on the
	// game thread by the Mass sync processor, and copying two values is negligible next to pose evaluation.
	if (const AMassWarUnitVisualCharacter* PuppetPtr = Puppet.Get())
	{
		const FMassWarVisualState& State = PuppetPtr->GetVisualState();
		UnitSpeed = State.Speed;
		bUnitIsMoving = State.bIsMoving;
		bUnitIsDead = State.bIsDead;
		UnitDeathVariant = State.DeathVariant;
	}
}
