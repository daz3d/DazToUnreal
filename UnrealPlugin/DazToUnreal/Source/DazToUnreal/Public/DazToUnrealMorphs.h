#pragma once

#include "CoreMinimal.h"
#include "DazToUnrealEnums.h"
#include "Dom/JsonObject.h"
#include "DazJointControlledMorphAnimInstance.h"

class UDazJointControlledMorphAnimInstance;
class USkeleton;
class USkeletalMesh;

class FDazToUnrealMorphs
{
public:
	// Called to create the JCM AnimInstance
	static UDazJointControlledMorphAnimInstance* CreateJointControlAnimation(TSharedPtr<FJsonObject> JsonObject, FString Folder, FString CharacterName, USkeleton* Skeleton, USkeletalMesh* Mesh);

	// Returns whether the DTU file contains data for AutoJCM
	static bool IsAutoJCMImport(TSharedPtr<FJsonObject> JsonObject);

private:

	// Internal function for creating the AnimBlueprint for the AnimInstance
	static UAnimBlueprint* CreateBlueprint(UObject* InParent, FName Name, USkeleton* Skeleton);

	// Internal function for creating the AnimBlueprint for the AnimInstance
	static void FakeDualQuarternion(FName MorphName, FName BoneName, EDazMorphAnimInstanceDriver Axis, float MinBend, float MaxBend, USkeletalMesh* Mesh);
};