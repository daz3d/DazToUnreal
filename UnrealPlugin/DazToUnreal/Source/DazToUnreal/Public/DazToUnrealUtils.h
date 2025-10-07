#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "DazToUnrealEnums.h"

// forward declaration
enum class EDazMaterialType: uint8;

class FDazToUnrealUtils
{
public:
	static FString SanitizeName(FString OriginalName);
	static bool MakeDirectoryAndCheck(FString& Directory);
	static bool IsModelFacingX(UObject* MeshObject);
	static FString GetDTUPathForModel(FSoftObjectPath MeshObjectPath);
	static FSoftObjectPath GetSkeletonForImport(const DazToUnrealImportData& DazImportData);

	static void InstallPluginContentToProject();
	static void DuplicatePluginAsset(FName AssetPathInPlugin, const FString& DestPackagePath);
	static FString InstallCommonMaterialFromPlugin(FString PluginMaterialPath);

	static FSoftObjectPath FindMaterial(FString ShaderName, EDazMaterialType MaterialType);
	static void MoveSingleAsset(const FString& SourcePath, const FString& DestinationFolder);
	static void MakeNewFabLevel(const FString& NewMapPath);
	static void SaveNewFabLevel(const FString& NewMapPath);
	static void ReplaceSkeleton(FString AnimPath, FString SkeletonPath);
	static void AssignSkeletalMeshToActor(USkeletalMesh* pMesh);

private:
	static bool IsSkeletonUsed(FSoftObjectPath SkeletonPath);
};