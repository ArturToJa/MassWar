// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/MassWarWeaponDefinition.h"
#include "Engine/StaticMesh.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarWeaponDefinition)

UStaticMesh* UMassWarWeaponDefinition::LoadMesh() const
{
	return Mesh.IsNull() ? nullptr : Mesh.LoadSynchronous();
}

UNiagaraSystem* UMassWarWeaponDefinition::LoadMuzzleEffect() const
{
	return MuzzleEffect.IsNull() ? nullptr : MuzzleEffect.LoadSynchronous();
}

