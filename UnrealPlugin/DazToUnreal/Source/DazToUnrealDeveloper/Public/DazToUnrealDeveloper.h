#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDazToUnrealDeveloperModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static inline FDazToUnrealDeveloperModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FDazToUnrealDeveloperModule >("DazToUnrealDeveloper");
	}


};