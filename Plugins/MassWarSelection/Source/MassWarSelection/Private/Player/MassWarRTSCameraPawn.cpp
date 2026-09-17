// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/MassWarRTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

AMassWarRTSCameraPawn::AMassWarRTSCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AMassWarRTSCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FMath::IsNearlyZero(PendingForward) || !FMath::IsNearlyZero(PendingRight))
	{
		const float HeightScale = FMath::Max(GetActorLocation().Z / 1000.f, 0.5f);
		// Negated: with the camera looking down the world +X axis, panning by raw +X/+Y felt inverted
		// to the player (W drove the view "backward" on screen) - flipping both axes matches expectations.
		const FVector Delta = (FVector::ForwardVector * -PendingForward + FVector::RightVector * -PendingRight)
			* PanSpeed * HeightScale * DeltaSeconds;
		AddActorWorldOffset(FVector(Delta.X, Delta.Y, 0.f));
	}
}

void AMassWarRTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AMassWarRTSCameraPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AMassWarRTSCameraPawn::MoveRight);
	PlayerInputComponent->BindAction(TEXT("ZoomIn"), IE_Pressed, this, &AMassWarRTSCameraPawn::ZoomIn);
	PlayerInputComponent->BindAction(TEXT("ZoomOut"), IE_Pressed, this, &AMassWarRTSCameraPawn::ZoomOut);
}

void AMassWarRTSCameraPawn::MoveForward(float Value)
{
	PendingForward = Value;
}

void AMassWarRTSCameraPawn::MoveRight(float Value)
{
	PendingRight = Value;
}

void AMassWarRTSCameraPawn::ZoomIn()
{
	FVector Location = GetActorLocation();
	Location.Z = FMath::Clamp(Location.Z - ZoomSpeed, MinHeight, MaxHeight);
	SetActorLocation(Location);
}

void AMassWarRTSCameraPawn::ZoomOut()
{
	FVector Location = GetActorLocation();
	Location.Z = FMath::Clamp(Location.Z + ZoomSpeed, MinHeight, MaxHeight);
	SetActorLocation(Location);
}
