#pragma once

#include "dznode.h"
#include <fbxsdk.h>

struct MLDeformerTrainingSettings
{
	
};

class MLDeformer
{
public:
	static void GeneratePoses(DzNode* Node, int PoseCount);
	static void ExportTrainingData(DzNode* Node, QString FileName);

private:
	static void GetBoneList(DzNode* Node, QMap<QString, QList<DzNode*>>& Bones);
	static float RandomInRange(float Min, float Max);
	static void GetFigureList(DzNode* Node, QList<DzNode*>& Figures);
	static void TurnOnInteractiveUpdates(DzNode* Node);
};