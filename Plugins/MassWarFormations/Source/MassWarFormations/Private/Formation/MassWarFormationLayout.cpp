// Copyright Epic Games, Inc. All Rights Reserved.

#include "Formation/MassWarFormationLayout.h"

namespace MassWarFormationLayout
{
	namespace
	{
		int32 GridColumns(int32 Count)
		{
			return FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(2.f * Count)));
		}
	}

	void ComputeSlotOffsets(const FMassWarFormationSettings& Settings, int32 Count, TArray<FVector2D>& OutOffsets)
	{
		OutOffsets.Reset();
		OutOffsets.Reserve(Count);
		const double Spacing = Settings.Spacing;

		switch (Settings.Shape)
		{
		case EMassWarFormationShape::Line:
			for (int32 Index = 0; Index < Count; ++Index)
			{
				OutOffsets.Add(FVector2D(0.0, (Index - (Count - 1) * 0.5) * Spacing));
			}
			break;

		case EMassWarFormationShape::Grid:
		{
			const int32 Columns = GridColumns(Count);
			const int32 Rows = FMath::DivideAndRoundUp(Count, Columns);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const int32 Row = Index / Columns;
				const int32 Column = Index % Columns;
				const int32 InRow = FMath::Min(Columns, Count - Row * Columns);
				OutOffsets.Add(FVector2D((-Row + (Rows - 1) * 0.5) * Spacing, (Column - (InRow - 1) * 0.5) * Spacing));
			}
			break;
		}

		case EMassWarFormationShape::Blob:
		default:
		{
			// Sunflower spiral: even density, roughly one unit per Spacing x Spacing of ground.
			const double Scale = Spacing / FMath::Sqrt(UE_DOUBLE_PI);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const double Radius = Scale * FMath::Sqrt(Index + 0.5);
				const double Angle = Index * 2.39996323;
				OutOffsets.Add(FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius));
			}
			break;
		}
		}
	}

	float ComputeHalfWidth(const FMassWarFormationSettings& Settings, int32 Count)
	{
		switch (Settings.Shape)
		{
		case EMassWarFormationShape::Line:
			return Settings.Spacing * Count * 0.5f;
		case EMassWarFormationShape::Grid:
			return Settings.Spacing * GridColumns(Count) * 0.5f;
		case EMassWarFormationShape::Blob:
		default:
			return Settings.Spacing * (FMath::Sqrt(Count / UE_PI) + 0.5f);
		}
	}
}
