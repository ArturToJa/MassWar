// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Pawn.h"
#include "MassWarRTSCameraPawn.generated.h"

class UCameraComponent;

/**
 * Minimal RTS-style camera: fixed downward pitch (set once on the camera component itself, not driven
 * by Controller rotation), WASD pans in the world XY plane, mouse wheel adjusts height. No physics or
 * collision - it's a free-floating camera rig, not a simulated body.
 */
UCLASS()
class MASSWARSELECTION_API AMassWarRTSCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AMassWarRTSCameraPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "MassWar Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "MassWar Camera")
	float CameraPitch = -60.f;

	UPROPERTY(EditAnywhere, Category = "MassWar Camera")
	float PanSpeed = 2000.f;

	UPROPERTY(EditAnywhere, Category = "MassWar Camera")
	float ZoomSpeed = 1500.f;

	UPROPERTY(EditAnywhere, Category = "MassWar Camera")
	float MinHeight = 500.f;

	UPROPERTY(EditAnywhere, Category = "MassWar Camera")
	float MaxHeight = 20000.f;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void ZoomIn();
	void ZoomOut();

	float PendingForward = 0.f;
	float PendingRight = 0.f;
};
