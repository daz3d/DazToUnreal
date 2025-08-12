#include "DazToUnrealFbx.h"

#include "DazToUnrealSettings.h"
#include "DazToUnrealUtils.h"

void FDazToUnrealFbx::RenameDuplicateBones(FbxNode* RootNode)
{
	TMap<FString, int> ExistingBones;
	RenameDuplicateBones(RootNode, ExistingBones);
}

void FDazToUnrealFbx::RenameDuplicateBones(FbxNode* RootNode, TMap<FString, int>& ExistingBones)
{
	if (RootNode == nullptr) return;

	FbxNodeAttribute* Attr = RootNode->GetNodeAttribute();
	if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		FString BoneName = UTF8_TO_TCHAR(RootNode->GetName());
		if (ExistingBones.Contains(BoneName))
		{
			ExistingBones[BoneName] += 1;
			BoneName = FString::Printf(TEXT("%s_RENAMED_%d"), *BoneName, ExistingBones[BoneName]);
			RootNode->SetName(TCHAR_TO_UTF8(*BoneName));
		}
		else
		{
			ExistingBones.Add(BoneName, 1);
		}
	}

	for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode * ChildNode = RootNode->GetChild(ChildIndex);
		RenameDuplicateBones(ChildNode, ExistingBones);
	}
}

void FDazToUnrealFbx::FixClusterTranformLinks(FbxScene* Scene, FbxNode* RootNode)
{
	FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(RootNode->GetMesh());

	// Create missing weights
	if (NodeGeometry)
	{

		for (int DeformerIndex = 0; DeformerIndex < NodeGeometry->GetDeformerCount(); ++DeformerIndex)
		{
			FbxSkin* Skin = static_cast<FbxSkin*>(NodeGeometry->GetDeformer(DeformerIndex));
			if (Skin)
			{
				for (int ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ++ClusterIndex)
				{
					// Get the current tranform
					FbxAMatrix Matrix;
					FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
					Cluster->GetTransformLinkMatrix(Matrix);

					// Update the rotation
					FbxDouble3 Rotation = Cluster->GetLink()->PostRotation.Get();
					Matrix.SetR(Rotation);
					Cluster->SetTransformLinkMatrix(Matrix);
				}
			}
		}
	}

	for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
		FixClusterTranformLinks(Scene, ChildNode);
	}
}
void FDazToUnrealFbx::RemoveBindPoses(FbxScene* Scene)
{
	for (int32 PoseIndex = Scene->GetPoseCount() - 1; PoseIndex >= 0; --PoseIndex)
	{
		Scene->RemovePose(PoseIndex);
	}
}

void FDazToUnrealFbx::AddWeightsToAllNodes(FbxNode* Parent)
{
	//for (int ChildIndex = Parent->GetChildCount() - 1; ChildIndex >= 0; --ChildIndex)
	{
		FbxNode* ChildNode = Parent;//Parent->GetChild(ChildIndex);
		//RootBone->AddChild(ChildNode);
		FString ChildName = UTF8_TO_TCHAR(ChildNode->GetName());
		UE_LOG(LogTemp, Warning, TEXT("ChildNode %s Checking Weights"), *ChildName);

		if (FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(ChildNode->GetMesh()))
		{
			UE_LOG(LogTemp, Warning, TEXT("  No Deformers"), *ChildName);
			if (NodeGeometry->GetDeformerCount() == 0)
			{
				FbxCluster* Cluster = FbxCluster::Create(Parent->GetScene(), "");
				Cluster->SetLink(Parent);
				Cluster->SetLinkMode(FbxCluster::eTotalOne);

				FbxSkin* Skin = FbxSkin::Create(Parent->GetScene(), "");
				Skin->AddCluster(Cluster);
				NodeGeometry->AddDeformer(Skin);

				for (int PolygonIndex = 0; PolygonIndex < ChildNode->GetMesh()->GetPolygonCount(); ++PolygonIndex)
				{
					for (int VertexIndex = 0; VertexIndex < ChildNode->GetMesh()->GetPolygonSize(PolygonIndex); ++VertexIndex)
					{
						int Vertex = ChildNode->GetMesh()->GetPolygonVertex(PolygonIndex, VertexIndex);
						Cluster->AddControlPointIndex(Vertex, 1.0f);
					}
				}
			}
		}

		//AddWeightsToAllNodes(ChildNode);
	}


}

FString FDazToUnrealFbx::GetObjectNameForMaterial(FbxSurfaceMaterial* Material)
{
	FbxScene* Scene = Material->GetScene();

	for (int32 MeshIndex = Scene->GetGeometryCount() - 1; MeshIndex >= 0; --MeshIndex)
	{
		FbxGeometry* Geometry = Scene->GetGeometry(MeshIndex);
		FbxNode* GeometryNode = Geometry->GetNode();
		int32 MaterialCount = GeometryNode->GetMaterialCount();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; MaterialIndex++)
		{
			FbxSurfaceMaterial* NodeMaterial = GeometryNode->GetMaterial(MaterialIndex);
			if (NodeMaterial == Material)
			{
				FString ObjectName = UTF8_TO_TCHAR(Geometry->GetName());
				return ObjectName;
			}
		}
	}

	return FString();
}

// Some accesories attached in ways like using the DzRigidFollowNode become additional meshes.
// This function attached them to the skeleton of the primary mesh so the don't break the skeleton.
void FDazToUnrealFbx::ParentAdditionalSkeletalMeshes(FbxScene* Scene)
{
	FbxNode* RootNode = Scene->GetRootNode();

	// Find the root bone.  There should only be one bone off the scene root
	FbxNode* RootBone = nullptr;
	FString RootBoneName = TEXT("");
	for (int RootNodeIndex = Scene->GetNodeCount() -1; RootNodeIndex >= 0; --RootNodeIndex)
	{
		FbxNode* OtherRootNode = Scene->GetNode(RootNodeIndex);

		if (OtherRootNode != RootNode)
		{
			if (FbxSkeleton* OtherRootNodeSkeleton = OtherRootNode->GetSkeleton())
			{
				OtherRootNodeSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
			}
			else if(OtherRootNode->GetMesh() == nullptr)
			{
				FbxSkeleton* SkeletonAttribute = FbxSkeleton::Create(Scene, OtherRootNode->GetName());
				SkeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
				OtherRootNode->SetNodeAttribute(SkeletonAttribute);
			}
		}
	}
}


// Takes twist bones "out of line".  G3 and G8 have twist bones between some joints like thigh and knee.
void FDazToUnrealFbx::FixTwistBones(FbxNode* Node)
{
	if (Node == nullptr) return;

	// Process Children first since they'll get reparented
	for (int ChildIndex = Node->GetChildCount() - 1; ChildIndex >= 0; --ChildIndex)
	{
		FbxNode* ChildNode = Node->GetChild(ChildIndex);
		FixTwistBones(ChildNode);
	}

	FbxNodeAttribute* Attr = Node->GetNodeAttribute();
	if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		FString BoneName = UTF8_TO_TCHAR(Node->GetName());
		if (BoneName.Contains(TEXT("twist")))
		{
			for (int ChildIndex = Node->GetChildCount() - 1; ChildIndex >= 0; --ChildIndex)
			{
				FbxNode* ChildNode = Node->GetChild(ChildIndex);
				if (Node->GetParent())
				{
					Node->RemoveChild(ChildNode);
					Node->GetParent()->AddChild(ChildNode);
				}
			}
		}
	}
}

// Removes a node and re-parents it's children to it's current parent
void FDazToUnrealFbx::RemoveNodeAndReparent(FbxNode* NodeToRemove)
{
	if (FbxNode* ParentNode = NodeToRemove->GetParent())
	{
		for (int ChildIndex = NodeToRemove->GetChildCount() - 1; ChildIndex >= 0; --ChildIndex)
		{
			FbxNode* ChildNode = NodeToRemove->GetChild(ChildIndex);
			NodeToRemove->RemoveChild(ChildNode);
			ParentNode->AddChild(ChildNode);
		}
		ParentNode->RemoveChild(NodeToRemove);
	}
}

// Recursive Function to count bones in an FbxNode
int FDazToUnrealFbx::CountBonesInFbxNode(FbxNode* Node) {
	if (!Node) return 0;

	int boneCount = 0;
	if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
		boneCount++;
	}
	for (int i = 0; i < Node->GetChildCount(); i++) {
		boneCount += CountBonesInFbxNode(Node->GetChild(i));
	}
	return boneCount;
}
void FDazToUnrealFbx::ConvertToEpicSkeleton(FbxScene* Scene)
{
	TArray<FString> NodesToRemove;
	TMap<FString, FString> NodesToAdd;
	TMap<FString, FString> NodesToRename;

	NodesToRemove.Add(TEXT("pelvis"));

	// Add this under that
	NodesToAdd.Add(TEXT("spine_04"), TEXT("spine3"));

	NodesToRename.Add(TEXT("hip"), TEXT("pelvis"));




}


FbxNode* FDazToUnrealFbx::FindRootBone(FString &RootBoneName, FbxNode *RootNode, FbxScene *Scene, 
	DazAssetType AssetType, const UDazToUnrealSettings* CachedSettings, FString& AssetName)
{
	FbxNode* RootBone = nullptr;

	// DB 2025-06-11 added to arguments
	//	FString RootBoneName = TEXT("");
	for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
		FbxNodeAttribute* Attr = ChildNode->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			RootBone = ChildNode;
			RootBoneName = UTF8_TO_TCHAR(RootBone->GetName());
			RootBone->SetName(TCHAR_TO_UTF8(TEXT("root")));
			Attr->SetName(TCHAR_TO_UTF8(TEXT("root")));
			break;
		}
	}

	// Daz characters sometimes have additional skeletons inside the character for accesories
	if (AssetType == DazAssetType::SkeletalMesh)
	{
		FDazToUnrealFbx::ParentAdditionalSkeletalMeshes(Scene);
	}

	// Daz Studio puts the base bone rotations in a different place than Unreal expects them.
	if (CachedSettings->FixBoneRotationsOnImport && AssetType == DazAssetType::SkeletalMesh && RootBone)
	{
		FDazToUnrealFbx::RemoveBindPoses(Scene);
		FDazToUnrealFbx::FixClusterTranformLinks(Scene, RootBone);
	}

	// If this is a skeleton mesh, but a root bone wasn't found, it may be a scene under a group node or something similar
	// So create a root node.
	if (AssetType == DazAssetType::SkeletalMesh && RootBone == nullptr)
	{
		RootBoneName = AssetName;

		FbxSkeleton* NewRootNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("root")));
		NewRootNodeAttribute->SetSkeletonType(FbxSkeleton::eRoot);
		NewRootNodeAttribute->Size.Set(1.0);
		RootBone = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("root")));
		RootBone->SetNodeAttribute(NewRootNodeAttribute);
		RootBone->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));


		for (int ChildIndex = RootNode->GetChildCount() - 1; ChildIndex >= 0; --ChildIndex)
		{
			FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
			RootBone->AddChild(ChildNode);
			if (FbxSkeleton* ChildSkeleton = ChildNode->GetSkeleton())
			{
				if (ChildSkeleton->GetSkeletonType() == FbxSkeleton::eRoot)
				{
					ChildSkeleton->SetSkeletonType(FbxSkeleton::eLimb);
				}
			}
		}

		RootNode->AddChild(RootBone);
	}

	return RootBone;

}

bool FDazToUnrealFbx::DetachGeometryFromSkeleton(FbxNode* RootNode, FbxScene *Scene)
{
	// Detach geometry from the skeleton
	for (int NodeIndex = 0; NodeIndex < Scene->GetNodeCount(); ++NodeIndex)
	{
		FbxNode* SceneNode = Scene->GetNode(NodeIndex);
		if (SceneNode == nullptr)
		{
			continue;
		}
		FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(SceneNode->GetMesh());
		if (NodeGeometry)
		{
			if (SceneNode->GetParent() &&
				SceneNode->GetParent()->GetNodeAttribute() &&
				SceneNode->GetParent()->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				// DB 2023-May-26: Only detach skinned geometry, leave props attached to bones
				if (NodeGeometry->GetDeformerCount(FbxDeformer::eSkin) > 0)
				{
					SceneNode->GetParent()->RemoveChild(SceneNode);
					RootNode->AddChild(SceneNode);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("DazToUnreal: leaving prop geometry (%s) attached to bone: %s"), ANSI_TO_TCHAR(SceneNode->GetName()), ANSI_TO_TCHAR(SceneNode->GetParent()->GetName()));
				}
			}
		}
	}

	return true;
}

bool FDazToUnrealFbx::AddIKBones(FbxNode* RootBone, FbxScene* Scene,
	const UDazToUnrealSettings* CachedSettings)
{
	// Add IK bones
	if (RootBone && CachedSettings->AddIKBones)
	{
		// ik_foot_root
		FbxNode* IKRootNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_root")));
		if (!IKRootNode)
		{
			// Create IK Root
			FbxSkeleton* IKRootNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_foot_root")));
			IKRootNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKRootNodeAttribute->Size.Set(1.0);
			IKRootNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_foot_root")));
			IKRootNode->SetNodeAttribute(IKRootNodeAttribute);
			IKRootNode->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));
			RootBone->AddChild(IKRootNode);
		}

		// ik_foot_l
		FbxNode* IKFootLNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_l")));
		FbxNode* FootLNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("lFoot")));
		if (!FootLNode) FootLNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("l_foot")));
		if (!IKFootLNode && FootLNode)
		{
			// Create IK Root
			FbxSkeleton* IKFootLNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_foot_l")));
			IKFootLNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKFootLNodeAttribute->Size.Set(1.0);
			IKFootLNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_foot_l")));
			IKFootLNode->SetNodeAttribute(IKFootLNodeAttribute);
			FbxVector4 FootLocation = FootLNode->EvaluateGlobalTransform().GetT();
			IKFootLNode->LclTranslation.Set(FootLocation);
			IKRootNode->AddChild(IKFootLNode);
		}

		// ik_foot_r
		FbxNode* IKFootRNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_r")));
		FbxNode* FootRNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("rFoot")));
		if (!FootRNode) FootRNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("r_foot")));
		if (!IKFootRNode && FootRNode)
		{
			// Create IK Root
			FbxSkeleton* IKFootRNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_foot_r")));
			IKFootRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKFootRNodeAttribute->Size.Set(1.0);
			IKFootRNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_foot_r")));
			IKFootRNode->SetNodeAttribute(IKFootRNodeAttribute);
			FbxVector4 FootLocation = FootRNode->EvaluateGlobalTransform().GetT();
			IKFootRNode->LclTranslation.Set(FootLocation);
			IKRootNode->AddChild(IKFootRNode);
		}

		// ik_hand_root
		FbxNode* IKHandRootNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_root")));
		if (!IKHandRootNode)
		{
			// Create IK Root
			FbxSkeleton* IKHandRootNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_root")));
			IKHandRootNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRootNodeAttribute->Size.Set(1.0);
			IKHandRootNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_root")));
			IKHandRootNode->SetNodeAttribute(IKHandRootNodeAttribute);
			IKHandRootNode->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));
			RootBone->AddChild(IKHandRootNode);
		}

		// ik_hand_gun
		FbxNode* IKHandGunNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
		FbxNode* HandRNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("rHand")));
		if (!HandRNode) HandRNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("r_hand")));
		if (!IKHandGunNode && HandRNode)
		{
			// Create IK Root
			FbxSkeleton* IKHandGunNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
			IKHandGunNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandGunNodeAttribute->Size.Set(1.0);
			IKHandGunNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
			IKHandGunNode->SetNodeAttribute(IKHandGunNodeAttribute);
			FbxVector4 HandLocation = HandRNode->EvaluateGlobalTransform().GetT();
			IKHandGunNode->LclTranslation.Set(HandLocation);
			IKHandRootNode->AddChild(IKHandGunNode);
		}

		// ik_hand_r
		FbxNode* IKHandRNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_r")));
		if (!IKHandRNode && HandRNode && IKHandGunNode)
		{
			// Create IK Root
			FbxSkeleton* IKHandRNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_r")));
			IKHandRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRNodeAttribute->Size.Set(1.0);
			IKHandRNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_r")));
			IKHandRNode->SetNodeAttribute(IKHandRNodeAttribute);
			IKHandRNode->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));
			IKHandGunNode->AddChild(IKHandRNode);
		}

		// ik_hand_l
		FbxNode* IKHandLNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_l")));
		FbxNode* HandLNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("lHand")));
		if (!HandLNode) HandLNode = Scene->FindNodeByName(TCHAR_TO_UTF8(TEXT("l_hand")));
		if (!IKHandLNode && HandLNode && IKHandGunNode)
		{
			// Create IK Root
			FbxSkeleton* IKHandRNodeAttribute = FbxSkeleton::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_l")));
			IKHandRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRNodeAttribute->Size.Set(1.0);
			IKHandLNode = FbxNode::Create(Scene, TCHAR_TO_UTF8(TEXT("ik_hand_l")));
			IKHandLNode->SetNodeAttribute(IKHandRNodeAttribute);
			FbxVector4 HandLocation = HandLNode->EvaluateGlobalTransform().GetT();
			FbxVector4 ParentLocation = IKHandGunNode->EvaluateGlobalTransform().GetT();
			IKHandLNode->LclTranslation.Set(HandLocation - ParentLocation);
			IKHandGunNode->AddChild(IKHandLNode);
		}
	}

	return true;
}

bool FDazToUnrealFbx::ProcessMorphs(FbxScene* Scene,
	const UDazToUnrealSettings* CachedSettings, TSharedPtr<FJsonObject>& JsonObject)
{
	// Get a list of morph name mappings
	TMap<FString, FString> MorphMappings;
	TArray<TSharedPtr<FJsonValue>> morphList = JsonObject->GetArrayField(TEXT("Morphs"));
	for (int32 i = 0; i < morphList.Num(); i++)
	{
		TSharedPtr<FJsonObject> morph = morphList[i]->AsObject();
		FString MorphName = morph->GetStringField(TEXT("Name"));
		FString MorphLabel = morph->GetStringField(TEXT("Label"));

		// Daz Studio seems to strip the part of the name before a period when exporting the morph to FBX
		if (MorphName.Contains(TEXT(".")))
		{
			FString Left;
			MorphName.Split(TEXT("."), &Left, &MorphName);
		}

		if (CachedSettings->UseInternalMorphName)
		{
			MorphMappings.Add(MorphName, MorphName);
		}
		else
		{
			MorphMappings.Add(MorphName, MorphLabel);
		}
	}

	// Combine clothing and body morphs
/***************************************************************************
	Progress.EnterProgressFrame(1, LOCTEXT("CombiningMorphs", "Combining Morphs")); 
*****************************************************************************/
	for (int NodeIndex = 0; NodeIndex < Scene->GetNodeCount(); ++NodeIndex)
	{
		FbxNode* SceneNode = Scene->GetNode(NodeIndex);
		if (SceneNode == nullptr)
		{
			continue;
		}
		FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(SceneNode->GetMesh());
		if (NodeGeometry)
		{

			const int32 BlendShapeDeformerCount = NodeGeometry->GetDeformerCount(FbxDeformer::eBlendShape);
			for (int32 BlendShapeIndex = 0; BlendShapeIndex < BlendShapeDeformerCount; ++BlendShapeIndex)
			{
				FbxBlendShape* BlendShape = (FbxBlendShape*)NodeGeometry->GetDeformer(BlendShapeIndex, FbxDeformer::eBlendShape);
				const int32 BlendShapeChannelCount = BlendShape->GetBlendShapeChannelCount();

				TArray<FbxBlendShapeChannel*> ChannelsToRemove;
				for (int32 ChannelIndex = 0; ChannelIndex < BlendShapeChannelCount; ++ChannelIndex)
				{
					FbxBlendShapeChannel* Channel = BlendShape->GetBlendShapeChannel(ChannelIndex);
					if (Channel)
					{
						FString ChannelName = UTF8_TO_TCHAR(Channel->GetNameOnly());
						FString NewChannelName, Extra;
						ChannelName.Split(TEXT("__"), &Extra, &NewChannelName);
						if (MorphMappings.Contains(NewChannelName))
						{
							NewChannelName = MorphMappings[NewChannelName];
							Channel->SetName(TCHAR_TO_UTF8(*NewChannelName));
						}
						else
						{
							ChannelsToRemove.AddUnique(Channel);
						}
					}
				}

				for (FbxBlendShapeChannel* ChannelToRemove : ChannelsToRemove)
				{
					BlendShape->RemoveBlendShapeChannel(ChannelToRemove);
				}
			}
		}
	}

	return true;
}

bool FDazToUnrealFbx::SaveUpdatedFbxFile(FbxManager* SdkManager, FbxScene* Scene, FbxNode* RootBone,
	FString& FBXFile,
	FString& FBXPath,
	FString& AssetName,
	const UDazToUnrealSettings* CachedSettings,
	DazToUnrealImportData& ImportData)
{
	// Create an exporter.
/***************************************************************************
	Progress.EnterProgressFrame(1, LOCTEXT("WritingUpdatedFBX", "Writing Updated FBX"));
***************************************************************************/
	FbxExporter* Exporter = FbxExporter::Create(SdkManager, "");
	int32 FileFormat = -1;

	// set file format
	if (CachedSettings->UpdatedFbxAsAscii)
	{
		FileFormat = SdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
	}
	else
	{
		FileFormat = SdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();
	}

	// Make folders for saving the updated FBX file
	FString UpdatedFBXFolder = FPaths::GetPath(FBXFile) / TEXT("UpdatedFBX");
	FString UpdatedFBXFile = FPaths::GetPath(FBXFile) / TEXT("UpdatedFBX") / FPaths::GetCleanFilename(FBXPath);
	if (!FDazToUnrealUtils::MakeDirectoryAndCheck(UpdatedFBXFolder)) return false;

	// Initialize the exporter by providing a filename.
	if (!Exporter->Initialize(TCHAR_TO_UTF8(*UpdatedFBXFile), FileFormat, SdkManager->GetIOSettings()))
	{
		return false;
	}

	// DB 2023-Sep-1: Re-Import Crash Prevention
	// 1. Obtain number of bones in the UpdatedFBX
	// 2. Check if skeletal mesh destination asset path exists
	// 3. Check if number of bones in existing skeletal mesh exactly matches number of bones in UpdatedFBX
	// 4. If not equal, then fail gracefully
	int FbxBoneCount = FDazToUnrealFbx::CountBonesInFbxNode(RootBone);
	int ExistingBoneCount = -1;
	FString DestinationPath = ImportData.ImportLocation + "/" + AssetName;
	UObject* ExistingMesh = StaticLoadObject(UObject::StaticClass(), nullptr, *DestinationPath);
	if (ExistingMesh) {
		USkeletalMesh* ExistingSkeletalMesh = Cast<USkeletalMesh>(ExistingMesh);
		if (ExistingSkeletalMesh) {
			ExistingBoneCount = ExistingSkeletalMesh->RefSkeleton.GetNum();
		}
	}
	if (ExistingBoneCount != -1 && ExistingBoneCount != FbxBoneCount)
	{
		const FString ErrorMessage = TEXT("The number of bones in the existing skeletal mesh does not match the number of \
bones in the new import. Aborting import.\n\n\
Please make sure the number of bones match, or change the Asset Name in the DazToUnreal Bridge to something different from \
the existing skeletal mesh in Unreal.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
		FText DialogText = FText::FromString(ErrorMessage);
		FText DialogTitle = FText::FromString(TEXT("DazToUnreal Import Error"));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText, &DialogTitle);
		Exporter->Destroy();
		return false;
	}

	// Export the scene.
	bool Status = Exporter->Export(Scene);

	// Destroy the exporter.
	Exporter->Destroy();

	return true;
}
