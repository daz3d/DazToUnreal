#include "DazToUnrealEnvironment.h"
#include "DazToUnrealSettings.h"
#include "DazToUnrealUtils.h"
#include "DazToUnreal.h"
#include "Dom/JsonObject.h"
#include "EditorLevelLibrary.h"
#include "Math/UnrealMathUtility.h"
#if ENGINE_MAJOR_VERSION > 4
#include "Subsystems/EditorActorSubsystem.h"
#endif

void FDazToUnrealEnvironment::ImportEnvironment(TSharedPtr<FJsonObject> JsonObject)
{
	// Get the list of instances
	const UDazToUnrealSettings* CachedSettings = GetDefault<UDazToUnrealSettings>();
	TArray<TSharedPtr<FJsonValue>> InstanceList = JsonObject->GetArrayField(TEXT("Instances"));
	TMap<FString, AActor*> GuidToActor;
	TMap<FString, TArray<FString>> ParentToChild;
	for (int32 Index = 0; Index< InstanceList.Num(); Index++)
	{
		TSharedPtr<FJsonObject> Instance = InstanceList[Index]->AsObject();

		// Get the instance details
		int32 Version = Instance->GetIntegerField(TEXT("Version"));
		FString InstanceAssetName = FDazToUnrealUtils::SanitizeName(Instance->GetStringField(TEXT("InstanceAsset")));
		double InstanceXPos = Instance->GetNumberField(TEXT("TranslationX"));
		double InstanceYPos = Instance->GetNumberField(TEXT("TranslationZ"));
		double InstanceZPos = Instance->GetNumberField(TEXT("TranslationY"));

		double Pitch = FMath::RadiansToDegrees(Instance->GetNumberField(TEXT("RotationZ")));
		double Yaw = FMath::RadiansToDegrees(Instance->GetNumberField(TEXT("RotationY"))) * -1.0f;
		double Roll = FMath::RadiansToDegrees(Instance->GetNumberField(TEXT("RotationX")));

		// Apply the rotations in the correct order
		FQuat PitchQuat(FRotator(Pitch, 0.0f, 0.0f));
		FQuat YawQuat(FRotator(0.0f, Yaw, 0.0f));
		FQuat RollQuat(FRotator(0.0f, 0.0f, Roll));
		FQuat Quat = PitchQuat * YawQuat * RollQuat;

		double ScaleXPos = Instance->GetNumberField(TEXT("ScaleX"));
		double ScaleYPos = Instance->GetNumberField(TEXT("ScaleZ"));
		double ScaleZPos = Instance->GetNumberField(TEXT("ScaleY"));

		FString ParentId = Instance->GetStringField(TEXT("ParentID"));
		FString InstanceId = Instance->GetStringField(TEXT("Guid"));
		FString InstanceLabel = Instance->GetStringField(TEXT("InstanceLabel"));

		// Make the child list if needed
		if (!ParentToChild.Contains(ParentId))
		{
			ParentToChild.Add(ParentId, TArray<FString>());
		}

		// Find the asset for this instance
		UObject* InstanceObject = nullptr;
		if (InstanceAssetName.Len() > 0)
		{
			FString AssetPath = CachedSettings->ImportDirectory.Path / InstanceAssetName / InstanceAssetName + TEXT(".") + InstanceAssetName;
			if (FDazToUnrealModule::BatchConversionMode != 0)
			{
				// Use AssetIDLookup
				FString RealAssetName = FDazToUnrealModule::AssetIDLookup[InstanceAssetName];
				AssetPath = CachedSettings->ImportDirectory.Path / FDazToUnrealModule::BatchConversionDestPath / RealAssetName + TEXT(".") + RealAssetName;
			}
			InstanceObject = LoadObject<UObject>(NULL, *AssetPath, NULL, LOAD_None, NULL);
		}

		// If this was imported with Force Front X Axis, fix the rotation
		if (InstanceObject && FDazToUnrealUtils::IsModelFacingX(InstanceObject))
		{
			FQuat FacingUndo(FRotator(0.0f, 90.0f, 0.0f));
			Quat = FacingUndo * PitchQuat * YawQuat * RollQuat;
		}

		// Spawn the object into the scene
		if (InstanceObject)
		{
			FVector Location = FVector(InstanceXPos, InstanceYPos, InstanceZPos);
			FRotator Rotation = Quat.Rotator();

#if ENGINE_MAJOR_VERSION > 4
			UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
			AActor* NewActor = EditorActorSubsystem->SpawnActorFromObject(InstanceObject, Location, Rotation);
#else
			AActor* NewActor = UEditorLevelLibrary::SpawnActorFromObject(InstanceObject, Location, Rotation);
#endif
			if (NewActor)
			{
				NewActor->SetActorScale3D(FVector(ScaleXPos, ScaleYPos, ScaleZPos));
				GuidToActor.Add(InstanceId, NewActor);
				ParentToChild[ParentId].Add(InstanceId);
				if (InstanceLabel.Len() > 0)
				{
					NewActor->SetActorLabel(InstanceLabel);
				}
			}
		}
		else
		{
			// If we didn't find the object, or it was a group node spawn a dummy actor
			FVector Location = FVector(InstanceXPos, InstanceYPos, InstanceZPos);
			FRotator Rotation = Quat.Rotator();

#if ENGINE_MAJOR_VERSION > 4
			UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
			AActor* NewActor = EditorActorSubsystem->SpawnActorFromClass(AActor::StaticClass(), Location, Rotation);
#else
			AActor* NewActor = UEditorLevelLibrary::SpawnActorFromClass(AActor::StaticClass(), Location, Rotation);
#endif
			if (NewActor)
			{
				NewActor->SetActorScale3D(FVector(ScaleXPos, ScaleYPos, ScaleZPos));
				if (USceneComponent* ParentDefaultAttachComponent = NewActor->GetDefaultAttachComponent())
				{
					ParentDefaultAttachComponent->Mobility = EComponentMobility::Static;
				}
				GuidToActor.Add(InstanceId, NewActor);
				ParentToChild[ParentId].Add(InstanceId);
				if (InstanceLabel.Len() > 0)
				{
					NewActor->SetActorLabel(InstanceLabel);
				}
			}
		}
	}

	// Re-create the hierarchy from Daz Studio
	for (auto Pair : ParentToChild)
	{
		for (FString ChildGuid : Pair.Value)
		{
			if (GuidToActor.Contains(Pair.Key) && GuidToActor.Contains(ChildGuid))
			{
				AActor* ParentActor = GuidToActor[Pair.Key];
				AActor* ChildActor = GuidToActor[ChildGuid];
				ChildActor->AttachToActor(ParentActor, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}

}
