// This node is a stripped down version of UAnimGraphNode_BoneDrivenController with some rotation improvements.

#include "AnimGraphNode_DazMorphDriver.h"
#include "AnimNode_DazMorphDriver.h"
#include "SceneManagement.h"
#include "Components/SkeletalMeshComponent.h"
#include "Widgets/Text/STextBlock.h"
#include "Kismet2/CompilerResultsLog.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "DetailCategoryBuilder.h"
#include "AnimationCustomVersion.h"

#define LOCTEXT_NAMESPACE "DazToUnreal"

UAnimGraphNode_DazMorphDriver::UAnimGraphNode_DazMorphDriver(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void UAnimGraphNode_DazMorphDriver::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAnimationCustomVersion::GUID);

	const int32 AnimVersion = Ar.CustomVer(FAnimationCustomVersion::GUID);

	if (AnimVersion < FAnimationCustomVersion::BoneDrivenControllerRemapping)
	{
		if (AnimVersion < FAnimationCustomVersion::BoneDrivenControllerMatchingMaya)
		{

			// The old definition of range was clamping the output, rather than the input
			if (Node.bUseRange && !FMath::IsNearlyZero(Node.Multiplier))
			{
				// Before: Output = clamp(Input * Multipler)
				// After: Output = clamp(Input) * Multiplier
				Node.RangeMin /= Node.Multiplier;
				Node.RangeMax /= Node.Multiplier;
			}
		}

		Node.RemappedMin = Node.RangeMin;
		Node.RemappedMax = Node.RangeMax;
	}
}

FText UAnimGraphNode_DazMorphDriver::GetTooltipText() const
{
	return LOCTEXT("UAnimGraphNode_DazMorphDriver_ToolTip", "Drives a morph target using the transform of a bone");
}

FText UAnimGraphNode_DazMorphDriver::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((Node.SourceBone.BoneName == NAME_None) && ((TitleType == ENodeTitleType::ListView) || (TitleType == ENodeTitleType::MenuTitle)))
	{
		return GetControllerDescription();
	}
	else
	{
		// Determine the mapping
		FText FinalSourceExpression;
		{
			FFormatNamedArguments SourceArgs;
			SourceArgs.Add(TEXT("SourceBone"), FText::FromName(Node.SourceBone.BoneName));
			SourceArgs.Add(TEXT("SourceComponent"), ComponentTypeToText(Node.SourceComponent));

			if (Node.DrivingCurve != nullptr)
			{
				FinalSourceExpression = LOCTEXT("BoneDrivenByCurve", "curve({SourceBone}.{SourceComponent})");
			}
			else
			{
				if (Node.bUseRange)
				{
					if (Node.Multiplier == 1.0f)
					{
						FinalSourceExpression = LOCTEXT("WithRangeBoneMultiplierIs1", "remap({SourceBone}.{SourceComponent})");
					}
					else
					{
						SourceArgs.Add(TEXT("Multiplier"), FText::AsNumber(Node.Multiplier));
						FinalSourceExpression = LOCTEXT("WithRangeNonUnityMultiplier", "remap({SourceBone}.{SourceComponent}) * {Multiplier}");
					}
				}
				else
				{
					if (Node.Multiplier == 1.0f)
					{
						FinalSourceExpression = LOCTEXT("BoneMultiplierIs1", "{SourceBone}.{SourceComponent}");
					}
					else
					{
						SourceArgs.Add(TEXT("Multiplier"), FText::AsNumber(Node.Multiplier));
						FinalSourceExpression = LOCTEXT("NonUnityMultiplier", "{SourceBone}.{SourceComponent} * {Multiplier}");
					}
				}
			}

			FinalSourceExpression = FText::Format(FinalSourceExpression, SourceArgs);
		}

		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDesc"), GetControllerDescription());

		Args.Add(TEXT("MorphName"), FText::FromName(Node.MorphName));


		// Determine the target component
		int32 NumComponents = 0;
		EComponentType::Type TargetComponent = EComponentType::None;

		#define UE_CHECK_TARGET_COMPONENT(ComponentProperty, ComponentChoice) if (ComponentProperty) { ++NumComponents; TargetComponent = ComponentChoice; }

		Args.Add(TEXT("TargetComponents"), (NumComponents <= 1) ? ComponentTypeToText(TargetComponent) : LOCTEXT("MultipleTargetComponents", "multiple"));

		if ((TitleType == ENodeTitleType::ListView) || (TitleType == ENodeTitleType::MenuTitle))
		{
			Args.Add(TEXT("Delim"), FText::FromString(TEXT(" - ")));
		}
		else
		{
			Args.Add(TEXT("Delim"), FText::FromString(TEXT("\n")));
		}

		Args.Add(TEXT("SourceExpression"), FinalSourceExpression);

		FText FormattedText;

		FormattedText = FText::Format(LOCTEXT("AnimGraphNode_BoneDrivenController_Title_Curve", "{MorphName} = {SourceExpression}{Delim}{ControllerDesc}"), Args);


		return FormattedText;
	}	
}

FText UAnimGraphNode_DazMorphDriver::GetControllerDescription() const
{
	return LOCTEXT("DazMorphDriver", "Daz Morph Driver");
}

void UAnimGraphNode_DazMorphDriver::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{	
	static const float ArrowHeadWidth = 5.0f;
	static const float ArrowHeadHeight = 8.0f;

	const int32 SourceIdx = SkelMeshComp->GetBoneIndex(Node.SourceBone.BoneName);


	if ((SourceIdx != INDEX_NONE))
	{
		const FTransform SourceTM = SkelMeshComp->GetComponentSpaceTransforms()[SourceIdx] * SkelMeshComp->GetComponentTransform();

		PDI->DrawPoint(SourceTM.GetTranslation(), FLinearColor(0.8f, 0.8f, 0.2f), 5.0f, SDPG_Foreground);

	}
}

void UAnimGraphNode_DazMorphDriver::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.SourceBone.BoneName) == INDEX_NONE)
	{
		MessageLog.Warning(*LOCTEXT("DriverJoint_NoSourceBone", "@@ - You must pick a source bone as the Driver joint").ToString(), this);
	}

	if (Node.SourceComponent == EComponentType::None)
	{
		MessageLog.Warning(*LOCTEXT("DriverJoint_NoSourceComponent", "@@ - You must pick a source component on the Driver joint").ToString(), this);
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}


void UAnimGraphNode_DazMorphDriver::AddTripletPropertyRow(const FText& Name, const FText& Tooltip, IDetailCategoryBuilder& Category, TSharedRef<IPropertyHandle> PropertyHandle, const FName XPropertyName, const FName YPropertyName, const FName ZPropertyName, TAttribute<EVisibility> VisibilityAttribute)
{
	const float XYZPadding = 5.0f;

	TSharedPtr<IPropertyHandle> XProperty = PropertyHandle->GetChildHandle(XPropertyName);
	Category.GetParentLayout().HideProperty(XProperty);

	TSharedPtr<IPropertyHandle> YProperty = PropertyHandle->GetChildHandle(YPropertyName);
	Category.GetParentLayout().HideProperty(YProperty);

	TSharedPtr<IPropertyHandle> ZProperty = PropertyHandle->GetChildHandle(ZPropertyName);
	Category.GetParentLayout().HideProperty(ZProperty);

	Category.AddCustomRow(Name)
	.Visibility(VisibilityAttribute)
	.NameContent()
	[
		SNew(STextBlock)
		.Text(Name)
		.ToolTipText(Tooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(0.f, 0.f, XYZPadding, 0.f)
		.AutoWidth()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				XProperty->CreatePropertyNameWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				XProperty->CreatePropertyValueWidget()
			]
		]

		+ SHorizontalBox::Slot()
		.Padding(0.f, 0.f, XYZPadding, 0.f)
		.AutoWidth()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				YProperty->CreatePropertyNameWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				YProperty->CreatePropertyValueWidget()
			]
		]

		+ SHorizontalBox::Slot()
		.Padding(0.f, 0.f, XYZPadding, 0.f)
		.AutoWidth()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ZProperty->CreatePropertyNameWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ZProperty->CreatePropertyValueWidget()
			]
		]
	];
}

void UAnimGraphNode_DazMorphDriver::AddRangePropertyRow(const FText& Name, const FText& Tooltip, IDetailCategoryBuilder& Category, TSharedRef<IPropertyHandle> PropertyHandle, const FName MinPropertyName, const FName MaxPropertyName, TAttribute<EVisibility> VisibilityAttribute)
{
	const float MiddlePadding = 4.0f;

	TSharedPtr<IPropertyHandle> MinProperty = PropertyHandle->GetChildHandle(MinPropertyName);
	Category.GetParentLayout().HideProperty(MinProperty);

	TSharedPtr<IPropertyHandle> MaxProperty = PropertyHandle->GetChildHandle(MaxPropertyName);
	Category.GetParentLayout().HideProperty(MaxProperty);

	Category.AddCustomRow(Name)
	.Visibility(VisibilityAttribute)
	.NameContent()
	[
		SNew(STextBlock)
		.Text(Name)
		.ToolTipText(Tooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(100.0f * 2.0f)
	.MaxDesiredWidth(100.0f * 2.0f)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.FillWidth(1)
		.Padding(0.0f, 0.0f, MiddlePadding, 0.0f)
		.VAlign(VAlign_Center)
		[
			MinProperty->CreatePropertyValueWidget()
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MinMaxSpacer", ".."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]

		+SHorizontalBox::Slot()
		.FillWidth(1)
		.Padding(MiddlePadding, 0.0f, 0.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			MaxProperty->CreatePropertyValueWidget()
		]
	];
}

void UAnimGraphNode_DazMorphDriver::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	Super::CustomizeDetails(DetailBuilder);

	TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAnimGraphNode_DazMorphDriver, Node), GetClass());

	// if it doesn't have this node, that means this isn't really bone driven controller
	if (!NodeHandle->IsValidHandle())
	{
		return;
	}

	TAttribute<EVisibility> NotUsingCurveVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&UAnimGraphNode_DazMorphDriver::AreNonCurveMappingValuesVisible, &DetailBuilder));
	TAttribute<EVisibility> MapRangeVisiblity = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&UAnimGraphNode_DazMorphDriver::AreRemappingValuesVisible, &DetailBuilder));

	// Source (Driver) category
	IDetailCategoryBuilder& SourceCategory = DetailBuilder.EditCategory(TEXT("Source (Driver)"));

	// Mapping category
	IDetailCategoryBuilder& MappingCategory = DetailBuilder.EditCategory(TEXT("Mapping"));
	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, DrivingCurve)));

	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, bUseRange))).Visibility(NotUsingCurveVisibility);

	AddRangePropertyRow(
		/*Name=*/ LOCTEXT("InputRangeLabel", "Source Range"),
		/*Tooltip=*/ LOCTEXT("InputRangeTooltip", "The range (relative to the reference pose) over which to limit the effect of the input component on the output component"),
		/*Category=*/ MappingCategory,
		/*PropertyHandle=*/ NodeHandle,
		/*X=*/ GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, RangeMin),
		/*Y=*/ GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, RangeMax),
		MapRangeVisiblity);
	AddRangePropertyRow(
		/*Name=*/ LOCTEXT("MappedRangeLabel", "Mapped Range"),
		/*Tooltip=*/ LOCTEXT("MappedRangeTooltip", "The range of mapped values that correspond to the input range"),
		/*Category=*/ MappingCategory,
		/*PropertyHandle=*/ NodeHandle,
		/*X=*/ GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, RemappedMin),
		/*Y=*/ GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, RemappedMax),
		MapRangeVisiblity);

	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, Multiplier))).Visibility(NotUsingCurveVisibility);

	// Destination visiblity
	TAttribute<EVisibility> BoneTargetVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&UAnimGraphNode_DazMorphDriver::AreTargetBonePropertiesVisible, &DetailBuilder));
	TAttribute<EVisibility> CurveTargetVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&UAnimGraphNode_DazMorphDriver::AreTargetCurvePropertiesVisible, &DetailBuilder));

	// Destination (Driven) category
	IDetailCategoryBuilder& TargetCategory = DetailBuilder.EditCategory(TEXT("Destination (Driven)"));

	//TargetCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, DestinationMode)));

	TargetCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, MorphName))).Visibility(CurveTargetVisibility);

	// Add a note about the space (it is not configurable, and this helps set expectations)
	const FText TargetBoneSpaceName = LOCTEXT("TargetComponentSpace", "Target Component Space");
	TargetCategory.AddCustomRow(TargetBoneSpaceName)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(TargetBoneSpaceName)
			.Font(IDetailLayoutBuilder::GetDetailFont())			
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TargetComponentSpaceIsAlwaysParentBoneSpace", "Parent Bone Space"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.Visibility(BoneTargetVisibility);


	//TargetCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_DazMorphDriver, ModificationMode))).Visibility(BoneTargetVisibility);
}

FText UAnimGraphNode_DazMorphDriver::ComponentTypeToText(EComponentType::Type Component)
{
	switch (Component)
	{
	case EComponentType::TranslationX:
		return LOCTEXT("ComponentType_TranslationX", "translateX");
	case EComponentType::TranslationY:
		return LOCTEXT("ComponentType_TranslationY", "translateY");
	case EComponentType::TranslationZ:
		return LOCTEXT("ComponentType_TranslationZ", "translateZ");
	case EComponentType::RotationX:
		return LOCTEXT("ComponentType_RotationX", "rotateX");
	case EComponentType::RotationY:
		return LOCTEXT("ComponentType_RotationY", "rotateY");
	case EComponentType::RotationZ:
		return LOCTEXT("ComponentType_RotationZ", "rotateZ");
	case EComponentType::Scale:
		return LOCTEXT("ComponentType_ScaleMax", "scaleMax");
	case EComponentType::ScaleX:
		return LOCTEXT("ComponentType_ScaleX", "scaleX");
	case EComponentType::ScaleY:
		return LOCTEXT("ComponentType_ScaleY", "scaleY");
	case EComponentType::ScaleZ:
		return LOCTEXT("ComponentType_ScaleZ", "scaleZ");
	default:
		return LOCTEXT("ComponentType_None", "(none)");
	}
}

EVisibility UAnimGraphNode_DazMorphDriver::AreNonCurveMappingValuesVisible(IDetailLayoutBuilder* DetailLayoutBuilder)
{
	const TArray<TWeakObjectPtr<UObject>>& SelectedObjectsList = DetailLayoutBuilder->GetSelectedObjects();
	for(TWeakObjectPtr<UObject> Object : SelectedObjectsList)
	{
		if(UAnimGraphNode_DazMorphDriver* BoneDrivenController = Cast<UAnimGraphNode_DazMorphDriver>(Object.Get()))
		{
			if (BoneDrivenController->Node.DrivingCurve == nullptr)
			{
				return EVisibility::Visible;
			}
		}
	}

	return EVisibility::Collapsed;
}

EVisibility UAnimGraphNode_DazMorphDriver::AreRemappingValuesVisible(IDetailLayoutBuilder* DetailLayoutBuilder)
{
	const TArray<TWeakObjectPtr<UObject>>& SelectedObjectsList = DetailLayoutBuilder->GetSelectedObjects();
	for(TWeakObjectPtr<UObject> Object : SelectedObjectsList)
	{
		if(UAnimGraphNode_DazMorphDriver* BoneDrivenController = Cast<UAnimGraphNode_DazMorphDriver>(Object.Get()))
		{
			if((BoneDrivenController->Node.DrivingCurve == nullptr) && BoneDrivenController->Node.bUseRange)
			{
				return EVisibility::Visible;
			}
		}
	}

	return EVisibility::Collapsed;
}

EVisibility UAnimGraphNode_DazMorphDriver::AreTargetBonePropertiesVisible(IDetailLayoutBuilder* DetailLayoutBuilder)
{
	return EVisibility::Collapsed;
}

EVisibility UAnimGraphNode_DazMorphDriver::AreTargetCurvePropertiesVisible(IDetailLayoutBuilder* DetailLayoutBuilder)
{
	TArray<TWeakObjectPtr<UObject>> SelectedObjectsList = DetailLayoutBuilder->GetSelectedObjects();
	for(TWeakObjectPtr<UObject> Object : SelectedObjectsList)
	{
		if(UAnimGraphNode_DazMorphDriver* BoneDrivenController = Cast<UAnimGraphNode_DazMorphDriver>(Object.Get()))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
