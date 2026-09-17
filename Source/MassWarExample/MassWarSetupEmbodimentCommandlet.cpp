// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarSetupEmbodimentCommandlet.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorModule.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeCompilerLog.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "Evaluators/MassWarSTEval_FindNearestEnemyActor.h"
#include "Conditions/MassWarSTCondition_HasEnemyInRangeActor.h"
#include "Tasks/MassWarSTTask_MoveToActor.h"
#include "Tasks/MassWarSTTask_AttackTargetActor.h"
#include "Characters/MassWarUnitVisualCharacter.h"
#include "MassEntityConfigAsset.h"
#include "MassMovableVisualizationTrait.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

static void MassWarSetupEmbodiment_WireVisualPuppet()
{
	const FString AssetPath = TEXT("/Game/MassWar/DA_MassWarDemoUnit.DA_MassWarDemoUnit");
	const FString PackagePath = TEXT("/Game/MassWar/DA_MassWarDemoUnit");

	UMassEntityConfigAsset* Config = LoadObject<UMassEntityConfigAsset>(nullptr, *AssetPath);
	if (!Config)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupEmbodiment: could not load %s"), *AssetPath);
		return;
	}

	const UMassEntityTraitBase* Existing = Config->FindTrait(UMassMovableVisualizationTrait::StaticClass());
	UMassMovableVisualizationTrait* VisTrait = const_cast<UMassMovableVisualizationTrait*>(Cast<UMassMovableVisualizationTrait>(Existing));
	if (!VisTrait)
	{
		// Deliberately not adding one: DA_MassWarDemoUnit's actual visualization trait (with its already
		// configured StaticMeshInstanceDesc mesh) was set up manually in the editor back in Pass 4, and
		// guessing at a different trait class here risks creating a redundant, misconfigured duplicate.
		// Calling this out explicitly rather than silently doing nothing useful (see Pass 7 plan's own
		// "call this out to the user rather than silently degrading" note for the content-import fallback).
		UE_LOG(LogTemp, Warning, TEXT("MassWarSetupEmbodiment: %s has no UMassMovableVisualizationTrait - could not wire the near-LOD visual puppet automatically. Set its HighResTemplateActor to AMassWarUnitVisualCharacter manually in the editor."), *AssetPath);
		return;
	}

	VisTrait->HighResTemplateActor = AMassWarUnitVisualCharacter::StaticClass();

	UPackage* Package = Config->GetOutermost();
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, Config, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Warning, TEXT("MassWarSetupEmbodiment: wired near-LOD visual puppet, saved=%d path=%s"), bSaved, *PackageFileName);
}

int32 UMassWarSetupEmbodimentCommandlet::Main(const FString& Params)
{
	const FString PackagePath = TEXT("/Game/MassWar/ST_MassWarUnit_Actor");
	const FString AssetName = TEXT("ST_MassWarUnit_Actor");

	UPackage* Package = CreatePackage(*PackagePath);
	Package->FullyLoad();

	UStateTree* StateTreeAsset = NewObject<UStateTree>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);

	FStateTreeEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FStateTreeEditorModule>(TEXT("StateTreeEditorModule"));
	TNonNullSubclassOf<UStateTreeEditorData> EditorDataClass = EditorModule.GetEditorDataClass(UStateTreeAIComponentSchema::StaticClass());
	UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTreeAsset, EditorDataClass, FName(), RF_Transactional);
	StateTreeAsset->EditorData = EditorData;

	// UStateTreeAIComponent (AMassWarUnitAIController's brain component) requires this exact schema class,
	// not the more general UStateTreeComponentSchema - UStateTreeAIComponent::GetSchema() hard-codes it,
	// and UStateTreeComponent::ValidateStateTreeReference rejects a mismatched schema at load time.
	// AIControllerClass stays at its default (AAIController) - it only types the schema's generic
	// "AIController" context data slot, which none of our Actor-schema nodes bind to.
	EditorData->Schema = NewObject<UStateTreeAIComponentSchema>(EditorData, UStateTreeAIComponentSchema::StaticClass(), FName(), RF_Transactional);

	// --- Build the tree: Idle -> (enemy spotted) MoveToRange -> (arrived) Attack -> (resolved) Idle ---
	// Identical shape to ST_MassWarUnit_Mass (MassWarSetupStateTreeCommandlet), using the Actor-schema
	// node variants so the same decision logic drives an embodied Actor unit instead of a Mass entity.
	UStateTreeState& Root = EditorData->AddSubTree(FName(TEXT("Root")));

	auto& FindEnemyEval = EditorData->AddEvaluator<FMassWarSTEval_FindNearestEnemyActor>();

	UStateTreeState& IdleState = Root.AddChildState(FName(TEXT("Idle")));
	UStateTreeState& MoveState = Root.AddChildState(FName(TEXT("MoveToRange")));
	UStateTreeState& AttackState = Root.AddChildState(FName(TEXT("Attack")));

	FStateTreeTransition& IdleToMove = IdleState.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &MoveState);
	auto& IdleToMoveCond = IdleToMove.AddCondition<FMassWarSTCondition_HasEnemyInRangeActor>();
	IdleToMoveCond.GetInstanceData().Range = 50000.f;
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("bHasEnemy"), IdleToMoveCond, TEXT("bHasEnemy"));
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("DistanceToNearestEnemy"), IdleToMoveCond, TEXT("DistanceToEnemy"));

	auto& MoveTask = MoveState.AddTask<FMassWarSTTask_MoveToActor>();
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("NearestEnemyLocation"), MoveTask, TEXT("Destination"));

	MoveState.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &AttackState);

	FStateTreeTransition& MoveToAttack = MoveState.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &AttackState);
	auto& MoveToAttackCond = MoveToAttack.AddCondition<FMassWarSTCondition_HasEnemyInRangeActor>();
	MoveToAttackCond.GetInstanceData().Range = 1000.f;
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("bHasEnemy"), MoveToAttackCond, TEXT("bHasEnemy"));
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("DistanceToNearestEnemy"), MoveToAttackCond, TEXT("DistanceToEnemy"));

	auto& AttackTask = AttackState.AddTask<FMassWarSTTask_AttackTargetActor>();
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("NearestEnemy"), AttackTask, TEXT("Target"));
	AttackState.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &IdleState);

	// --- Compile ---
	FStateTreeCompilerLog Log;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTreeAsset, Log);
	if (!bCompiled)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupEmbodiment: compile failed, see log below"));
		Log.DumpToLog(LogTemp);
		return 1;
	}

	// --- Save ---
	FAssetRegistryModule::AssetCreated(StateTreeAsset);
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, StateTreeAsset, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Warning, TEXT("MassWarSetupEmbodiment: compiled=%d saved=%d path=%s"), bCompiled, bSaved, *PackageFileName);

	MassWarSetupEmbodiment_WireVisualPuppet();

	return bSaved ? 0 : 1;
}
