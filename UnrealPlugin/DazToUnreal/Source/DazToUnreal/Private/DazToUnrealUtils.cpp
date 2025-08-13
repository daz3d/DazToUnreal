#include "DazToUnrealUtils.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"
// only include in UE 4.26 and later
#if !(ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25)
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"

#include "Engine/StaticMesh.h"
#include "EditorFramework/AssetImportData.h"
#include "Factories/FbxAssetImportData.h"

#include "IPlatformFilePak.h"

#include "DazToUnrealSettings.h"

//////////////////////////////////////////////////////////////////
#include "IPlatformFilePak.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
FPakPlatformFile* MountPakForReadOnlyAccess(const FString& PakFilePath, const FString& MountPoint)
{
	// Use existing platform file, don't replace it
	IPlatformFile& InnerPlatform = FPlatformFileManager::Get().GetPlatformFile();

	// Create temporary FPakPlatformFile that delegates to the current platform file
	FPakPlatformFile* PakPlatformFile = new FPakPlatformFile();
	if (!PakPlatformFile->Initialize(&InnerPlatform, TEXT("")))
	{
		UE_LOG(LogTemp, Error, TEXT("PakPlatformFile failed to initialize."));
		delete PakPlatformFile;
		return nullptr;
	}

	// Mount the PAK file
	if (!PakPlatformFile->Mount(*PakFilePath, 0, *MountPoint))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to mount pak file: %s"), *PakFilePath);
		delete PakPlatformFile;
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("Mounted pak at %s"), *MountPoint);

	return PakPlatformFile;
}
void CopyPakFile(FPakPlatformFile* PakPlatformFile, const FString& InPakFile, const FString& OutputFilePath)
{
	if (PakPlatformFile->FileExists(*InPakFile))
	{
		if (FPaths::FileExists(OutputFilePath)) 
		{
			UE_LOG(LogTemp, Warning, TEXT("File already exists at output path: %s"), *OutputFilePath);
			return;
		}
		else
		{
			PakPlatformFile->CopyFile(*OutputFilePath, *InPakFile);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("File not found in pak: %s"), *InPakFile);
	}
}

#include "Materials/Material.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "UObject/UnrealType.h" // For FProperty
#include "Engine/Engine.h"
void AssignMaterialFunctionToMaterial(FString MaterialPath, FString FunctionPath)
{
	FSoftObjectPath BaseMaterialPath = FSoftObjectPath( MaterialPath);
	FSoftObjectPath BaseFunctionPath = FSoftObjectPath( FunctionPath);
	UMaterial* Material = Cast<UMaterial>(BaseMaterialPath.TryLoad());
	UMaterialFunction* NewFunction = Cast<UMaterialFunction>(BaseFunctionPath.TryLoad());

	if (!Material || !NewFunction)
    {
        UE_LOG(LogTemp, Error, TEXT("Material or MaterialFunction is null."));
        return;
    }

    // Iterate through expressions and find the function call node
    for (UMaterialExpression* Expr : Material->GetExpressions())
    {
        if (UMaterialExpressionMaterialFunctionCall* FunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
        {
			// If the function call already has the same function, do nothing
			if (FunctionCall->MaterialFunction == NewFunction) {
				UE_LOG(LogTemp, Log, TEXT("Material function is already assigned to the material."));
				return;
			}
			if (FunctionCall->MaterialFunction != nullptr) {
				UE_LOG(LogTemp, Log, TEXT("Material function currently assigned to: %s and will be replaced with: %s"), *FunctionCall->MaterialFunction->GetName(), *NewFunction->GetName());
			}

            // Pre-change notification
            FunctionCall->PreEditChange(nullptr);
            FunctionCall->MaterialFunction = NewFunction;

            // Look up the FProperty for 'MaterialFunction' to trigger correct GUI behavior
            static const FName PropertyName("MaterialFunction");
            FProperty* ChangedProp = FindFProperty<FProperty>(UMaterialExpressionMaterialFunctionCall::StaticClass(), PropertyName);
            if (ChangedProp) {
                // Post-change notification that preserves input/output links by name
                FPropertyChangedEvent PropertyChangedEvent(ChangedProp, EPropertyChangeType::ValueSet);
                FunctionCall->PostEditChangeProperty(PropertyChangedEvent);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Could not find 'MaterialFunction' FProperty."));
            }

            // Update the material and mark dirty
            Material->Modify();
            Material->PostEditChange();
            Material->MarkPackageDirty();
            Material->ForceRecompileForRendering();

            UE_LOG(LogTemp, Log, TEXT("Successfully assigned material function in place."));
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("No MaterialFunctionCall node found in material '%s'"), *Material->GetName());
}
void UnMountPak(FPakPlatformFile* PakPlatformFile, const FString& PakFilePath)
{
	PakPlatformFile->Unmount(*PakFilePath);
	UE_LOG(LogTemp, Log, TEXT("Unmounted pak file: %s"), *PakFilePath);
	delete PakPlatformFile;
}
//////////////////////////////////////////////////////////////////

FString FDazToUnrealUtils::SanitizeName(FString OriginalName)
{
	return OriginalName.Replace(TEXT(" "), TEXT(""))
		.Replace(TEXT("("), TEXT("_"))
		.Replace(TEXT(")"), TEXT("_"))
		.Replace(TEXT("."), TEXT("_"))
		.Replace(TEXT("&"), TEXT("_"))
		.Replace(TEXT("!"), TEXT("_"))
		.Replace(TEXT("*"), TEXT("_"))
		.Replace(TEXT("<"), TEXT("_"))
		.Replace(TEXT(">"), TEXT("_"))
		.Replace(TEXT("?"), TEXT("_"))
		.Replace(TEXT("\\"), TEXT("_"))
		.Replace(TEXT(":"), TEXT("_"))
		.Replace(TEXT("'"), TEXT("_"));
}

bool FDazToUnrealUtils::MakeDirectoryAndCheck(FString& Directory)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!FPaths::DirectoryExists(Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
		if (!FPaths::DirectoryExists(Directory))
		{
			UE_LOG(LogTemp, Warning, TEXT("DazToUnreal: ERROR: Unable to create directory tree: %s"), *Directory);
			return false;
		}
	}
	return true;
}

bool FDazToUnrealUtils::IsModelFacingX(UObject* MeshObject)
{
	if(USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(MeshObject))
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
		if (UAssetImportData* AssetImportData = SkeletalMesh->AssetImportData)
#else
		if (UAssetImportData* AssetImportData = SkeletalMesh->GetAssetImportData())
#endif
		{
			UFbxAssetImportData* FbxAssetImportData = Cast<UFbxAssetImportData>(AssetImportData);
			if (FbxAssetImportData != nullptr && FbxAssetImportData->bForceFrontXAxis)
			{
				return true;
			}
		}
	}
	if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshObject))
	{
		if (UAssetImportData* AssetImportData = StaticMesh->AssetImportData)
		{
			UFbxAssetImportData* FbxAssetImportData = Cast<UFbxAssetImportData>(AssetImportData);
			if (FbxAssetImportData != nullptr && FbxAssetImportData->bForceFrontXAxis)
			{
				return true;
			}
		}
	}
	return false;
}

FString FDazToUnrealUtils::GetDTUPathForModel(FSoftObjectPath MeshObjectPath)
{
	if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(MeshObjectPath.TryLoad()))
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
		if (UAssetImportData* AssetImportData = SkeletalMesh->AssetImportData)
#else
		if (UAssetImportData* AssetImportData = SkeletalMesh->GetAssetImportData())
#endif
		{
			if (UFbxAssetImportData* FbxAssetImportData = Cast<UFbxAssetImportData>(AssetImportData))
			{
				for (FAssetImportInfo::FSourceFile SourceFile : FbxAssetImportData->GetSourceData().SourceFiles)
				{
					FString SourceFilePath = SourceFile.RelativeFilename;
					TArray<FString> LikelyPaths;
					LikelyPaths.Add(FPaths::ChangeExtension(SourceFilePath, TEXT("dtu")));
					LikelyPaths.Add(FPaths::GetPath(SourceFilePath) + TEXT("/../") + FPaths::ChangeExtension(FPaths::GetCleanFilename(SourceFilePath), TEXT("dtu")));
					for (FString PossiblePath : LikelyPaths)
					{
						if (FPaths::FileExists(PossiblePath))
						{
							return PossiblePath;
						}
					}
				}
			}
		}
	}

	return FString();
}

FSoftObjectPath FDazToUnrealUtils::GetSkeletonForImport(const DazToUnrealImportData& DazImportData)
{
	UDazToUnrealSettings* CachedSettings = GetMutableDefault<UDazToUnrealSettings>();

	USkeleton* Skeleton = nullptr;
	FSoftObjectPath SkeletonPath;
	if (!DazImportData.bCreateUniqueSkeleton)
	{
		if (DazImportData.bFixTwistBones)
		{
			// Some character types share a skeleton.  Get the mapped name.
			FString MappedSkeletonName = DazImportData.CharacterTypeName;
			if (CachedSettings->CharacterTypeMapping.Contains(DazImportData.CharacterTypeName))
			{
				MappedSkeletonName = CachedSettings->CharacterTypeMapping[DazImportData.CharacterTypeName];
			}

			if (CachedSettings->SkeletonsWithTwistFix.Contains(MappedSkeletonName))
			{
				Skeleton = (USkeleton*)CachedSettings->SkeletonsWithTwistFix[MappedSkeletonName].TryLoad();
				if (Skeleton)
				{
					SkeletonPath = CachedSettings->SkeletonsWithTwistFix[MappedSkeletonName];
				}
				else
				{
					CachedSettings->SkeletonsWithTwistFix.Remove(MappedSkeletonName);
				}
			}
		}
		else
		{
			// Some character types share a skeleton.  Get the mapped name.
			FString MappedSkeletonName = DazImportData.CharacterTypeName;
			if (CachedSettings->CharacterTypeMapping.Contains(DazImportData.CharacterTypeName))
			{
				MappedSkeletonName = CachedSettings->CharacterTypeMapping[DazImportData.CharacterTypeName];
			}

			// Look for an existing skeleton for the project.
			if (CachedSettings->OtherSkeletons.Contains(MappedSkeletonName))
			{
				Skeleton = (USkeleton*)CachedSettings->OtherSkeletons[MappedSkeletonName].TryLoad();
				if (Skeleton)
				{
					SkeletonPath = CachedSettings->OtherSkeletons[MappedSkeletonName];
				}
				else
				{
					CachedSettings->OtherSkeletons.Remove(MappedSkeletonName);
				}
			}
			else
			{
				// Check in the plugin for a skeleton (going away soon)
				if (DazImportData.CharacterType == DazCharacterType::Genesis1)
				{
					Skeleton = (USkeleton*)CachedSettings->Genesis1Skeleton.TryLoad();
					SkeletonPath = CachedSettings->Genesis1Skeleton;
				}
				if (DazImportData.CharacterType == DazCharacterType::Genesis3Male || DazImportData.CharacterType == DazCharacterType::Genesis3Female)
				{
					Skeleton = (USkeleton*)CachedSettings->Genesis3Skeleton.TryLoad();
					SkeletonPath = CachedSettings->Genesis3Skeleton;
				}
				if (DazImportData.CharacterType == DazCharacterType::Genesis8Male || DazImportData.CharacterType == DazCharacterType::Genesis8Female)
				{
					Skeleton = (USkeleton*)CachedSettings->Genesis8Skeleton.TryLoad();
					SkeletonPath = CachedSettings->Genesis8Skeleton;
				}
			}


			// Only return one of the plugin skeletons if it's already used in the project.
			// We're moving away from using skeletons that are included with the plugin
			if (Skeleton && SkeletonPath.ToString().StartsWith(TEXT("/DazToUnreal/")))
			{
				if (!IsSkeletonUsed(SkeletonPath))
				{
					Skeleton = nullptr;
					SkeletonPath.Reset();
				}
			}
		}
	}

	return SkeletonPath;
}

bool FDazToUnrealUtils::IsSkeletonUsed(FSoftObjectPath SkeletonPath)
{
#if ENGINE_MAJOR_VERSION > 4
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetIdentifier> Referencers;
	AssetRegistry.GetReferencers(FAssetIdentifier(SkeletonPath.GetLongPackageFName()), Referencers);
	for (const FAssetIdentifier& Identifier : Referencers)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(Identifier.PackageName, Assets);

		for (const FAssetData& Asset : Assets)
		{
			if (Asset.IsInstanceOf(USkeletalMesh::StaticClass()))
			{
				return true;
			}
		}
	}
	return false;
#else
	return true;
#endif
}

void FDazToUnrealUtils::InstallPluginContentToProject()
{
	UDazToUnrealSettings* CachedSettings = GetMutableDefault<UDazToUnrealSettings>();
	FString DazImportFolder = CachedSettings->ImportDirectory.Path;
	FString DazCommonFolder = FPaths::Combine(DazImportFolder, TEXT("Common"));

	FString RelativePakPath = TEXT("Plugins/DazToUnreal/Content/DazToUnreal_Common.pak");
	FString ProjectPath = FPaths::ProjectDir();
	FString ProjectContentDir = FPaths::ProjectContentDir();
	FString FullPakPath = ProjectPath + RelativePakPath;

	FString PakMountPoint = TEXT("/Game/DazCommonPak/");
	FString FullCommonFolder = DazCommonFolder.Replace(TEXT("/Game/"), *ProjectContentDir);

	if (!FDazToUnrealUtils::MakeDirectoryAndCheck(FullCommonFolder)) {
		UE_LOG(LogTemp, Warning, TEXT("ERROR: DazToUnreal: Unable to create Common Content Folder: %s"), *FullCommonFolder);
		return;
	}

	FPakPlatformFile* PakPlatformFile = MountPakForReadOnlyAccess(FullPakPath, PakMountPoint);
	if (!PakPlatformFile) {
		return;
	}
	
    TArray<FString> Files;
    PakPlatformFile->FindFilesRecursively(Files, *PakMountPoint, TEXT(""));
    for (const FString& File : Files)
    {
        FString RelativePath = File;
        FPaths::MakePathRelativeTo(RelativePath, *PakMountPoint);
        FString DestPath = FPaths::Combine(FullCommonFolder, RelativePath);

        // Ensure directory exists
        FString DestDir = FPaths::GetPath(DestPath);
        PakPlatformFile->CreateDirectoryTree(*DestDir);

		CopyPakFile(PakPlatformFile, File, DestPath);
    }

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous({ FullCommonFolder }, true);

	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BasePBRSkinMaterial.BasePBRSkinMaterial"),
		DazCommonFolder + TEXT("/Materials/PBRSkinParameters.PBRSkinParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/IrayUberBaseMaterial.IrayUberBaseMaterial"),
		DazCommonFolder + TEXT("/Materials/IrayUberParameters.IrayUberParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/IrayUberSkinMaterial.IrayUberSkinMaterial"),
		DazCommonFolder + TEXT("/Materials/IrayUberParameters.IrayUberParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/AoASubsurfaceBaseMaterial.AoASubsurfaceBaseMaterial"),
		DazCommonFolder + TEXT("/Materials/AoASubsurfaceParameters.AoASubsurfaceParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/AoASubsurfaceSkinMaterial.AoASubsurfaceSkinMaterial"),
		DazCommonFolder + TEXT("/Materials/AoASubsurfaceParameters.AoASubsurfaceParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/omUberBaseMaterial.omUberBaseMaterial"),
		DazCommonFolder + TEXT("/Materials/omUberParameters.omUberParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/omUberSkinMaterial.omUberSkinMaterial"),
		DazCommonFolder + TEXT("/Materials/omUberParameters.omUberParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseMaterial.BaseMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseAlphaMaterial.BaseAlphaMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseHairMaterial.BaseHairMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseScalpMaterial.BaseScalpMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseSkinMaterial.BaseSkinMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseSSSSkinMaterial.BaseSSSSkinMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseMaskedMaterial.BaseMaskedMaterial"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);
	AssignMaterialFunctionToMaterial(
		DazCommonFolder + TEXT("/Materials/BaseMaterialTessellated.BaseMaterialTessellated"),
		DazCommonFolder + TEXT("/Materials/DazParameters.DazParameters")
	);

	AssetRegistryModule.Get().ScanPathsSynchronous({ FullCommonFolder }, true);

	UnMountPak(PakPlatformFile, FullPakPath);

	UE_LOG(LogTemp, Log, TEXT("INFO: DazToUnreal: Common Content Folder installed."));
	return;

}
