#include "DazToUnrealUtils.h"
#include "DazToUnreal.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"

// REQUIRED TO USE UE_VERSION_NEWER_THAN
#include "Misc/EngineVersionComparison.h"

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
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	for (UMaterialExpression* Expr : Material->GetExpressions())
#else
	for (UMaterialExpression* Expr : Material->Expressions)
#endif
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

	if (FDazToUnrealModule::BatchConversionMode != 0)
	{
		DazCommonFolder = FDazToUnrealModule::OverrideConversionDestPath;
	}

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

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27)
	AssetRegistryModule.Get().ScanPathsSynchronous({ DazCommonFolder }, true);
#else
	AssetRegistryModule.Get().ScanPathsSynchronous({ FullCommonFolder }, true);
#endif

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

#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 27)
	AssetRegistryModule.Get().ScanPathsSynchronous({ DazCommonFolder }, true);
#else
	AssetRegistryModule.Get().ScanPathsSynchronous({ FullCommonFolder }, true);
#endif

	UnMountPak(PakPlatformFile, FullPakPath);

	UE_LOG(LogTemp, Log, TEXT("INFO: DazToUnreal: Common Content Folder installed."));
	return;

}

FSoftObjectPath FDazToUnrealUtils::FindMaterial(FString ShaderName, EDazMaterialType MaterialType)
{
	const UDazToUnrealSettings* CachedSettings = GetDefault<UDazToUnrealSettings>();

	for (FSoftObjectPath MaterialPackPath : CachedSettings->MaterialPacks)
	{
		if (UDazToUnrealMaterialPack* MaterialPack = Cast<UDazToUnrealMaterialPack>(MaterialPackPath.TryLoad()))
		{
			FSoftObjectPath MaterialPath = FDazToUnrealUtils::FindMaterial(ShaderName, MaterialType);
			if (MaterialPath.IsValid() && !MaterialPath.IsNull())
			{
				return MaterialPath;
			}
		}
	}

	FString DazCommonFolder = CachedSettings->ImportDirectory.Path + TEXT("/Common");
	FString CommonMaterialsFolder = DazCommonFolder + TEXT("/Materials");
	if (FDazToUnrealModule::BatchConversionMode != 0)
	{
		CommonMaterialsFolder = FDazToUnrealModule::OverrideConversionDestPath + TEXT("/Materials");
	}

	// Hard code the old settings.  This could be done via a built in pack instead, but would need to be made in 4.25.
	if (MaterialType == EDazMaterialType::Base)
	{
		if (ShaderName.Compare(TEXT("Daz Studio Default")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/DSDBaseMaterial.DSDBaseMaterial"));
		if (ShaderName.Compare(TEXT("omUberSurface")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/omUberBaseMaterial.omUberBaseMaterial"));
		if (ShaderName.Compare(TEXT("AoA_Subsurface")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/AoASubsurfaceBaseMaterial.AoASubsurfaceBaseMaterial"));
		if (ShaderName.Compare(TEXT("Iray Uber")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/IrayUberBaseMaterial.IrayUberBaseMaterial"));
		if (ShaderName.Compare(TEXT("PBRSkin")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BasePBRSkinMaterial.BasePBRSkinMaterial"));
	}

	if (MaterialType == EDazMaterialType::Skin)
	{
		if (ShaderName.Compare(TEXT("Daz Studio Default")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/DSDBaseMaterial.DSDBaseMaterial"));
		if (ShaderName.Compare(TEXT("omUberSurface")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/omUberSkinMaterial.omUberSkinMaterial"));
		if (ShaderName.Compare(TEXT("AoA_Subsurface")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/AoASubsurfaceSkinMaterial.AoASubsurfaceSkinMaterial"));
		if (ShaderName.Compare(TEXT("Iray Uber")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/IrayUberSkinMaterial.IrayUberSkinMaterial"));
		if (ShaderName.Compare(TEXT("PBRSkin")) == 0) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BasePBRSkinMaterial.BasePBRSkinMaterial"));
	}

	if(MaterialType == EDazMaterialType::Base) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseMaterial.BaseMaterial"));
	if (MaterialType == EDazMaterialType::Alpha) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseAlphaMaterial.BaseAlphaMaterial"));
	if (MaterialType == EDazMaterialType::Masked) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseMaskedMaterial.BaseMaskedMaterial"));
	if (MaterialType == EDazMaterialType::Skin) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseSSSSkinMaterial.BaseSSSSkinMaterial"));
	if (MaterialType == EDazMaterialType::Hair) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseHairMaterial.BaseHairMaterial"));
	if (MaterialType == EDazMaterialType::StrandHair) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseStrandHairMaterial.BaseStrandHairMaterial"));
	if (MaterialType == EDazMaterialType::Scalp) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseScalpMaterial.BaseScalpMaterial"));
	if (MaterialType == EDazMaterialType::EyeMoisture) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseAlphaMaterial.BaseAlphaMaterial"));
	if (MaterialType == EDazMaterialType::Cornea) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseAlphaMaterial.BaseAlphaMaterial"));
	if (MaterialType == EDazMaterialType::NoDraw) return FSoftObjectPath(CommonMaterialsFolder + TEXT("/NoDrawMaterial.NoDrawMaterial"));

	// Fall back to a known default if nothing else was found.
	return FSoftObjectPath(CommonMaterialsFolder + TEXT("/BaseMaterial.BaseMaterial"));
}

#include "AssetToolsModule.h"
bool FDazToUnrealUtils::MoveSingleAsset(const FString& SourcePath, const FString& DestinationFolder)
{
	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
	IAssetTools& AssetTools = AssetToolsModule.Get();

	// Load the asset
//	UObject* Asset = LoadObject<UObject>(nullptr, *SourcePath);
	UObject* Asset = FSoftObjectPath(SourcePath).TryLoad();
	if (!Asset)
	{
		UE_LOG(LogDazToUnreal, Error, TEXT("MoveSingleAsset: Failed to load asset: %s"), *SourcePath);
		return false;
	}

	FString AbsoluteDestinationFolder = DestinationFolder.Replace(TEXT("/Game/"), *FPaths::ProjectContentDir());
	if (!FDazToUnrealUtils::MakeDirectoryAndCheck(AbsoluteDestinationFolder))
	{
		UE_LOG(LogDazToUnreal, Error, TEXT("MoveSingleAsset: Could not create directory %s"), *AbsoluteDestinationFolder);
		return false;
	}

	FString AssetName = FPackageName::GetShortName(SourcePath);
	TArray<FAssetRenameData> AssetsToRename;
	AssetsToRename.Emplace(Asset, DestinationFolder, AssetName);

	bool bRenameSuccessful = AssetTools.RenameAssets(AssetsToRename);
	if (!bRenameSuccessful)
	{
		UE_LOG(LogDazToUnreal, Error, TEXT("MoveSingleAsset: Failed to rename asset: %s"), *SourcePath);
		return false;
	}

	FString OldObjectPath = SourcePath;
	FString NewObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DestinationFolder, *AssetName, *AssetName);

	// Fix soft references in all loaded packages
	TArray<UPackage*> PackagesToCheck;
	{
		FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AllAssets;
		Arm.Get().GetAssetsByPath(FName("/Game"), AllAssets, true);
		for (const FAssetData& Ad : AllAssets)
		{
			if (UPackage* Pkg = Ad.GetPackage())
				PackagesToCheck.AddUnique(Pkg);
		}
	}

	TMap<FSoftObjectPath, FSoftObjectPath> RedirectMap;
	RedirectMap.Add(FSoftObjectPath(OldObjectPath), FSoftObjectPath(NewObjectPath));

	AssetTools.RenameReferencingSoftObjectPaths(PackagesToCheck, RedirectMap);

	// 6. Collect and clean redirectors in destination
	TArray<UObjectRedirector*> Redirectors;
	{
		FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> Assets;
		Arm.Get().GetAssetsByPath(*DestinationFolder, Assets, true);

		for (const FAssetData& Ad : Assets)
		{
			if (Ad.AssetClass == UObjectRedirector::StaticClass()->GetFName())
			{
				if (UObjectRedirector* R = Cast<UObjectRedirector>(Ad.GetAsset()))
					Redirectors.Add(R);
			}
		}
	}

	if (Redirectors.Num() > 0)
	{
#if UE_VERSION_NEWER_THAN(5,0,99)
		AssetTools.FixupReferencers(Redirectors, /*bCheckoutDialogPrompt*/ false, ERedirectFixupMode::DeleteFixedUpRedirectors);
#elif UE_VERSION_NEWER_THAN(4,26,99)
		AssetTools.FixupReferencers(Redirectors, /*bCheckoutDialogPrompt*/ false);
#else
		AssetTools.FixupReferencers(Redirectors);  // UE4.25 and earlier
#endif
	}

	UE_LOG(LogDazToUnreal, Log, TEXT("MoveSingleAsset: Moved asset %s → %s"), *SourcePath, *NewObjectPath);

	return true;
}


void FDazToUnrealUtils::ReplaceSkeleton(FString AnimPath, FString SkeletonPath)
{
	FSoftObjectPath animSoftPath(AnimPath);
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(animSoftPath.TryLoad());

	if (!AnimSequence) {
		UE_LOG(LogDazToUnreal, Error, TEXT("DazToUnreal: ReplaceSkeleton: Could not load animation at path: %s"), *AnimPath);
		return;
	}

	FSoftObjectPath softPath(SkeletonPath);
	USkeleton* pSkeleton = Cast<USkeleton>(softPath.TryLoad());

	if (!pSkeleton) {
		UE_LOG(LogDazToUnreal, Error, TEXT("DazToUnreal: ReplaceSkeleton: Could not load skeleton at path: %s"), *SkeletonPath);
		return;
	}

	AnimSequence->SetSkeleton(pSkeleton);
	AnimSequence->MarkPackageDirty();

}


#include "EditorLevelLibrary.h"
#include "LevelEditor.h"
#if UE_VERSION_NEWER_THAN(5, 0, 99)
#include "LevelEditorSubsystem.h"
#endif

void FDazToUnrealUtils::MakeNewFabLevel(const FString& NewMapPath)
{

	FString LevelPath = NewMapPath;
	FString TemplatePath = TEXT("/Game/Level_01");
#if UE_VERSION_NEWER_THAN(5,0,99)
	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		LevelEditorSubsystem->NewLevelFromTemplate(LevelPath, TemplatePath);
		LevelEditorSubsystem->LoadLevel(LevelPath);
	}
#else
	UEditorLevelLibrary::NewLevelFromTemplate(LevelPath, TemplatePath);
#endif

#if UE_VERSION_NEWER_THAN(5, 0, 99)
	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		LevelEditorSubsystem->SaveCurrentLevel();
	}
#else
	UEditorLevelLibrary::SaveCurrentLevel();
#endif

}

void FDazToUnrealUtils::SaveCurrentLevel()
{

#if UE_VERSION_NEWER_THAN(5, 0, 99)
	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		LevelEditorSubsystem->SaveCurrentLevel();
	}
#else
	UEditorLevelLibrary::SaveCurrentLevel();
#endif

}


#include "Animation/SkeletalMeshActor.h"
// #include "Engine/SkeletalMeshActor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Editor.h"
#include "EngineUtils.h"

void FDazToUnrealUtils::AssignSkeletalMeshToActor(FString sActorLabel, USkeletalMesh* pMesh)
{
	UWorld* pWorld = GEditor->GetEditorWorldContext().World();

	for (TActorIterator<ASkeletalMeshActor> It(pWorld); It; ++It)
	{
		ASkeletalMeshActor* pActor = *It;
		if (pActor)
		{
			UE_LOG(LogTemp, Log, TEXT("Found SkeletalMeshActor: %s"), *pActor->GetActorLabel ());
			if (pActor->GetActorLabel ().Contains(sActorLabel))
			{
				UE_LOG(LogTemp, Log, TEXT("Assigning SkeletalMesh to Actor: %s"), *pActor->GetActorLabel ());
				pActor->GetSkeletalMeshComponent()->SetSkeletalMesh(pMesh);
				pActor->GetSkeletalMeshComponent()->MarkRenderStateDirty();
				return;
			}
		}
	}
}

void FDazToUnrealUtils::AssignAnimSequenceToActor(FString sActorLabel, UAnimSequence* pAnim)
{
	UWorld* pWorld = GEditor->GetEditorWorldContext().World();

	for (TActorIterator<ASkeletalMeshActor> It(pWorld); It; ++It)
	{
		ASkeletalMeshActor* pActor = *It;
		if (pActor)
		{
			UE_LOG(LogTemp, Log, TEXT("Found SkeletalMeshActor: %s"), *pActor->GetActorLabel ());
			if (pActor->GetActorLabel ().Contains(sActorLabel))
			{
				UE_LOG(LogTemp, Log, TEXT("Assigning Animation to Actor: %s"), *pActor->GetActorLabel ());
				USkeletalMeshComponent* pSkeletalMeshComponent = pActor->GetSkeletalMeshComponent();
				pSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				pSkeletalMeshComponent->InitializeAnimScriptInstance();

				pSkeletalMeshComponent->AnimationData.AnimToPlay = pAnim;
				pSkeletalMeshComponent->AnimationData.bSavedPlaying = true;
				pSkeletalMeshComponent->AnimationData.bSavedLooping = true;
				pSkeletalMeshComponent->AnimationData.SavedPosition = 0.f;

				pSkeletalMeshComponent->PostEditChange();
				pSkeletalMeshComponent->MarkRenderStateDirty();
				return;
			}
		}
	}
}

#include "Animation/Skeleton.h"
#include "UObject/Package.h"

bool FDazToUnrealUtils::AddCompatibleSkeleton(USkeleton* pTarget, USkeleton* pCompatible)
{
	if (!pTarget || !pCompatible || pTarget == pCompatible)
		return false;

#if UE_VERSION_NEWER_THAN(4, 27, 99)
	const TArray<TSoftObjectPtr<USkeleton>>& aExisting = pTarget->GetCompatibleSkeletons();
	if (aExisting.Contains(pCompatible)) {
		UE_LOG(LogDazToUnreal, Warning, TEXT("Skeleton %s is already compatible with %s"), *pCompatible->GetName(), *pTarget->GetName());
		// return false;
	}

	pTarget->Modify();
	pTarget->AddCompatibleSkeleton(pCompatible);
	pTarget->MarkPackageDirty();

	return true;
#else
	return false;
#endif

}

#if UE_VERSION_NEWER_THAN(5,0,99)
#include "Editor/MaterialEditor/Public/MaterialEditingLibrary.h"
#endif
bool FDazToUnrealUtils::ModifyMaterial_TranslucentToMasked(UMaterial* pMaterial, bool bSaveChanges)
{
	if (!pMaterial)
		return false;

	// Switch from Translucent to Masked
	pMaterial->BlendMode = BLEND_Masked;
	pMaterial->SetShadingModel(MSM_DefaultLit);
	pMaterial->bUseMaterialAttributes = false;

	// Find existing opacity input connection
#if UE_VERSION_NEWER_THAN(5,0,99)
	UMaterialExpression* opacityExpression = pMaterial->GetExpressionInputForProperty(MP_Opacity)
		? pMaterial->GetExpressionInputForProperty(MP_Opacity)->Expression
		: nullptr;
#else
	UMaterialExpression* opacityExpression = pMaterial->Opacity.Expression;
#endif
	if (!opacityExpression)
	{
		UE_LOG(LogDazToUnreal, Warning, TEXT("Material %s has no connected Opacity expression."), *pMaterial->GetName());
		return false;
	}

	// Disconnect Opacity
#if UE_VERSION_NEWER_THAN(5,0,99)
	pMaterial->GetExpressionInputForProperty(MP_Opacity)->Expression = nullptr;
#else
	pMaterial->Opacity.Expression = nullptr;
#endif

	// Connect to Opacity Mask
#if UE_VERSION_NEWER_THAN(5,0,99)
	FExpressionInput* opacityMaskInput = pMaterial->GetExpressionInputForProperty(MP_OpacityMask);
	if (opacityMaskInput) {
		opacityMaskInput->Expression = opacityExpression;
	}
#else
	pMaterial->OpacityMask.Expression = opacityExpression;
#endif

	// Mark and save
	if (bSaveChanges)
	{
		pMaterial->Modify();
		pMaterial->PostEditChange();
		pMaterial->MarkPackageDirty();
		UE_LOG(LogDazToUnreal, Log, TEXT("Updated material %s: Translucent -> Masked."), *pMaterial->GetName());
	}

	return true;
}

#if UE_VERSION_NEWER_THAN(5,3,99)
#include "Materials/MaterialExpressionOneMinus.h"
#endif
bool FDazToUnrealUtils::ModifyMaterial_InvertOpacity(UMaterial* pMaterial, bool bSaveChanges)
{
	if (!pMaterial)
		return false;

	// Find existing opacity input connection
#if UE_VERSION_NEWER_THAN(5,0,99)
	UMaterialExpression* opacityExpression = pMaterial->GetExpressionInputForProperty(MP_Opacity)
		? pMaterial->GetExpressionInputForProperty(MP_Opacity)->Expression
		: nullptr;
#else
	UMaterialExpression* opacityExpression = pMaterial->Opacity.Expression;
#endif
	if (!opacityExpression)
	{
		UE_LOG(LogDazToUnreal, Warning, TEXT("Material %s has no connected Opacity expression."), *pMaterial->GetName());
		return false;
	}

	// Create OneMinus node
#if UE_VERSION_NEWER_THAN(5,4,99)
UMaterialExpressionOneMinus* oneMinusNode = Cast<UMaterialExpressionOneMinus>(
		UMaterialEditingLibrary::CreateMaterialExpression( pMaterial, UMaterialExpressionOneMinus::StaticClass(),
		opacityExpression->MaterialExpressionEditorX + 400,
		opacityExpression->MaterialExpressionEditorY));
#elif UE_VERSION_NEWER_THAN(5,0,99)
	UMaterialExpressionOneMinus* oneMinusNode = Cast<UMaterialExpressionOneMinus>(
		UMaterialEditingLibrary::CreateMaterialExpression(pMaterial, UMaterialExpressionOneMinus::StaticClass())
	);
#else
	UMaterialExpressionOneMinus* oneMinusNode = NewObject<UMaterialExpressionOneMinus>(pMaterial);
	pMaterial->Expressions.Add(oneMinusNode);
#endif
	oneMinusNode->MaterialExpressionEditorX = opacityExpression->MaterialExpressionEditorX + 400;
	oneMinusNode->MaterialExpressionEditorY = opacityExpression->MaterialExpressionEditorY;
	oneMinusNode->Input.Connect(0, opacityExpression);

	// Connect OneMinus output to Opacity
#if UE_VERSION_NEWER_THAN(5,0,99)
	FExpressionInput* opacityMaskInput = pMaterial->GetExpressionInputForProperty(MP_OpacityMask);
	if (opacityMaskInput) {
		opacityMaskInput->Expression = oneMinusNode;
	}
#else
	pMaterial->Opacity.Expression = oneMinusNode;
#endif

	// Mark and save
	if (bSaveChanges)
	{
		pMaterial->Modify();
		pMaterial->PostEditChange();
		pMaterial->MarkPackageDirty();
	}
	UE_LOG(LogDazToUnreal, Log, TEXT("Updated material %s: Translucent -> Masked and remapped Opacity to OneMinus->Opacity."), *pMaterial->GetName());

	return true;
}

#if UE_VERSION_NEWER_THAN(5,3,99)
#include "Engine/StaticMeshActor.h"
#endif
bool FDazToUnrealUtils::PlaceAssetInLevel(UObject* pAsset, const FString& sActorLabel, FVector vLocation, FRotator vRotation)
{
	// Spawn actor at origin adjusted for bottom offset
	UWorld* pWorld = GEditor->GetEditorWorldContext().World();
	if (!pWorld)
		return false;

	// if static mesh
	UStaticMesh* pStaticMesh = Cast<UStaticMesh>(pAsset);
	if (pStaticMesh) {
		AStaticMeshActor* pActor = pWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), vLocation, vRotation);
		if (pActor && pActor->GetStaticMeshComponent())
		{
			pActor->GetStaticMeshComponent()->SetStaticMesh(pStaticMesh);
			pActor->SetActorLabel(sActorLabel);
			UE_LOG(LogDazToUnreal, Log, TEXT("Placed %s at %s"), *sActorLabel, *vLocation.ToString());
		}
		else {
			UE_LOG(LogDazToUnreal, Warning, TEXT("PlaceAssetInLevel: Could not place StaticMeshActor for %s"), *sActorLabel);
			return false;
		}
	}
	// if skeletal mesh
	else {
		USkeletalMesh* pSkeletalMesh = Cast<USkeletalMesh>(pAsset);
		if (pSkeletalMesh) {
			ASkeletalMeshActor* pActor = pWorld->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), vLocation, vRotation);
			if (pActor && pActor->GetSkeletalMeshComponent())
			{
				pActor->GetSkeletalMeshComponent()->SetSkeletalMesh(pSkeletalMesh);
				pActor->SetActorLabel(sActorLabel);
				UE_LOG(LogDazToUnreal, Log, TEXT("Placed %s at %s"), *sActorLabel, *vLocation.ToString());
			}
			else {
				UE_LOG(LogDazToUnreal, Warning, TEXT("PlaceAssetInLevel: Could not place SkeletalMeshActor for %s"), *sActorLabel);
				return false;
			}
		}
		else {
			UE_LOG(LogDazToUnreal, Warning, TEXT("PlaceAssetInLevel: Unsupported asset type for %s"), *sActorLabel);
			return false;
		}
	}

	return true;
}

#include "EditorBuildUtils.h"
bool FDazToUnrealUtils::BakeLightingForCurrentMap()
{
	if (GEditor == nullptr)
	{
		UE_LOG(LogDazToUnreal, Error, TEXT("GEditor is null. Editor context required."));
		return false;
	}

	UWorld* pWorld = GEditor->GetEditorWorldContext().World();
	if (pWorld == nullptr)
	{
		UE_LOG(LogDazToUnreal, Error, TEXT("Editor world not found."));
		return false;
	}

	AWorldSettings* pWS = pWorld->GetWorldSettings();
	if (pWS && pWS->bForceNoPrecomputedLighting)
	{
		UE_LOG(LogDazToUnreal, Warning, TEXT("WorldSettings.bForceNoPrecomputedLighting is true. Lighting build will be skipped by the engine."));
		// TODO: Consider forcing false to bake lighting
	}

	UE_LOG(LogDazToUnreal, Log, TEXT("Starting lighting build for map: %s"), *pWorld->GetMapName());
	FEditorBuildUtils::EditorBuild(pWorld, FBuildOptions::BuildLighting, /* Show Dialog */ false);

	return true;
}

