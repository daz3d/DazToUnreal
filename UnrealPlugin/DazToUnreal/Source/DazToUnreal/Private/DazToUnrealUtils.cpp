#include "DazToUnrealUtils.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"
#include "Engine/SkeletalMesh.h"


#include "Engine/StaticMesh.h"
#include "EditorFramework/AssetImportData.h"
#include "Factories/FbxAssetImportData.h"

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
		.Replace(TEXT(":"), TEXT("_"));
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
