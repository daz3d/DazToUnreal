#include "DazToUnrealMaterialPack.h"
#include "UObject/SoftObjectPath.h"

FDazMaterialMappingInfo::FDazMaterialMappingInfo()
{

}


UDazToUnrealMaterialPack::UDazToUnrealMaterialPack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FSoftObjectPath UDazToUnrealMaterialPack::FindMaterial(FString ShaderName, EDazMaterialType MaterialType)
{
	for (FDazMaterialMappingInfo MaterialInfo : Materials)
	{
		if (MaterialInfo.DazShaderName.Compare(ShaderName, ESearchCase::IgnoreCase) == 0 &&
			MaterialInfo.MaterialType == MaterialType)
		{
			return MaterialInfo.MaterialPath;
		}
	}
	return FSoftObjectPath();
}