#include "DazToUnrealFbx.h"



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
