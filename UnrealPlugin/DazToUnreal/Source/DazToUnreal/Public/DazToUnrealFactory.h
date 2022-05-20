#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "DazToUnrealFactory.generated.h"

// The Daz Studio plugin creates a dtu file with the same string that's send over the network.  This factory allows that file to be imported to start the same process.
UCLASS()
class UDazToUnrealFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:

	// UFactory interface
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	// End of UFactory interface
};
