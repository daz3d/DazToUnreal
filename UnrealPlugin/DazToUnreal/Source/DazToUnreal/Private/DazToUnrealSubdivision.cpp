#include "DazToUnrealSubdivision.h"

#ifndef M_PI
#define M_PI_NOT_DEFINED
#define M_PI PI		// OpenSubdiv is expecting M_PI to be defined already
#endif

#if ENGINE_MAJOR_VERSION > 4
#include "opensubdiv/far/topologyDescriptor.h"
#include "opensubdiv/far/primvarRefiner.h"
#else
#include "far/topologyDescriptor.h"
#include "far/primvarRefiner.h"
#endif

struct FDtuOsdSkinWeight
{
	FDtuOsdSkinWeight()
	{
		Weight = 0.0f;
	}

	void Clear(void* UnusedPtr = nullptr)
	{
		Weight = 0.0f;
	}

	void AddWithWeight(FDtuOsdSkinWeight const& SourceSkinWeight, float InWeight)
	{
		Weight += InWeight * SourceSkinWeight.Weight;
	}

	void SetWeight(float InWeight)
	{
		Weight = InWeight;
	}

	const float GetWeight() const 
	{
		return Weight;
	}

	float Weight;
};

struct FDtuOsdVector
{
	void Clear(void* UnusedPtr = nullptr)
	{
		Position = FVector(0, 0, 0);
	}

	void AddWithWeight(FDtuOsdVector const& SourceVertexPosition, float Weight)
	{
		Position += SourceVertexPosition.Position * Weight;
	}

	void SetPosition(FbxVector4 Vector) {
		Position[0] = (float)Vector[0];
		Position[1] = (float)Vector[1];
		Position[2] = (float)Vector[2];
	}

	FVector Position;
};

void FDazToUnrealSubdivision::SubdivideMesh(FbxNode* BaseNode, FbxNode* TranferWeightsToNode, int SubdLevel)
{
	FbxMesh* BaseMesh = BaseNode->GetMesh();

	// Get the topology
	TArray<int> VertsPerFace;
	VertsPerFace.AddDefaulted(BaseMesh->GetPolygonCount());
	TArray<int> VertexIndicesPerFace;
	for (int PolygonIndex = 0; PolygonIndex < BaseMesh->GetPolygonCount(); PolygonIndex++)
	{
		VertsPerFace[PolygonIndex] = BaseMesh->GetPolygonSize(PolygonIndex);
		for (int VertexIndex = 0; VertexIndex < VertsPerFace[PolygonIndex]; VertexIndex++)
		{
			int ControlPointIndex = BaseMesh->GetPolygonVertex(PolygonIndex, VertexIndex);
			if (ControlPointIndex < 0)
			{
				continue;
			}
			VertexIndicesPerFace.Add(ControlPointIndex);
		}
	}

	// Setup Options
	OpenSubdiv::Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;
	OpenSubdiv::Sdc::Options Options;
	Options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	// Setup Data
	OpenSubdiv::Far::TopologyDescriptor Descriptor;
	Descriptor.numVertices = BaseMesh->GetControlPointsCount();
	Descriptor.numFaces = BaseMesh->GetPolygonCount();
	Descriptor.numVertsPerFace = VertsPerFace.GetData();
	Descriptor.vertIndicesPerFace = VertexIndicesPerFace.GetData();

	// Create a Refiner
	OpenSubdiv::Far::TopologyRefiner* Refiner = OpenSubdiv::Far::TopologyRefinerFactory<OpenSubdiv::Far::TopologyDescriptor>::Create(Descriptor,
		OpenSubdiv::Far::TopologyRefinerFactory<OpenSubdiv::Far::TopologyDescriptor>::Options(type, Options));

	// Refine topology
	Refiner->RefineUniform(OpenSubdiv::Far::TopologyRefiner::UniformOptions(SubdLevel));

	// Determine the sizes for our needs:
	int CoarseVertsNum = BaseMesh->GetControlPointsCount();;
	int FineVertsNum = Refiner->GetLevel(SubdLevel).GetNumVertices();
	int TempVertsNum = Refiner->GetNumVerticesTotal() - CoarseVertsNum;

	// **************************************
	// First Interpolate the vertix positions

	// Get Base Mesh's vertex positions
	TArray<FDtuOsdVector> BasePositionBuffer;
	BasePositionBuffer.AddDefaulted(CoarseVertsNum);
	FbxVector4* ControlPoints = BaseMesh->GetControlPoints();
	for (int VertexIndex = 0; VertexIndex < CoarseVertsNum; VertexIndex++)
	{
		BasePositionBuffer[VertexIndex].SetPosition(ControlPoints[VertexIndex]);
	}

	// Prepare arrays for in and out data
	TArray<FDtuOsdVector> TempPositionBuffer;
	TempPositionBuffer.AddDefaulted(TempVertsNum);

	FDtuOsdVector* SourcePositions = reinterpret_cast<FDtuOsdVector*>(BasePositionBuffer.GetData());
	FDtuOsdVector* DestinationPositions = reinterpret_cast<FDtuOsdVector*>(TempPositionBuffer.GetData());


	// Interpolate the positions
	OpenSubdiv::Far::PrimvarRefiner primvarRefiner(*Refiner);

	for (int Level = 1; Level <= SubdLevel; ++Level)
	{
		primvarRefiner.Interpolate(Level, SourcePositions, DestinationPositions);
		SourcePositions = DestinationPositions;
		DestinationPositions += Refiner->GetLevel(Level).GetNumVertices();
	}

	// **************************************
	// Second Interpolate the new vertex weights

	// Interpolate for each Cluster in the Geometry
	FbxGeometry* Geometry = BaseMesh;
	int SkinCount = Geometry->GetDeformerCount(FbxDeformer::eSkin);
	for (int SkinIndex = 0; SkinIndex < SkinCount; ++SkinIndex)
	{
		int ClusterCount = ((FbxSkin*)Geometry->GetDeformer(SkinIndex, FbxDeformer::eSkin))->GetClusterCount();
		for (int ClusterIndex = 0; ClusterIndex != ClusterCount; ++ClusterIndex)
		{
			// Get the Base Cluster weights
			FbxCluster* BaseCluster = ((FbxSkin*)Geometry->GetDeformer(SkinIndex, FbxDeformer::eSkin))->GetCluster(ClusterIndex);
			int* ControlPointIndices = BaseCluster->GetControlPointIndices();
			double* ControlPointWeights = BaseCluster->GetControlPointWeights();

			TArray<FDtuOsdSkinWeight> BaseSkinWeightBuffer;
			BaseSkinWeightBuffer.AddDefaulted(CoarseVertsNum);
			for (int ControlPointIndex = 0; ControlPointIndex < BaseCluster->GetControlPointIndicesCount(); ControlPointIndex++)
			{
				BaseSkinWeightBuffer[ControlPointIndices[ControlPointIndex]].SetWeight((float)ControlPointWeights[ControlPointIndex]);
			}

			// Prepare In and Out Arrays
			TArray<FDtuOsdSkinWeight> TempSkinWeightBuffer;
			TempSkinWeightBuffer.AddDefaulted(TempVertsNum);
			TArray<FDtuOsdSkinWeight> InterpolatedSkinWeightBuffer;
			InterpolatedSkinWeightBuffer.AddDefaulted(FineVertsNum);

			// Interpolate skin weights
			FDtuOsdSkinWeight* SourceWeights = reinterpret_cast<FDtuOsdSkinWeight*>(BaseSkinWeightBuffer.GetData());
			FDtuOsdSkinWeight* DestinationWeights = reinterpret_cast<FDtuOsdSkinWeight*>(TempSkinWeightBuffer.GetData());
			for (int Level = 1; Level < SubdLevel; ++Level)
			{
				primvarRefiner.Interpolate(Level, SourceWeights, DestinationWeights);
				SourceWeights = DestinationWeights;
				DestinationWeights += Refiner->GetLevel(Level).GetNumVertices();
			}

			// Need to hold onto the last set of weights
			primvarRefiner.Interpolate(SubdLevel, SourceWeights, InterpolatedSkinWeightBuffer);

			// The TransferWeightToNode is the node in the HD FBX that's missing the weights.  Add the missing weights from the newly interpolated ones.
			FbxCluster* TransferCluster = ((FbxSkin*)TranferWeightsToNode->GetMesh()->GetDeformer(SkinIndex, FbxDeformer::eSkin))->GetCluster(ClusterIndex);
			int* TransferIndices = TransferCluster->GetControlPointIndices();
			TArray<int> ExistingWeightArray;
			for (int TransferControlPointIndex = 0; TransferControlPointIndex < TransferCluster->GetControlPointIndicesCount(); TransferControlPointIndex++)
			{
				ExistingWeightArray.Add(TransferIndices[TransferControlPointIndex]);
			}

			for (int VertexIndex = 0; VertexIndex < FineVertsNum; VertexIndex++)
			{
				float Weight = InterpolatedSkinWeightBuffer[VertexIndex].GetWeight();
				if (Weight > 0.0f)
				{
					// If the Vertex already has a weight, keep it.
					if (ExistingWeightArray.Contains(VertexIndex))
					{
						TransferCluster->GetControlPointWeights()[ExistingWeightArray.Find(VertexIndex)] = Weight;
					}
					// Otherwise, use the new weight
					else
					{
						TransferCluster->AddControlPointIndex(VertexIndex, Weight);
					}
				}
			}

		}

	}
}