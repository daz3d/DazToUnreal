// This node is a stripped down version of FAnimNode_BoneDrivenController with some rotation improvements.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Animation/AnimTypes.h"
#include "AnimNode_DazMorphDriver.generated.h"

class UCurveFloat;
class USkeletalMeshComponent;

UENUM()
enum class EDazMorphDriverRotationOrder : uint8
{
	Auto,
	XYZ,
	XZY,
	YXZ,
	YZX,
	ZXY,
	ZYX
};

/**
 * This is the runtime version of a bone driven controller, which maps part of the state from one bone to another (e.g., 2 * source.x -> target.z)
 */
USTRUCT()
struct DAZTOUNREALRUNTIME_API FAnimNode_DazMorphDriver : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	// Bone to use as controller input
	UPROPERTY(EditAnywhere, Category="Source (driver)")
	FBoneReference SourceBone;

	/** Curve used to map from the source attribute to the driven attributes if present (otherwise the Multiplier will be used) */
	UPROPERTY(EditAnywhere, Category=Mapping)
	UCurveFloat* DrivingCurve;

	// Multiplier to apply to the input value (Note: Ignored when a curve is used)
	UPROPERTY(EditAnywhere, Category=Mapping)
	float Multiplier;

	// Minimum limit of the input value (mapped to RemappedMin, only used when limiting the source range)
	// If this is rotation, the unit is radian
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange, DisplayName="Source Range Min"))
	float RangeMin;

	// Maximum limit of the input value (mapped to RemappedMax, only used when limiting the source range)
	// If this is rotation, the unit is radian
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange, DisplayName="Source Range Max"))
	float RangeMax;

	// Minimum value to apply to the destination (remapped from the input range)
	// If this is rotation, the unit is radian
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(EditCondition=bUseRange, DisplayName="Mapped Range Min"))
	float RemappedMin;

	// Maximum value to apply to the destination (remapped from the input range)
	// If this is rotation, the unit is radian
	UPROPERTY(EditAnywhere, Category = Mapping, meta = (EditCondition = bUseRange, DisplayName="Mapped Range Max"))
	float RemappedMax;

	/** Name of Morph Target to drive using the source attribute */
	UPROPERTY(EditAnywhere, Category = "Destination (driven)")
	FName MorphName;

public:

	// Transform component to use as input
	UPROPERTY(EditAnywhere, Category="Source (driver)")
	TEnumAsByte<EComponentType::Type> SourceComponent;

	// Transform component to use as input
	UPROPERTY(EditAnywhere, Category = "Source (driver)")
	EDazMorphDriverRotationOrder RotationConversionOrder;

	// Whether or not to clamp the driver value and remap it before scaling it
	UPROPERTY(EditAnywhere, Category=Mapping, meta=(DisplayName="Remap Source"))
	uint8 bUseRange : 1;

public:
	FAnimNode_DazMorphDriver();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

protected:
	
	// FAnimNode_SkeletalControlBase protected interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	/** Extracts the value used to drive the target bone or parameter */
	const float ExtractSourceValue(const FTransform& InCurrentBoneTransform, const FTransform& InRefPoseBoneTransform);
	// End of FAnimNode_SkeletalControlBase protected interface

	FVector EulerFromQuat(const FQuat& Rotation, EDazMorphDriverRotationOrder RotationOrder);
};
