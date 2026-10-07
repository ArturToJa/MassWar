// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarSetupStateTreeCommandlet.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorModule.h"
#include "StateTreeEditorSchema.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeCompilerLog.h"
#include "MassStateTreeSchema.h"
#include "Evaluators/MassWarSTEval_FindNearestEnemy.h"
#include "Conditions/MassWarSTCondition_HasEnemyInRange.h"
#include "Tasks/MassWarSTTask_MoveTo.h"
#include "Tasks/MassWarSTTask_AttackTarget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

int32 UMassWarSetupStateTreeCommandlet::Main(const FString& Params)
{
	const FString PackagePath = TEXT("/Game/MassWar/ST_MassWarUnit_Mass");
	const FString AssetName = TEXT("ST_MassWarUnit_Mass");

	UPackage* Package = CreatePackage(*PackagePath);
	Package->FullyLoad();

	UStateTree* StateTreeAsset = NewObject<UStateTree>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);

	FStateTreeEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FStateTreeEditorModule>(TEXT("StateTreeEditorModule"));
	TNonNullSubclassOf<UStateTreeEditorData> EditorDataClass = EditorModule.GetEditorDataClass(UMassStateTreeSchema::StaticClass());
	UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTreeAsset, EditorDataClass, FName(), RF_Transactional);
	StateTreeAsset->EditorData = EditorData;

	EditorData->Schema = NewObject<UMassStateTreeSchema>(EditorData, UMassStateTreeSchema::StaticClass(), FName(), RF_Transactional);

	// Note: deliberately not setting EditorData->EditorSchema here - GetEditorSchemaClass() isn't
	// DLL-exported from StateTreeEditorModule (no UE_API on that declaration), so it's not linkable
	// from outside the module. EditorSchema appears to be editor-UI-only cosmetics (customizing the
	// graph editor's own behavior), not required for compiling/running the tree.

	// --- Build the tree: Idle -> (enemy spotted) MoveToRange -> (arrived) Attack -> (resolved) Idle ---
	UStateTreeState& Root = EditorData->AddSubTree(FName(TEXT("Root")));

	auto& FindEnemyEval = EditorData->AddEvaluator<FMassWarSTEval_FindNearestEnemy>();

	UStateTreeState& IdleState = Root.AddChildState(FName(TEXT("Idle")));
	UStateTreeState& MoveState = Root.AddChildState(FName(TEXT("MoveToRange")));
	UStateTreeState& AttackState = Root.AddChildState(FName(TEXT("Attack")));

	// Idle -> MoveToRange as soon as any enemy is found at all (the evaluator's own SearchRadius,
	// deliberately map-wide for now, gates whether an enemy is found in the first place).
	FStateTreeTransition& IdleToMove = IdleState.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &MoveState);
	auto& IdleToMoveCond = IdleToMove.AddCondition<FMassWarSTCondition_HasEnemyInRange>();
	IdleToMoveCond.GetInstanceData().Range = 50000.f;
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("bHasEnemy"), IdleToMoveCond, TEXT("bHasEnemy"));
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("DistanceToNearestEnemy"), IdleToMoveCond, TEXT("DistanceToEnemy"));

	// MoveToRange: walk to the enemy's last known position (straight line, no pathfinding).
	auto& MoveTask = MoveState.AddTask<FMassWarSTTask_MoveTo>();
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("NearestEnemyLocation"), MoveTask, TEXT("Destination"));

	// Fallback: if the unit actually reaches its (possibly stale, since the target may have wandered
	// off chasing its own nearest enemy) destination without ever coming into real attack range.
	MoveState.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &AttackState);

	// Primary: transition the moment the *current* nearest enemy (re-evaluated every tick, so this
	// naturally follows a moving target) comes within actual engagement range - without waiting to
	// arrive at wherever it was standing when this state was entered.
	FStateTreeTransition& MoveToAttack = MoveState.AddTransition(EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &AttackState);
	auto& MoveToAttackCond = MoveToAttack.AddCondition<FMassWarSTCondition_HasEnemyInRange>();
	MoveToAttackCond.GetInstanceData().Range = 1000.f;
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("bHasEnemy"), MoveToAttackCond, TEXT("bHasEnemy"));
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("DistanceToNearestEnemy"), MoveToAttackCond, TEXT("DistanceToEnemy"));

	// Attack: fight whatever the evaluator currently considers nearest (re-evaluated fresh on entry,
	// so a target that died while approaching is naturally replaced, or the task fails back to Idle).
	auto& AttackTask = AttackState.AddTask<FMassWarSTTask_AttackTarget>();
	EditorData->AddPropertyBinding(FindEnemyEval, TEXT("NearestEnemy"), AttackTask, TEXT("Target"));
	AttackState.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &IdleState);

	// --- Compile ---
	FStateTreeCompilerLog Log;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTreeAsset, Log);
	if (!bCompiled)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupStateTree: compile failed, see log below"));
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

	UE_LOG(LogTemp, Warning, TEXT("MassWarSetupStateTree: compiled=%d saved=%d path=%s"), bCompiled, bSaved, *PackageFileName);

	return bSaved ? 0 : 1;
}
