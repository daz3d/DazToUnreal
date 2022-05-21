// This node is a stripped down version of FAnimNode_BoneDrivenController with some rotation improvements.

#include "AnimNode_DazMorphDriver.h"

#include "Curves/CurveFloat.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimTrace.h"


FAnimNode_DazMorphDriver::FAnimNode_DazMorphDriver()
	: DrivingCurve(nullptr)
	, Multiplier(1.0f)
	, RangeMin(-1.0f)
	, RangeMax(1.0f)
	, RemappedMin(0.0f)
	, RemappedMax(1.0f)
	, SourceComponent(EComponentType::None)
	, RotationConversionOrder(EDazMorphDriverRotationOrder::Auto)
	, bUseRange(false)
{
}

void FAnimNode_DazMorphDriver::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += "(";
	AddDebugNodeData(DebugLine);

	DebugLine += FString::Printf(TEXT("  DrivingBone: %s\nDrivenMorph: %s"), *SourceBone.BoneName.ToString(), *MorphName.ToString());

	
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_DazMorphDriver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{

}

void FAnimNode_DazMorphDriver::EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateComponentSpaceInternal)
	// Early out if we're not driving from or to anything
	if (SourceComponent == EComponentType::None)
	{
		return;
	}

	// Get the Local space transform and the ref pose transform to see how the transform for the source bone has changed
	const FBoneContainer& BoneContainer = Context.Pose.GetPose().GetBoneContainer();
	const FTransform& SourceRefPoseBoneTransform = BoneContainer.GetRefPoseArray()[SourceBone.BoneIndex];
	const FTransform SourceCurrentBoneTransform = Context.Pose.GetLocalSpaceTransform(SourceBone.GetCompactPoseIndex(BoneContainer));

	const float FinalDriverValue = ExtractSourceValue(SourceCurrentBoneTransform, SourceRefPoseBoneTransform);

	USkeleton* Skeleton = Context.AnimInstanceProxy->GetSkeleton();
	SmartName::UID_Type NameUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, MorphName);
	if (NameUID != SmartName::MaxUID)
	{
		Context.Curve.Set(NameUID, FinalDriverValue);
	}
}

const float FAnimNode_DazMorphDriver::ExtractSourceValue(const FTransform &InCurrentBoneTransform, const FTransform &InRefPoseBoneTransform)
{
	// Resolve source value
	float SourceValue = 0.0f;
	if (SourceComponent < EComponentType::RotationX)
	{
		const FVector TranslationDiff = InCurrentBoneTransform.GetLocation() - InRefPoseBoneTransform.GetLocation();
		SourceValue = TranslationDiff[(int32)(SourceComponent - EComponentType::TranslationX)];
	}
	else if (SourceComponent < EComponentType::Scale)
	{
		EDazMorphDriverRotationOrder RotationOrder = RotationConversionOrder;
		
		if (RotationOrder == EDazMorphDriverRotationOrder::Auto)
		{
			if (SourceComponent == EComponentType::RotationX)
			{
				RotationOrder = EDazMorphDriverRotationOrder::ZYX;
			}
			if (SourceComponent == EComponentType::RotationY)
			{
				RotationOrder = EDazMorphDriverRotationOrder::ZXY;
			}
			if (SourceComponent == EComponentType::RotationZ)
			{
				RotationOrder = EDazMorphDriverRotationOrder::YXZ;
			}
		}

		const FVector RotationDiff = EulerFromQuat((InCurrentBoneTransform * InRefPoseBoneTransform.Inverse()).GetRotation(), RotationOrder);
		SourceValue = RotationDiff[(int32)(SourceComponent - EComponentType::RotationX)];
		if (SourceComponent != EComponentType::RotationZ)
		{
			SourceValue = SourceValue * -1.0f;
		}
	}
	else if (SourceComponent == EComponentType::Scale)
	{
		const FVector CurrentScale = InCurrentBoneTransform.GetScale3D();
		const FVector RefScale = InRefPoseBoneTransform.GetScale3D();
		const float ScaleDiff = FMath::Max3(CurrentScale[0], CurrentScale[1], CurrentScale[2]) - FMath::Max3(RefScale[0], RefScale[1], RefScale[2]);
		SourceValue = ScaleDiff;
	}
	else
	{
		const FVector ScaleDiff = InCurrentBoneTransform.GetScale3D() - InRefPoseBoneTransform.GetScale3D();
		SourceValue = ScaleDiff[(int32)(SourceComponent - EComponentType::ScaleX)];
	}

	// Determine the resulting value
	float FinalDriverValue = SourceValue;
	if (DrivingCurve != nullptr)
	{
		// Remap thru the curve if set
		FinalDriverValue = DrivingCurve->GetFloatValue(FinalDriverValue);
	}
	else
	{
		// Apply the fixed function remapping/clamping
		if (bUseRange)
		{
			const float ClampedAlpha = FMath::Clamp(FMath::GetRangePct(RangeMin, RangeMax, FinalDriverValue), 0.0f, 1.0f);
			FinalDriverValue = FMath::Lerp(RemappedMin, RemappedMax, ClampedAlpha);
		}

		FinalDriverValue *= Multiplier;
	}

	return FinalDriverValue;
}

bool FAnimNode_DazMorphDriver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	return SourceBone.IsValidToEvaluate(RequiredBones);
}


void FAnimNode_DazMorphDriver::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	SourceBone.Initialize(RequiredBones);
}

// Copied from FControlRigMathLibrary::EulerFromQuat which is in an experimental plugin
FVector FAnimNode_DazMorphDriver::EulerFromQuat(const FQuat& Rotation, EDazMorphDriverRotationOrder RotationOrder)
{
	float X = Rotation.X;
	float Y = Rotation.Y;
	float Z = Rotation.Z;
	float W = Rotation.W;
	float X2 = X * 2.f;
	float Y2 = Y * 2.f;
	float Z2 = Z * 2.f;
	float XX2 = X * X2;
	float XY2 = X * Y2;
	float XZ2 = X * Z2;
	float YX2 = Y * X2;
	float YY2 = Y * Y2;
	float YZ2 = Y * Z2;
	float ZX2 = Z * X2;
	float ZY2 = Z * Y2;
	float ZZ2 = Z * Z2;
	float WX2 = W * X2;
	float WY2 = W * Y2;
	float WZ2 = W * Z2;

	FVector AxisX, AxisY, AxisZ;
	AxisX.X = (1.f - (YY2 + ZZ2));
	AxisY.X = (XY2 + WZ2);
	AxisZ.X = (XZ2 - WY2);
	AxisX.Y = (XY2 - WZ2);
	AxisY.Y = (1.f - (XX2 + ZZ2));
	AxisZ.Y = (YZ2 + WX2);
	AxisX.Z = (XZ2 + WY2);
	AxisY.Z = (YZ2 - WX2);
	AxisZ.Z = (1.f - (XX2 + YY2));

	FVector Result = FVector::ZeroVector;

	if (RotationOrder == EDazMorphDriverRotationOrder::XYZ)
	{
		Result.Y = FMath::Asin(-FMath::Clamp<float>(AxisZ.X, -1.f, 1.f));

		if (FMath::Abs(AxisZ.X) < 1.f - SMALL_NUMBER)
		{
			Result.X = FMath::Atan2(AxisZ.Y, AxisZ.Z);
			Result.Z = FMath::Atan2(AxisY.X, AxisX.X);
		}
		else
		{
			Result.X = 0.f;
			Result.Z = FMath::Atan2(-AxisX.Y, AxisY.Y);
		}
	}
	else if (RotationOrder == EDazMorphDriverRotationOrder::XZY)
	{

		Result.Z = FMath::Asin(FMath::Clamp<float>(AxisY.X, -1.f, 1.f));

		if (FMath::Abs(AxisY.X) < 1.f - SMALL_NUMBER)
		{
			Result.X = FMath::Atan2(-AxisY.Z, AxisY.Y);
			Result.Y = FMath::Atan2(-AxisZ.X, AxisX.X);
		}
		else
		{
			Result.X = 0.f;
			Result.Y = FMath::Atan2(AxisX.Z, AxisZ.Z);
		}
	}
	else if (RotationOrder == EDazMorphDriverRotationOrder::YXZ)
	{
		Result.X = FMath::Asin(FMath::Clamp<float>(AxisZ.Y, -1.f, 1.f));

		if (FMath::Abs(AxisZ.Y) < 1.f - SMALL_NUMBER)
		{
			Result.Y = FMath::Atan2(-AxisZ.X, AxisZ.Z);
			Result.Z = FMath::Atan2(-AxisX.Y, AxisY.Y);
		}
		else
		{
			Result.Y = 0.f;
			Result.Z = FMath::Atan2(AxisY.X, AxisX.X);
		}
	}
	else if (RotationOrder == EDazMorphDriverRotationOrder::YZX)
	{
		Result.Z = FMath::Asin(-FMath::Clamp<float>(AxisX.Y, -1.f, 1.f));

		if (FMath::Abs(AxisX.Y) < 1.f - SMALL_NUMBER)
		{
			Result.X = FMath::Atan2(AxisZ.Y, AxisY.Y);
			Result.Y = FMath::Atan2(AxisX.Z, AxisX.X);
		}
		else
		{
			Result.X = FMath::Atan2(-AxisY.Z, AxisZ.Z);
			Result.Y = 0.f;
		}
	}
	else if (RotationOrder == EDazMorphDriverRotationOrder::ZXY)
	{
		Result.X = FMath::Asin(-FMath::Clamp<float>(AxisY.Z, -1.f, 1.f));

		if (FMath::Abs(AxisY.Z) < 1.f - SMALL_NUMBER)
		{
			Result.Y = FMath::Atan2(AxisX.Z, AxisZ.Z);
			Result.Z = FMath::Atan2(AxisY.X, AxisY.Y);
		}
		else
		{
			Result.Y = FMath::Atan2(-AxisZ.X, AxisX.X);
			Result.Z = 0.f;
		}
	}
	else if (RotationOrder == EDazMorphDriverRotationOrder::ZYX)
	{
		Result.Y = FMath::Asin(FMath::Clamp<float>(AxisX.Z, -1.f, 1.f));

		if (FMath::Abs(AxisX.Z) < 1.f - SMALL_NUMBER)
		{
			Result.X = FMath::Atan2(-AxisY.Z, AxisZ.Z);
			Result.Z = FMath::Atan2(-AxisX.Y, AxisX.X);
		}
		else
		{
			Result.X = FMath::Atan2(AxisZ.Y, AxisY.Y);
			Result.Z = 0.f;
		}
	}

	return Result * 180.f / PI;
}