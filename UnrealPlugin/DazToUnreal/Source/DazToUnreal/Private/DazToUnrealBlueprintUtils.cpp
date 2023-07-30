#include "DazToUnrealBlueprintUtils.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
#include "IKRigDefinition.h"
#endif
#include "Animation/Skeleton.h"
#include "ReferenceSkeleton.h"

UDazToUnrealBlueprintUtils::UDazToUnrealBlueprintUtils(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

TArray<FName> UDazToUnrealBlueprintUtils::GetBoneList(const UObject* IKRigObject)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
	if (const UIKRigDefinition* IKRig = Cast<UIKRigDefinition>(IKRigObject))
	{
		return IKRig->GetSkeleton().BoneNames;
	}
#endif
	return TArray<FName>();
}


FName UDazToUnrealBlueprintUtils::GetChildBone(const USkeleton* Skeleton, FName ParentBone)
{
	if (Skeleton)
	{
		int32 ParentBoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(ParentBone);
		if (ParentBoneIndex != INDEX_NONE)
		{
			TArray<int32> ChildBoneIndexes;
			Skeleton->GetChildBones(ParentBoneIndex, ChildBoneIndexes);
			for (int32 ChildBoneIndex : ChildBoneIndexes)
			{
				return Skeleton->GetReferenceSkeleton().GetBoneName(ChildBoneIndex);
			}
		}
	}
	return NAME_None;
}