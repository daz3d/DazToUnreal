#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDazToUnrealRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static inline FDazToUnrealRuntimeModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FDazToUnrealRuntimeModule >("DazToUnrealRuntime");
	}


};