#include "DazToUnrealTextures.h"
#include "DazToUnrealSettings.h"

// Gets a texture specified in the settings for sub sarface aplha for a specified character and body part.
FString FDazToUnrealTextures::GetSubSurfaceAlphaTexture(const DazCharacterType CharacterType, const FString& MaterialName)
{
	const UDazToUnrealSettings* CachedSettings = GetDefault<UDazToUnrealSettings>();
	FString Seperator; 
	if ( CachedSettings->UseOriginalMaterialName)
	{
		Seperator = "";
	}
	else
	{
		Seperator = "_";
	}
	if (CharacterType == DazCharacterType::Genesis8Male)
	{

		if (MaterialName.EndsWith(Seperator + TEXT("Arms")) ||
			MaterialName.EndsWith(Seperator + TEXT("Fingernails")))
		{
			return CachedSettings->ArmsSubSurfaceOpacityGenesis8MaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Face")) ||
			MaterialName.EndsWith(Seperator + TEXT("Lips")) ||
			MaterialName.EndsWith(Seperator + TEXT("Ears")))
		{
			return CachedSettings->FaceSubSurfaceOpacityGenesis8MaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Torso")) ||
			MaterialName.EndsWith(Seperator + TEXT("Genitalia")))
		{
			return CachedSettings->TorsoSubSurfaceOpacityGenesis8MaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Legs")) ||
			MaterialName.EndsWith(Seperator + TEXT("ToeNails")))
		{
			return CachedSettings->LegsSubSurfaceOpacityGenesis8MaleTexture.ToString();
		}
	}

	if (CharacterType == DazCharacterType::Genesis8Female)
	{
		if (MaterialName.EndsWith(Seperator + TEXT("Arms")) ||
			MaterialName.EndsWith(Seperator + TEXT("Fingernails")))
		{
			return CachedSettings->ArmsSubSurfaceOpacityGenesis8FemaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Face")) ||
			MaterialName.EndsWith(Seperator + TEXT("Lips")) ||
			MaterialName.EndsWith(Seperator + TEXT("Ears")))
		{
			return CachedSettings->FaceSubSurfaceOpacityGenesis8FemaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Torso")) ||
			MaterialName.EndsWith(Seperator + TEXT("Genitalia")))
		{
			return CachedSettings->TorsoSubSurfaceOpacityGenesis8FemaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Legs")) ||
			MaterialName.EndsWith(Seperator + TEXT("ToeNails")))
		{
			return CachedSettings->LegsSubSurfaceOpacityGenesis8FemaleTexture.ToString();
		}
	}

	if (CharacterType == DazCharacterType::Genesis3Male)
	{
		if (MaterialName.EndsWith(Seperator + TEXT("Arms")) ||
			MaterialName.EndsWith(Seperator + TEXT("Fingernails")))
		{
			return CachedSettings->ArmsSubSurfaceOpacityGenesis3MaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Face")) ||
			MaterialName.EndsWith(Seperator + TEXT("Lips")) ||
			MaterialName.EndsWith(Seperator + TEXT("Ears")))
		{
			return CachedSettings->FaceSubSurfaceOpacityGenesis3MaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Torso")) ||
			MaterialName.EndsWith(Seperator + TEXT("Genitalia")))
		{
			return CachedSettings->TorsoSubSurfaceOpacityGenesis3MaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Legs")) ||
			MaterialName.EndsWith(Seperator + TEXT("ToeNails")))
		{
			return CachedSettings->LegsSubSurfaceOpacityGenesis3MaleTexture.ToString();
		}
	}

	if (CharacterType == DazCharacterType::Genesis3Female)
	{
		if (MaterialName.EndsWith(Seperator + TEXT("Arms")) ||
			MaterialName.EndsWith(Seperator + TEXT("Fingernails")))
		{
			return CachedSettings->ArmsSubSurfaceOpacityGenesis3FemaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Face")) ||
			MaterialName.EndsWith(Seperator + TEXT("Lips")) ||
			MaterialName.EndsWith(Seperator + TEXT("Ears")))
		{
			return CachedSettings->FaceSubSurfaceOpacityGenesis3FemaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Torso")) ||
			MaterialName.EndsWith(Seperator + TEXT("Genitalia")))
		{
			return CachedSettings->TorsoSubSurfaceOpacityGenesis3FemaleTexture.ToString();
		}
		if (MaterialName.EndsWith(Seperator + TEXT("Legs")) ||
			MaterialName.EndsWith(Seperator + TEXT("ToeNails")))
		{
			return CachedSettings->LegsSubSurfaceOpacityGenesis3FemaleTexture.ToString();
		}
	}
	return TEXT("");
}