#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "DazJointControlledMorphAnimInstance.generated.h"

UENUM()
enum class EDazMorphAnimInstanceRotationOrder : uint8
{
	Auto,
	XYZ,
	XZY,
	YXZ,
	YZX,
	ZXY,
	ZYX
};

UENUM()
enum class EDazMorphAnimInstanceDriver : uint8
{
	None,
	RotationX,
	RotationY,
	RotationZ
};

USTRUCT(Blueprintable)
struct DAZTOUNREALRUNTIME_API FDazJointControlLinkKey
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		float BoneRotation = 0.0f;;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		float MorphAlpha = 0.0f;
};

USTRUCT(Blueprintable)
struct DAZTOUNREALRUNTIME_API FDazJointControlLink
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FName BoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		EDazMorphAnimInstanceDriver PrimaryAxis = EDazMorphAnimInstanceDriver::RotationY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		EDazMorphAnimInstanceDriver SecondaryAxis = EDazMorphAnimInstanceDriver::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		FName MorphName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		float Scalar = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		float Alpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TArray<FDazJointControlLinkKey> Keys;
};

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct DAZTOUNREALRUNTIME_API FDazJointControlledMorphAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FDazJointControlledMorphAnimInstanceProxy()
	{
	}

	FDazJointControlledMorphAnimInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual ~FDazJointControlledMorphAnimInstanceProxy();

	// FAnimInstanceProxy interface
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	//virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;

	//TMap<int32, FTransform> StoredTransforms;
	//TMap<SmartName::UID_Type, float> StoredCurves;

private:
	//TArray<FDazJointControlLink> XRotation;
	//TArray<FDazJointControlLink> YRotation;
	//TArray<FDazJointControlLink> ZRotation;
	TArray<FDazJointControlLink> ControlLinks;

	const float ExtractSourceValue(const FTransform& InCurrentBoneTransform, const FTransform& InRefPoseBoneTransform, EDazMorphAnimInstanceDriver Controller, EDazMorphAnimInstanceDriver SecondaryController);
	FVector EulerFromQuat(const FQuat& Rotation, EDazMorphAnimInstanceRotationOrder RotationOrder);
	void ProcessLink(FPoseContext& Output, FDazJointControlLink Link, EDazMorphAnimInstanceDriver Driver, EDazMorphAnimInstanceDriver SecondaryDriver);
};

UCLASS(Blueprintable)
class DAZTOUNREALRUNTIME_API UDazJointControlledMorphAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UDazJointControlledMorphAnimInstance() {};
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<FDazJointControlLink> ControlLinks;

private:
	//TArray<FDazJointControlLink> XRotation;
	//TArray<FDazJointControlLink> YRotation;
	//TArray<FDazJointControlLink> ZRotation;

protected:

	// UAnimInstance interface
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

};