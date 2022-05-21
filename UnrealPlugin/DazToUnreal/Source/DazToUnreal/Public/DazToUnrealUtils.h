#pragma once

#include "CoreMinimal.h"


class FDazToUnrealUtils
{
public:
	static FString SanitizeName(FString OriginalName);
	static bool MakeDirectoryAndCheck(FString& Directory);
	static bool IsModelFacingX(UObject* MeshObject);
};