#include "DazToUnrealMorphs.h"
#include "DazJointControlledMorphAnimInstance.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "EdGraphSchema_K2_Actions.h"
#include "AnimGraphNode_LinkedInputPose.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Animation/MorphTarget.h"
#include "LODUtilities.h"
#include "Math/DualQuat.h"

UDazJointControlledMorphAnimInstance* FDazToUnrealMorphs::CreateJointControlAnimation(TSharedPtr<FJsonObject> JsonObject, FString Folder, FString CharacterName, USkeleton* Skeleton, USkeletalMesh* Mesh)
{
	// Only only create the JCM Anim if the JointLinks object exists
	const TArray<TSharedPtr<FJsonValue>>* JointLinkList;// = JsonObject->GetArrayField(TEXT("JointLinks"));
	if (JsonObject->TryGetArrayField(TEXT("JointLinks"), JointLinkList))
	{
		const FString AssetName = CharacterName + TEXT("_JCMAnim");
		const FString PackageName = FPackageName::ObjectPathToPackageName(Folder) / AssetName;
		const FString ObjectName = FPackageName::ObjectPathToObjectName(PackageName);

		// Create the new blueprint next to the SkeletalMesh and get the AnimInstance from it.
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
		UPackage* NewPackage = CreatePackage(nullptr, *PackageName);
#else
		UPackage* NewPackage = CreatePackage(*PackageName);
#endif
		UAnimBlueprint* AnimBlueprint = CreateBlueprint(NewPackage, FName(AssetName), Skeleton);
		UDazJointControlledMorphAnimInstance* AnimInstance = Cast<UDazJointControlledMorphAnimInstance>(AnimBlueprint->GetAnimBlueprintGeneratedClass()->ClassDefaultObject);

		// Get the joint link info for each link
		for (int i = 0; i < JointLinkList->Num(); i++)
		{
			FDazJointControlLink NewJointLink;
			const TSharedPtr<FJsonValue> JointLinkValue = (*JointLinkList)[i];
			TSharedPtr<FJsonObject> JointLink = JointLinkValue->AsObject();
			NewJointLink.BoneName = FName(JointLink->GetStringField(TEXT("Bone")));

			// Unreal ends up with different axis
			FString DazAxis = JointLink->GetStringField(TEXT("Axis"));
			if (DazAxis == TEXT("XRotate")) 
			{
				NewJointLink.PrimaryAxis = EDazMorphAnimInstanceDriver::RotationY;
			}
			if (DazAxis == TEXT("YRotate"))
			{
				NewJointLink.PrimaryAxis = EDazMorphAnimInstanceDriver::RotationZ;
			}
			if (DazAxis == TEXT("ZRotate"))
			{
				NewJointLink.PrimaryAxis = EDazMorphAnimInstanceDriver::RotationX;
			}

			NewJointLink.MorphName = FName(JointLink->GetStringField(TEXT("Morph")));
			NewJointLink.Scalar = JointLink->GetNumberField(TEXT("Scalar"));

			// These two are flipped when we get in Unreal
			if (NewJointLink.PrimaryAxis == EDazMorphAnimInstanceDriver::RotationY)
			{
				NewJointLink.Scalar = NewJointLink.Scalar * -1.0f;
			}
			if (NewJointLink.PrimaryAxis == EDazMorphAnimInstanceDriver::RotationZ)
			{
				NewJointLink.Scalar = NewJointLink.Scalar * -1.0f;
			}
			NewJointLink.Alpha = JointLink->GetNumberField(TEXT("Alpha"));

			// Get the Keys if they exist
			const TArray<TSharedPtr<FJsonValue>>* JointLinkKeys;
			if (JointLink->TryGetArrayField(TEXT("Keys"), JointLinkKeys))
			{
				for (int KeyIndex = 0; KeyIndex < JointLinkKeys->Num(); KeyIndex++)
				{
					FDazJointControlLinkKey NewJointLinkKey;
					const TSharedPtr<FJsonValue> JointLinkKeyValue = (*JointLinkKeys)[KeyIndex];
					TSharedPtr<FJsonObject> JointLinkKey = JointLinkKeyValue->AsObject();
					NewJointLinkKey.BoneRotation = JointLinkKey->GetNumberField(TEXT("Angle"));
					if (NewJointLink.PrimaryAxis == EDazMorphAnimInstanceDriver::RotationY)
					{
						NewJointLinkKey.BoneRotation = NewJointLinkKey.BoneRotation * -1.0f;
					}
					if (NewJointLink.PrimaryAxis == EDazMorphAnimInstanceDriver::RotationZ)
					{
						NewJointLinkKey.BoneRotation = NewJointLinkKey.BoneRotation * -1.0f;
					}
					NewJointLinkKey.MorphAlpha = JointLinkKey->GetNumberField(TEXT("Value"));
					NewJointLink.Keys.Add(NewJointLinkKey);
				}
			}
			if (false)
			{
				FakeDualQuarternion(NewJointLink.MorphName, NewJointLink.BoneName, NewJointLink.PrimaryAxis, 0.0f, 90.0f, Mesh);
			}
			AnimInstance->ControlLinks.Add(NewJointLink);
		}

#if 0
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
		FLODUtilities::RegenerateLOD(Mesh, -1, true, true);
#else
		FLODUtilities::RegenerateLOD(Mesh, nullptr, -1, true, true);
#endif
#endif

		// Setup Secondary Axis.  We need to know the primary and secondary axis 
		// because if we check the rotation in the wrong order the morphs can pop.
		for (FDazJointControlLink& PrimaryAxisLink : AnimInstance->ControlLinks)
		{
			for (FDazJointControlLink& SecondaryAxisLink : AnimInstance->ControlLinks)
			{
				if (PrimaryAxisLink.BoneName == SecondaryAxisLink.BoneName &&
					PrimaryAxisLink.PrimaryAxis != SecondaryAxisLink.PrimaryAxis)
				{
					PrimaryAxisLink.SecondaryAxis = SecondaryAxisLink.PrimaryAxis;
					SecondaryAxisLink.SecondaryAxis = PrimaryAxisLink.PrimaryAxis;
					break;
				}
			}
		}

		// Recompile the AnimBlueprint and return the updated AnimInstance
		FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		
		AnimInstance = Cast<UDazJointControlledMorphAnimInstance>(AnimBlueprint->GetAnimBlueprintGeneratedClass()->ClassDefaultObject);
		return AnimInstance;
	}
	return nullptr;
}

// This is a stripped down version of UAnimBlueprintFactory::FactoryCreateNew
UAnimBlueprint* FDazToUnrealMorphs::CreateBlueprint(UObject* InParent, FName Name, USkeleton* Skeleton)
{
	// Create the Blueprint
	UClass* ClassToUse = UDazJointControlledMorphAnimInstance::StaticClass();
	UAnimBlueprint* NewBP = CastChecked<UAnimBlueprint>(FKismetEditorUtilities::CreateBlueprint(ClassToUse, InParent, Name, BPTYPE_Normal, UAnimBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), NAME_None));

	NewBP->TargetSkeleton = Skeleton;

	// Because the BP itself didn't have the skeleton set when the initial compile occured, it's not set on the generated classes either
	if (UAnimBlueprintGeneratedClass* TypedNewClass = Cast<UAnimBlueprintGeneratedClass>(NewBP->GeneratedClass))
	{
		TypedNewClass->TargetSkeleton = Skeleton;
	}
	if (UAnimBlueprintGeneratedClass* TypedNewClass_SKEL = Cast<UAnimBlueprintGeneratedClass>(NewBP->SkeletonGeneratedClass))
	{
		TypedNewClass_SKEL->TargetSkeleton = Skeleton;
	}

	// Need to add the Pose Input Node and connect it to the Output Node in the AnimGraph
	TArray<UEdGraph*> Graphs;
	NewBP->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetFName() == UEdGraphSchema_K2::GN_AnimGraph)
		{
			UEdGraphNode* OutNode = nullptr;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				OutNode = Node;
			}
			// If the Blueprint isn't open in an Editor Window adding the UAnimGraphNode_LinkedInputPose node will crash.
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewBP);
			UAnimGraphNode_LinkedInputPose* const InputTemplate = NewObject<UAnimGraphNode_LinkedInputPose>();
			UEdGraphNode* const NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UAnimGraphNode_LinkedInputPose>(Graph, InputTemplate, FVector2D(0.0f, 0.0f), false);
			if (NewNode && OutNode)
			{
				UEdGraphPin* NextPin = NewNode->FindPin(TEXT("Pose"));
				UEdGraphPin* OutPin = OutNode->FindPin(TEXT("Result"));

				NextPin->MakeLinkTo(OutPin);
			}

			// Don't need the editor open anymore, so close it now.
			// Note: Turns out I can't close it because part of the action happens on a tick
			//GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(NewBP);
		}
	}

	return NewBP;

	
}

bool FDazToUnrealMorphs::IsAutoJCMImport(TSharedPtr<FJsonObject> JsonObject)
{
	const TArray<TSharedPtr<FJsonValue>>* JointLinkList;
	if (JsonObject->TryGetArrayField(TEXT("JointLinks"), JointLinkList))
	{
		return true;
	}
	return false;
}

void FDazToUnrealMorphs::FakeDualQuarternion(FName MorphName, FName BoneName, EDazMorphAnimInstanceDriver Axis, float MinBend, float MaxBend, USkeletalMesh* Mesh)
{
#if 0
	if (UMorphTarget* Morph = Mesh->FindMorphTarget(MorphName))
	{
#if ENGINE_MAJOR_VERSION > 4
		const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
#else
		const FReferenceSkeleton& RefSkeleton = Mesh->RefSkeleton;
#endif

		TArray<FTransform> ComponentSpaceTransforms;
		FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RefSkeleton.GetRefBonePose(), ComponentSpaceTransforms);

		int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
		//const FMeshBoneInfo& BoneInfo = RefSkeleton.GetRefBoneInfo()[BoneIndex];

		///int32 ParentBoneIndex = BoneInfo.ParentIndex;
		///const FMeshBoneInfo& ParentBoneInfo = RefSkeleton.GetRefBoneInfo()[ParentBoneIndex];
		///FName ParentBoneName = ParentBoneInfo.Name;

		FSkeletalMeshImportData ImportData;
		Mesh->LoadLODImportedData(0, ImportData);

		/*TMap<int32, float> ParentBoneVertexToWeight;
		TMap<int32, float> ChildBoneVertexToWeight;

		for (SkeletalMeshImportData::FRawBoneInfluence Influence : ImportData.Influences)
		{
			if (Influence.BoneIndex == ParentBoneIndex)
			{
				ParentBoneVertexToWeight.Add(Influence.VertexIndex, Influence.Weight);
			}
			if (Influence.BoneIndex == BoneIndex)
			{
				ChildBoneVertexToWeight.Add(Influence.VertexIndex, Influence.Weight);
			}
		}*/

		// Get the Bone's component position
		//const FMeshBoneInfo& RecurseBone = BoneInfo;
		/*int32 RecurseBoneIndex = BoneIndex;
		FVector BoneComponentPosition = RefSkeleton.GetRefBonePose()[BoneIndex].GetTranslation();
		while (RecurseBoneIndex != 0)
		{
			const FMeshBoneInfo& RecurseBoneInfo = RefSkeleton.GetRefBoneInfo()[RecurseBoneIndex];
			BoneComponentPosition += RefSkeleton.GetRefBonePose()[RecurseBoneInfo.ParentIndex].GetTranslation();
			RecurseBoneIndex = RecurseBoneInfo.ParentIndex;
		}*/

		int32 ImportMorphIndex = 0;
		for (FSkeletalMeshImportData& ImportMorphData : ImportData.MorphTargets)
		{
			FName ImportMorphName = FName(ImportData.MorphTargetNames[ImportMorphIndex]);
			if (ImportMorphName == MorphName)
			{
				// Setup the Rotation Info
				FVector RotationVector = FVector(0.0f, 0.0f, 1.0f);
				float Direction = 1.0f;
				if (Axis == EDazMorphAnimInstanceDriver::RotationZ)
				{
					RotationVector = FVector(0.0f, 0.0f, 1.0f);
				}

				if (Axis == EDazMorphAnimInstanceDriver::RotationY)
				{
					RotationVector = FVector(0.0f, 1.0f, 0.0f);
					//Direction = -1.0f;
				}
				FTransform RotationTransform = FTransform(FQuat(RotationVector, FMath::DegreesToRadians(90.0f * Direction)));

				//for (int32 ModifiedPointIndex = 0; ModifiedPointIndex < ImportData.MorphTargetModifiedPoints[ImportMorphIndex].Num(); ModifiedPointIndex++)
				int32 ModifiedPointIndex = 0;
				for (uint32 ModifiedPointVertexIndex : ImportData.MorphTargetModifiedPoints[ImportMorphIndex])
				{
					//auto VertexSet = ImportData.MorphTargetModifiedPoints[ImportMorphIndex];
					//FSetElementId SetId = VertexSet[FSetElementId::FromInteger(ModifiedPointVertexIndex)];//.FindId(ModifiedPointIndex);
					//uint32 ModifiedPointVertexIndex = VertexSet[FSetElementId::FromInteger(ModifiedPointVertexIndex)];

					// Find all the bones influencing this vertex
					TMap<int32, float> BoneIndexToWeight;
					for (SkeletalMeshImportData::FRawBoneInfluence Influence : ImportData.Influences)
					{
						if (Influence.VertexIndex == ModifiedPointVertexIndex)
						{
							BoneIndexToWeight.Add(Influence.BoneIndex, Influence.Weight);
						}
					}

					// Get the Linear Blend Skinning for the vertex
					FVector PointPosition = ImportData.Points[ModifiedPointVertexIndex];
					FVector LinearBlendPostion = FVector(0.0f, 0.0f, 0.0f);
					float WeightSum = 0.0f;
					FTransform LinearBlendTransform = FTransform::Identity;
					FDualQuat SummedDualQuat(FTransform::Identity);
					// Sum up from each influence
					for (TPair<int32, float> BoneToWeightPair : BoneIndexToWeight)
					{
						//FTransform BoneRefTransform = ComponentSpaceTransforms[BoneToWeightPair.Key];
						const FMeshBoneInfo& BoneInfo = RefSkeleton.GetRefBoneInfo()[BoneToWeightPair.Key];
						FTransform BoneWorldTransform = ComponentSpaceTransforms[BoneToWeightPair.Key];
						float BoneWeight = BoneToWeightPair.Value;

						//const FMeshBoneInfo& ParentBoneInfo = RefSkeleton.GetRefBoneInfo()[BoneInfo.ParentIndex];
						FTransform ParentWorldTransform = ComponentSpaceTransforms[BoneInfo.ParentIndex];
						FTransform BoneTransformInParentSpace = BoneWorldTransform * ParentWorldTransform.Inverse();
						FVector VertexPositionInBoneSpace = PointPosition - BoneWorldTransform.GetTranslation();
						FTransform VertexTransformInBoneSpace = FTransform::Identity; //BoneTransformInParentSpace;
						VertexTransformInBoneSpace.AddToTranslation(VertexPositionInBoneSpace);
						FTransform RotatedBoneTransform = BoneWorldTransform;

						//FVector RigidSkinningPosition = BoneRefTransform.TransformPosition(VertexPositionInBoneSpace);//PointPosition;
						if (BoneToWeightPair.Key >= BoneIndex)
						{
							VertexTransformInBoneSpace = VertexTransformInBoneSpace * RotationTransform;
							VertexPositionInBoneSpace = VertexTransformInBoneSpace.GetTranslation();
							//RotatedBoneTransform = BoneWorldTransform * RotationTransform;
							//FTransform ParentBoneTransform = ComponentSpaceTransforms[ParentBoneIndex];
							//FTransform BoneTransformInParentSpace = BoneRefTransform * ParentBoneTransform.Inverse();
							//FTransform RotatedBonTransformInParentSpace = BoneTransformInParentSpace * RotationTransform;
							//FTransform BoneTransformInComponentSpace = RotatedBonTransformInParentSpace * ParentBoneTransform;
							//FTransform BoneTransformInComponentSpace = RotationTransform * BoneRefTransform;

							//RigidSkinningPosition = BoneTransformInComponentSpace.TransformPosition(VertexPositionInBoneSpace);
							//LinearBlendPostion = RigidSkinningPosition;
							//BoneTransform = BoneTransform * ParentBoneTransform.Inverse() * RotationTransform * ParentBoneTransform;
							//FTransform RotationTransform = FTransform(FQuat(RotationVector, FMath::DegreesToRadians(90.0f * Direction)));
							//RigidSkinningPosition = VertexPositionInBoneSpace.RotateAngleAxis(90.0f * Direction, RotationVector);
						}
						FTransform VertexWorldTransform = FTransform(VertexTransformInBoneSpace.GetTranslation()) * FTransform(BoneWorldTransform.GetTranslation());// ParentWorldTransform;
						//FTransform VertexWorldTransform = BoneWorldTransform * VertexTransformInBoneSpace;
						FTransform BoneTransformInVertexSpace = VertexTransformInBoneSpace.Inverse();

						//FVector V = VertexWorldTransform;
						//FQuat Real = TransformForDualQuat.GetRotation();
						//FQuat Dual = FQuat(V.X, V.Y, V.Z, 0.f) * Real * 0.5f;
						FDualQuat DualQuatBoneTransform = FDualQuat(VertexWorldTransform);
						//LinearBlendPostion = RotatedBoneTransform * BoneWeight;
						LinearBlendTransform.Accumulate(VertexWorldTransform, ScalarRegister(BoneWeight));
						SummedDualQuat = SummedDualQuat * (DualQuatBoneTransform * BoneWeight);
						// Sum the weighted Rigid Skinning to get the Linear Blend Skinning
						//WeightSum += BoneWeight;
						//LinearBlendPostion += RigidSkinningPosition * BoneWeight;
					}

					// Weight sum should always be 1.0, but just in case;
					//LinearBlendPostion = (LinearBlendPostion / WeightSum);// +PointPosition;

					// Get the Dual Quaternion Skinning

					// Sum up from each influence
					if (0)
					{
						int32 QuatCount = 0;
						FVector TestVector = FVector(0.0f, 0.0f, 0.0f);
						for (TPair<int32, float> BoneToWeightPair : BoneIndexToWeight)
						{
							const FMeshBoneInfo& BoneInfo = RefSkeleton.GetRefBoneInfo()[BoneToWeightPair.Key];
							FTransform BoneRefTransform = ComponentSpaceTransforms[BoneToWeightPair.Key];
							//FTransform RotationTransform = FTransform(FQuat(RotationVector, FMath::DegreesToRadians(90.0f * Direction)));
							FVector VertexPositionInBoneSpace = PointPosition - BoneRefTransform.GetTranslation();
							FVector BonePositionInVertexSpace = BoneRefTransform.GetTranslation() - PointPosition;
							FVector RigidSkinningPosition = BoneRefTransform.TransformPosition(VertexPositionInBoneSpace);//PointPosition;

							if (BoneToWeightPair.Key == BoneIndex)
							{
								////FTransform ParentBoneTransform = ComponentSpaceTransforms[ParentBoneIndex];
								////BoneTransform = BoneTransform * ParentBoneTransform.Inverse() * RotationTransform * ParentBoneTransform;
								//FTransform BoneTransformInComponentSpace = RotationTransform * BoneTransform;
								////BoneTransform = BoneTransformInComponentSpace;
								////BoneTransform = BoneTransform * RotationTransform;
								////FDualQuat DualQuatBoneTransform = FDualQuat(BoneTransform);
								//FTransform RotationTransform = FTransform(FQuat(RotationVector, FMath::DegreesToRadians(90.0f * Direction)));
								FTransform ParentBoneTransform = ComponentSpaceTransforms[BoneInfo.ParentIndex];
								FTransform BoneTransformInParentSpace = BoneRefTransform * ParentBoneTransform.Inverse();
								FTransform RotatedBonTransformInParentSpace = BoneTransformInParentSpace * RotationTransform;
								//FTransform BoneTransformInComponentSpace = RotatedBonTransformInParentSpace * ParentBoneTransform;
								FTransform BoneTransformInComponentSpace = RotationTransform * BoneRefTransform;

								//RigidSkinningPosition = BoneTransformInComponentSpace.TransformPosition(VertexPositionInBoneSpace);
								//LinearBlendPostion = RigidSkinningPosition;
								FTransform BoneTransformInVertexSpace = FTransform::Identity * FTransform(BonePositionInVertexSpace);
								//FTransform TransformForDualQuat = BoneTransformInParentSpace * RotationTransform * ParentBoneTransform * BoneTransformInVertexSpace.Inverse();
								FTransform TransformForDualQuat = RotationTransform * BoneRefTransform; //BoneTransformInParentSpace * RotationTransform * BoneTransformInVertexSpace.Inverse();
								TestVector = TransformForDualQuat.TransformPosition(VertexPositionInBoneSpace);

								//FVector V = TransformForDualQuat.GetTranslation();
								FVector V = TransformForDualQuat.TransformPosition(VertexPositionInBoneSpace);
								FQuat Real = TransformForDualQuat.GetRotation();
								FQuat Dual = FQuat(V.X, V.Y, V.Z, 0.f) * Real * 0.5f;
								FDualQuat DualQuatBoneTransform = FDualQuat(Real, Dual);
								SummedDualQuat = DualQuatBoneTransform;
							}
							float BoneWeight = BoneToWeightPair.Value;

							//FDualQuat DualQuatBoneTransform = FDualQuat(BoneTransform);

							//FDualQuat WeightedDualQuatBoneTransform = DualQuatBoneTransform * BoneWeight;
							/*if (QuatCount == 0)
							{
								SummedDualQuat = WeightedDualQuatBoneTransform;
							}
							else
							{*/
							//SummedDualQuat = SummedDualQuat * WeightedDualQuatBoneTransform;// .Normalized();

							//}
							QuatCount++;
						}
					}
					LinearBlendTransform.NormalizeRotation();
					SummedDualQuat = SummedDualQuat.Normalized();
					FTransform DualQuatTransform = SummedDualQuat.AsFTransform();
					FVector DualQuaternionPosition = DualQuatTransform.TransformPosition(FVector(0.0f, 0.0f, 0.0f));//DualQuatTransform.TransformPosition(PointPosition);

					FVector Diff = FVector(DualQuaternionPosition.X - LinearBlendPostion.X,
						DualQuaternionPosition.Y - LinearBlendPostion.Y,
						DualQuaternionPosition.Z - LinearBlendPostion.Z);

					UE_LOG(LogTemp, Warning, TEXT("Diff: %f, %f, %f"), Diff.X, Diff.Y, Diff.Z);

					ImportMorphData.Points[ModifiedPointIndex] = LinearBlendTransform.GetTranslation();// +PointPosition;//ImportMorphData.Points[ModifiedPointIndex] + Diff;
					ModifiedPointIndex++;
				}
				//	//auto VertexSet = ImportData.MorphTargetModifiedPoints[ImportMorphIndex];
				//	//FSetElementId SetId = VertexSet[ModifiedPointIndex];//.FindId(ModifiedPointIndex);
				//	//uint32 ModifiedPointVertexIndex = VertexSet[SetId];
				//	//uint32 ModifiedPointVertexIndex = VertexSet[ModifiedPointIndex];

				//	if (ParentBoneVertexToWeight.Contains(ModifiedPointVertexIndex) && ChildBoneVertexToWeight.Contains(ModifiedPointVertexIndex))
				//	{
				//		float ParentWeight = ParentBoneVertexToWeight[ModifiedPointVertexIndex];
				//		float ChildWeight = ChildBoneVertexToWeight[ModifiedPointVertexIndex];
				//		FVector PointPosition = ImportData.Points[ModifiedPointVertexIndex];
				//		FVector BonePosition =  RefSkeleton.GetRefBonePose()[BoneIndex].GetTranslation();

				//		FVector Diff = FVector(0.0f, 0.0f, 0.0f);
				//		FVector RotationVector = FVector(0.0f, 0.0f, 1.0f);
				//		float Direction = 1.0f;
				//		if (Axis == EDazMorphAnimInstanceDriver::RotationZ)
				//		{
				//			RotationVector = FVector(0.0f, 0.0f, 1.0f);
				//		}

				//		if (Axis == EDazMorphAnimInstanceDriver::RotationY)
				//		{
				//			RotationVector = FVector(0.0f, 1.0f, 0.0f);
				//			//Direction = -1.0f;
				//		}

				//		FVector RelativePointPosition = PointPosition - BoneComponentPosition;
				//		FVector RotatedPosition = RelativePointPosition.RotateAngleAxis(90.0f * Direction, RotationVector);
				//		FVector RelativeRotationPosition = /*(RotatedPosition - RelativePointPosition)*/ RotatedPosition  * ParentBoneVertexToWeight[ModifiedPointVertexIndex];

				//		FQuat DualQuatRotation = FQuat(RotationVector, FMath::DegreesToRadians(90.0f * Direction));
				//		//FQuat DualQuatDisplacement = FQuat(RelativePointPosition.X / 2.0f, RelativePointPosition.Y / 2.0f, RelativePointPosition.Z / 2.0f, 0.0f);
				//		//FQuat DualQuatDisplacement = FQuat(RelativePointPosition.X, RelativePointPosition.Y, RelativePointPosition.Z, 0.0f);
				//		FQuat DualQuatDisplacement = FQuat(RelativePointPosition.X, RelativePointPosition.Y, RelativePointPosition.Z, 0.0f) * DualQuatRotation * 0.5f;
				//		FDualQuat DualQuat = FDualQuat(DualQuatRotation, DualQuatDisplacement);
				//		//DualQuat.AsFTransform().TransformVector
				//		DualQuat = DualQuat * ParentBoneVertexToWeight[ModifiedPointVertexIndex];
				//		FVector DualQuatRelativePostion = DualQuat.AsFTransform().TransformVector(RelativePointPosition);
				//		/*Diff = FVector(DualQuatRelativePostion.X - RelativePointPosition.X,
				//			DualQuatRelativePostion.Y - RelativePointPosition.Y,
				//			DualQuatRelativePostion.Z - RelativePointPosition.Z);*/

				//		Diff = FVector(RelativePointPosition.X - DualQuatRelativePostion.X,
				//			RelativePointPosition.Y - DualQuatRelativePostion.Y,
				//			RelativePointPosition.Z - DualQuatRelativePostion.Z);

				//		UE_LOG(LogTemp, Warning, TEXT("Diff: %f, %f, %f"), Diff.X, Diff.Y, Diff.Z);

				//		ImportMorphData.Points[ModifiedPointIndex] = ImportMorphData.Points[ModifiedPointIndex] + Diff;//FVector(100.0f, 0.0f, 0.0f);
				//	}
				//	ModifiedPointIndex++;
				//	
				//}
			}
			ImportMorphIndex++;
		}
		Mesh->SaveLODImportedData(0, ImportData);
	}
#endif
}