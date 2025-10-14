#define VODSVERSION 0

#include "DazToUnreal.h"
#include "DazToUnrealSettings.h"
#include "DazToUnrealStyle.h"
#include "DazToUnrealCommands.h"
#include "DazToUnrealMaterials.h"
#include "DazToUnrealUtils.h"
#include "DazToUnrealFbx.h"
#include "DazToUnrealEnvironment.h"
#include "DazToUnrealPoses.h"
#include "DazToUnrealSubdivision.h"
#include "DazToUnrealMorphs.h"
#include "DazToUnrealMLDeformer.h"
#include "DazToUnrealBlueprintUtils.h"

#include "EditorLevelLibrary.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Engine/ObjectLibrary.h"
#include "Factories/TextureFactory.h"
#include "ObjectTools.h"
#include "Engine/Texture2D.h"
#include "Utils.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "FbxImporter.h"
#include "AssetImportTask.h"
#include "Factories/MaterialImportHelpers.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Interfaces/IPluginManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "EditorAssetLibrary.h"
#include "Misc/EngineVersion.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "FileHelpers.h"
#include "Async/Async.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/PoseAsset.h"
#include "Rendering/SkeletalMeshModel.h"
#include "ToolMenuSection.h"
#include "ContentBrowserMenuContexts.h"
#include "Animation/PoseAsset.h"
#include "ShaderCompiler.h"

// REQUIRED TO USE UE_VERSION_NEWER_THAN
#include "Misc/EngineVersionComparison.h"

#if !(ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25)
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
#include "LevelEditorSubsystem.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION == 2
#include "IKRigDefinition.h"
#endif

#include "IMeshReductionInterfaces.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
#include "Engine/SkinnedAssetCommon.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 2
#include "Rig/IKRigDefinition.h"
#endif

DEFINE_LOG_CATEGORY(LogDazToUnreal);
//#include "ISkeletonEditorModule.h"
//#include "IEditableSkeleton.h"

// NOTE: This FBX include code was copied from FbxImporter.h
// Temporarily disable a few warnings due to virtual function abuse in FBX source files
#pragma warning( push )

#pragma warning( disable : 4263 ) // 'function' : member function does not override any base class virtual member function
#pragma warning( disable : 4264 ) // 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden

// Include the fbx sdk header
// temp undef/redef of _O_RDONLY because kfbxcache.h (included by fbxsdk.h) does
// a weird use of these identifiers inside an enum.
#ifdef _O_RDONLY
#define TMP_UNFBX_BACKUP_O_RDONLY _O_RDONLY
#define TMP_UNFBX_BACKUP_O_WRONLY _O_WRONLY
#undef _O_RDONLY
#undef _O_WRONLY
#endif

//Robert G. : Packing was only set for the 64bits platform, but we also need it for 32bits.
//This was found while trying to trace a loop that iterate through all character links.
//The memory didn't match what the debugger displayed, obviously since the packing was not right.
#pragma pack(push,8)

#if PLATFORM_WINDOWS
// _CRT_SECURE_NO_DEPRECATE is defined but is not enough to suppress the deprecation
// warning for vsprintf and stricmp in VS2010.  Since FBX is able to properly handle the non-deprecated
// versions on the appropriate platforms, _CRT_SECURE_NO_DEPRECATE is temporarily undefined before
// including the FBX headers

// The following is a hack to make the FBX header files compile correctly under Visual Studio 2012 and Visual Studio 2013
#if _MSC_VER >= 1700
#define FBX_DLL_MSC_VER 1600
#endif


#endif // PLATFORM_WINDOWS

// FBX casts null pointer to a reference
THIRD_PARTY_INCLUDES_START
#include <fbxsdk.h>
THIRD_PARTY_INCLUDES_END

#pragma pack(pop)

#ifdef TMP_UNFBX_BACKUP_O_RDONLY
#define _O_RDONLY TMP_FBX_BACKUP_O_RDONLY
#define _O_WRONLY TMP_FBX_BACKUP_O_WRONLY
#undef TMP_UNFBX_BACKUP_O_RDONLY
#undef TMP_UNFBX_BACKUP_O_WRONLY
#endif

#pragma warning( pop )
// end of fbx include

//static const FName DazToUnrealTabName("DazToUnreal");

#define LOCTEXT_NAMESPACE "FDazToUnrealModule"

FString FDazToUnrealModule::OverrideConversionDestPath;
int FDazToUnrealModule::BatchConversionMode;
FString FDazToUnrealModule::BatchConversionDestPath;
TMap<FString, FString> FDazToUnrealModule::AssetIDLookup;
TArray<UObject*> FDazToUnrealModule::TextureListToDisableSRGB;

void FDazToUnrealModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FDazToUnrealStyle::Initialize();
	FDazToUnrealStyle::ReloadTextures();

	FDazToUnrealCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	/*PluginCommands->MapAction(
		FDazToUnrealCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FDazToUnrealModule::PluginButtonClicked),
		FCanExecuteAction());*/

	PluginCommands->MapAction(
		FDazToUnrealCommands::Get().InstallDazStudioPlugin,
		FExecuteAction::CreateRaw(this, &FDazToUnrealModule::InstallDazStudioPlugin),
		FCanExecuteAction());

	/*PluginCommands->MapAction(
		FDazToUnrealCommands::Get().InstallSkeletonAssets,
		FExecuteAction::CreateRaw(this, &FDazToUnrealModule::InstallSkeletonAssetsToProject),
		FCanExecuteAction());*/

		/*PluginCommands->MapAction(
			FDazToUnrealCommands::Get().InstallMaterialAssets,
			FExecuteAction::CreateRaw(this, &FDazToUnrealModule::InstallMaterialAssetsToProject),
			FCanExecuteAction());*/

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FDazToUnrealModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FDazToUnrealModule::AddToolbarExtension));


		FString InstallerPath = IPluginManager::Get().FindPlugin("DazToUnreal")->GetBaseDir() / TEXT("Resources") / TEXT("DazToUnrealSetup.exe");
		FString InstallerAbsolutePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*InstallerPath);

		FString DazStudioPluginPath = IPluginManager::Get().FindPlugin("DazToUnreal")->GetBaseDir() / TEXT("Resources") / TEXT("dzunrealbridge.dll");
		FString DazStudioPluginAbsolutePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*InstallerPath);

		if (FPaths::FileExists(InstallerAbsolutePath)
			&& FPaths::FileExists(DazStudioPluginAbsolutePath))
		{
			LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		}
	}

	AddCreateRetargeterMenu();
	AddCreateFullBodyIKControlRigMenu();
	AddCreateIKLimbBasedControlRigMenu();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 3
	AddConvertToEpicSkeletonMenu();
#endif

	/*FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DazToUnrealTabName, FOnSpawnTab::CreateRaw(this, &FDazToUnrealModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FDazToUnrealTabTitle", "DazToUnreal"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);*/

	// DB Dec-21-2021: Start in Batch Conversion Mode, if autoexec-batch-file detected.
	FString jobPoolFilename = FPaths::ProjectDir() / TEXT("autoexec-jobpool.txt");
	if (FPaths::FileExists(jobPoolFilename))
	{
		BatchConversionMode = 1;
	}
	else
	{
		BatchConversionMode = 0;
	}
	StartupUDPListener();

}

void FDazToUnrealModule::ShutdownModule()
{
	 // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	 // we call this function before unloading the module.
	 FDazToUnrealStyle::Shutdown();

	 FDazToUnrealCommands::Unregister();

	 //FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DazToUnrealTabName);
}

/*TSharedRef<SDockTab> FDazToUnrealModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FDazToUnrealModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("DazToUnreal.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}*/

//void FDazToUnrealModule::PluginButtonClicked()
//{
	//FGlobalTabmanager::Get()->InvokeTab(DazToUnrealTabName);
//}

void FDazToUnrealModule::AddMenuExtension(FMenuBuilder& Builder)
{
	 Builder.AddMenuEntry(FDazToUnrealCommands::Get().OpenPluginWindow);
}

void FDazToUnrealModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	 //Builder.AddToolBarButton(FDazToUnrealCommands::Get().OpenPluginWindow);
	 Builder.AddComboButton(
		  FUIAction(),
		  FOnGetContent::CreateRaw(this, &FDazToUnrealModule::MakeDazToUnrealToolbarMenu, PluginCommands),
		  LOCTEXT("DazToUnrealCombo", "DazToUnreal"),
		  LOCTEXT("DazToUnrealCombo_ToolTip", "Actions for DazToUnreal"),
		  FSlateIcon(FDazToUnrealStyle::GetStyleSetName(), "DazToUnreal.ToolBar"),
		  false,
		  "LevelToolbarQuickSettings"
	 );
}

TSharedRef<SWidget> FDazToUnrealModule::MakeDazToUnrealToolbarMenu(TSharedPtr<FUICommandList> DazToUnrealCommandList)
{
	 FMenuBuilder MenuBuilder(true, DazToUnrealCommandList);

	 MenuBuilder.AddMenuEntry(FDazToUnrealCommands::Get().InstallDazStudioPlugin);
	 //MenuBuilder.AddMenuEntry(FDazToUnrealCommands::Get().InstallSkeletonAssets);
	 //MenuBuilder.AddMenuEntry(FDazToUnrealCommands::Get().InstallMaterialAssets);

	 return MenuBuilder.MakeWidget();
}

void FDazToUnrealModule::StartupUDPListener()
{
	 const UDazToUnrealSettings* CachedSettings = GetDefault<UDazToUnrealSettings>();

	 if (BatchConversionMode == 0)
	 {
		FIPv4Endpoint Endpoint(FIPv4Address::InternalLoopback, CachedSettings->Port);
		ServerSocket = FUdpSocketBuilder(TEXT("DazToUnrealServerSocket"))
			.AsNonBlocking()
			.AsReusable()
			.BoundToEndpoint(Endpoint);
	 }

	 TickDelegate = FTickerDelegate::CreateRaw(this, &FDazToUnrealModule::Tick);
#if ENGINE_MAJOR_VERSION > 4
	 TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate, 1.0f);
#else
	 TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate, 1.0f);
#endif
}
void FDazToUnrealModule::ShutdownUDPListener()
{
	 if (ServerSocket != nullptr)
	 {
		  ServerSocket->Close();
		  ServerSocket = nullptr;
	 }
}

bool FDazToUnrealModule::Tick(float DeltaTime)
{

	if (BatchConversionMode == 1)
	{
		BatchConversionMode = -1;

		// DB Dec-21-2021: Start in Batch Conversion Mode, if autoexec-batch-file detected.
		FString jobPoolFilename = FPaths::ProjectDir() / TEXT("autoexec-jobpool.txt");
		if (FPaths::FileExists(jobPoolFilename))
		{
			TArray<FString> jobPool;
			TArray<TSharedPtr<FJsonObject>> environmentQueue;

			FFileHelper::LoadFileToStringArray(jobPool, *jobPoolFilename);

			if (jobPool.Num() > 0) {
				FString sFirstJob = jobPool[0];
				if (sFirstJob.EndsWith(TEXT(".fbx"))) {
					BatchConversionMode = 3; // FBX only mode
					return true;
				}
			}

			for (int i = 0; i < jobPool.Num(); i++)
			{
				FString DtuFile = jobPool[i];
				// get containing folder
				FString sourcePath = FPaths::GetPath(DtuFile);
				FString containingFolder = FPaths::GetPathLeaf(sourcePath);
				BatchConversionDestPath = TEXT("BatchConversions") / containingFolder;

				FString Json;
				FFileHelper::LoadFileToString(Json, *DtuFile);
				TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Json);
				TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
				if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
				{
					if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("Environment"))
					{
						// move to environmentQueue
						environmentQueue.Add(JsonObject);
					}
					else
					{
						ImportFromDaz(JsonObject, DtuFile);
					}
				}
			}
			for (int i = 0; i < environmentQueue.Num(); i++)
			{
				ImportFromDaz(environmentQueue[i], jobPool[i]);
			}

		}
		BatchConversionMode = 2;
	}
	else if (BatchConversionMode == 0)
	{
		// DB 2023-May-23: Disable SRGB in a delayed step after importing textures is done to avoid engine crash
		if (FDazToUnrealModule::TextureListToDisableSRGB.Num() > 0)
		{
			for (int i = 0; i < FDazToUnrealModule::TextureListToDisableSRGB.Num(); i++)
			{
				// cast element to UTexture
				UTexture* Texture = Cast<UTexture>(FDazToUnrealModule::TextureListToDisableSRGB[i]);
				if (Texture)
				{
					Texture->PreEditChange(nullptr);
					Texture->SRGB = false;
					Texture->PostEditChange();
				}
			}
			FDazToUnrealModule::TextureListToDisableSRGB.Empty();
		}

		// Check from messages from the Daz Studio plugin
		uint32 BytesPending = 0;
		if (ServerSocket->HasPendingData(BytesPending))
		{
			uint8 Data[2048];
			ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
			TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();

			// Parse the message
			int32 BytesRead = 0;
			if (ServerSocket->RecvFrom(Data, sizeof(Data), BytesRead, *Sender))
			{
				char CharData[2048];
				memcpy(CharData, Data, BytesRead);
				CharData[BytesRead] = 0;

				FString FileName = ANSI_TO_TCHAR(CharData);
				if (FPaths::FileExists(FileName))
				{
					FString Json;
					FFileHelper::LoadFileToString(Json, *FileName);
					TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Json);
					TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
					if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
					{
						// In UE5 the ticker can happen on worker threads, but some import processes want the game (main) thread.
						//AsyncTask(ENamedThreads::GameThread, [this, JsonObject]() {
							ImportFromDaz(JsonObject, FileName);
							//});
						
					}
					else
					{
						UE_LOG(LogDazToUnreal, Warning, TEXT("DazToUnreal: ERROR: Unable to parse DTU File: %s"), *FileName);
					}
				}
				else
				{
					UE_LOG(LogDazToUnreal, Warning, TEXT("DazToUnreal: ERROR: Unable to find DTU file: %s"), *FileName);
				}
			}
		}

	}
	else if (BatchConversionMode == 2)
	{
		// wait for lighting bake to complete
		if (GEditor->IsLightingBuildCurrentlyRunning()) {
			// still baking
		} else {
			FDazToUnrealUtils::SaveCurrentLevel();
			BatchConversionMode = 100;
		}		
	}
	else if (BatchConversionMode == 100)
	{
		// Exit when batch conversion complete
		FEditorFileUtils::SaveDirtyPackages(false,false,true);
		FGenericPlatformMisc::RequestExit(false);
		// rename the autoexec-jobpool.txt to autoexec-jobpool.txt.done
		FString jobPoolFilename = FPaths::ProjectDir() / TEXT("autoexec-jobpool.txt");
		FString jobPoolDoneFilename = FPaths::ProjectDir() / TEXT("autoexec-jobpool.txt.done");
		if (FPaths::FileExists(jobPoolFilename))
		{
			IFileManager::Get().Move(*jobPoolDoneFilename, *jobPoolFilename);
		}
	}
	else if (BatchConversionMode == 3)
	{
		UE_LOG(LogDazToUnreal, Log, TEXT("DazToUnreal: FBX Only Batch Conversion Mode"));

		FString jobPoolFilename = FPaths::ProjectDir() / TEXT("autoexec-jobpool.txt");
		if (FPaths::FileExists(jobPoolFilename))
		{
			TArray<FString> jobPool;
			TArray<TSharedPtr<FJsonObject>> environmentQueue;
			FFileHelper::LoadFileToStringArray(jobPool, *jobPoolFilename);

			for (int i = 0; i < jobPool.Num(); i++)
			{
				FString FbxImportPath = jobPool[i];
				// do structured import
				UE_LOG(LogDazToUnreal, Log, TEXT("DazToUnreal: Importing FBX: %s"), *FbxImportPath);
				ImportFbxForFab(FbxImportPath, TEXT("/Game/") / FPaths::GetBaseFilename(FbxImportPath));
			}
		}
		BatchConversionMode = 2;

	}

	return true;
}

UObject* FDazToUnrealModule::ImportFromDaz(TSharedPtr<FJsonObject> JsonObject, const FString& FileName)
{
	FScopedSlowTask Progress(10.0f, LOCTEXT("CreatingAutoJCMControlRig", "Importing from Daz"));
	Progress.MakeDialog();
	const UDazToUnrealSettings* CachedSettings = GetDefault<UDazToUnrealSettings>();

	 TMap<FString, TArray<FDUFTextureProperty>> DtuMaterialsTable;

	 FString FBXPath = JsonObject->GetStringField(TEXT("FBX File"));
	 FString BaseFBXPath = JsonObject->GetStringField(TEXT("Base FBX File"));
	 FString HDFBXPath = JsonObject->GetStringField(TEXT("HD FBX File"));
	 FString AssetName = FDazToUnrealUtils::SanitizeName(JsonObject->GetStringField(TEXT("Asset Name")));
	 if (JsonObject->GetStringField(TEXT("Product Component Name")) != "")
		AssetName = FDazToUnrealUtils::SanitizeName(JsonObject->GetStringField(TEXT("Product Component Name")));
	 FString ImportFolder = JsonObject->GetStringField(TEXT("Import Folder"));
	 DazAssetType AssetType = DazAssetType::StaticMesh;
	 if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("SkeletalMesh"))
		 AssetType = DazAssetType::SkeletalMesh;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("StaticMesh"))
		 AssetType = DazAssetType::StaticMesh;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("Animation"))
		 AssetType = DazAssetType::Animation;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("Environment"))
		 AssetType = DazAssetType::Environment;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("Pose"))
		 AssetType = DazAssetType::Pose;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("MLDeformer"))
		 AssetType = DazAssetType::MLDeformer;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("R2x"))
		 AssetType = DazAssetType::R2x;
	 else if (JsonObject->GetStringField(TEXT("Asset Type")) == TEXT("SkeletalMesh_v2"))
		 AssetType = DazAssetType::SkeletalMesh_v2;
	 else
		 AssetType = DazAssetType::UNKNOWN;

	 bool UseExperimentalAnimationTransfer = false;
	 if (JsonObject->HasField(TEXT("Use Experimental Animation Transfer")))
	 {
		 UseExperimentalAnimationTransfer = JsonObject->GetBoolField(TEXT("Use Experimental Animation Transfer"));
	 }

	 DazMaterialCombineType MaterialCombineMethod = CachedSettings->CombineIdenticalMaterials ? DazMaterialCombineType::CombineIdentical : DazMaterialCombineType::NoCombine;
	 if (JsonObject->HasField(TEXT("MaterialCombineMethod")))
	 {
		 FString MaterialCombineName = JsonObject->GetStringField(TEXT("MaterialCombineMethod"));
		 if (MaterialCombineName.Compare(TEXT("No Combine"), ESearchCase::IgnoreCase) == 0)
		 {
			 MaterialCombineMethod = DazMaterialCombineType::NoCombine;
		 }
		 if (MaterialCombineName.Compare(TEXT("Combine Identical"), ESearchCase::IgnoreCase) == 0)
		 {
			 MaterialCombineMethod = DazMaterialCombineType::CombineIdentical;
		 }
		 if (MaterialCombineName.Compare(TEXT("Combine All"), ESearchCase::IgnoreCase) == 0)
		 {
			 MaterialCombineMethod = DazMaterialCombineType::CombineAll;
		 }
	 }

	 // Build AssetIDLookup
	 FString AssetID = JsonObject->GetStringField(TEXT("Asset ID"));
	 if (!AssetIDLookup.Contains(AssetID))
	 {
		 AssetIDLookup.Add(AssetID, AssetName);
	 }

	 // Set up the folder paths
	 FString ImportDirectory = FPaths::ProjectDir() / TEXT("Import");
	 if (!ImportFolder.IsEmpty())
	 {
		  ImportDirectory = ImportFolder;
	 }

	 FString ImportCharacterFolder = FPaths::GetPath(FBXPath);
	 FString ImportCharacterTexturesFolder = FPaths::GetPath(FBXPath) / TEXT("Textures");
	 FString ImportCharacterMaterialFolder = FPaths::GetPath(FBXPath) / TEXT("Materials");
	 FString FBXFile = FBXPath;
	 FString BaseFBXFile = BaseFBXPath;
	 FString HDFBXFile = HDFBXPath;

	 FString DAZImportFolder = CachedSettings->ImportDirectory.Path;
	 FString DAZAnimationImportFolder = CachedSettings->AnimationImportDirectory.Path;
	 FString DazMLDeformerImportFolder = CachedSettings->DeformerImportDirectory.Path;
	 FString CharacterFolder = DAZImportFolder / AssetName;
	 FString CharacterTexturesFolder = CharacterFolder / TEXT("Textures");
	 FString CharacterMaterialFolder = CharacterFolder / TEXT("Materials");
	 if (BatchConversionMode != 0)
	 {
		//  CharacterFolder = DAZImportFolder / BatchConversionDestPath;
		//  CharacterTexturesFolder = DAZImportFolder / BatchConversionDestPath / TEXT("Textures");
		//  CharacterMaterialFolder = DAZImportFolder / BatchConversionDestPath / TEXT("Materials");

		// Fab overrides
		DAZImportFolder = TEXT("/Game/") / AssetName;
		OverrideConversionDestPath = DAZImportFolder;
		CharacterFolder = DAZImportFolder / TEXT("Mesh");
		CharacterTexturesFolder = DAZImportFolder / TEXT("Textures");
		CharacterMaterialFolder = DAZImportFolder / TEXT("Materials");
	}

	 IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	 FString ContentDirectory = FPaths::ProjectContentDir();

	 FString LocalDAZImportFolder = DAZImportFolder.Replace(TEXT("/Game/"), *ContentDirectory);
	 FString LocalDAZAnimationImportFolder = DAZAnimationImportFolder.Replace(TEXT("/Game/"), *ContentDirectory);
	 FString LocalDAZMLDeformerImportFolder = DazMLDeformerImportFolder.Replace(TEXT("/Game/"), *ContentDirectory);
	 FString LocalCharacterFolder = CharacterFolder.Replace(TEXT("/Game/"), *ContentDirectory);
	 FString LocalCharacterTexturesFolder = CharacterTexturesFolder.Replace(TEXT("/Game/"), *ContentDirectory);
	 FString LocalCharacterMaterialFolder = CharacterMaterialFolder.Replace(TEXT("/Game/"), *ContentDirectory);

	 // Make common required folders.  If any of these fail, don't continue
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(ImportDirectory)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(ImportCharacterFolder)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(ImportCharacterTexturesFolder)) return nullptr;
#if VODSVERSION
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(DAZImportFolder)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(CharacterFolder)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(CharacterTexturesFolder)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(CharacterMaterialFolder)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(DAZAnimationImportFolder)) return nullptr;
	 if (!FDazToUnrealUtils::MakeDirectoryAndCheck(DazMLDeformerImportFolder)) return nullptr;
#else
	if (BatchConversionMode == 0)
	{
		if (!FDazToUnrealUtils::MakeDirectoryAndCheck(LocalDAZImportFolder)) return nullptr;
		if (!FDazToUnrealUtils::MakeDirectoryAndCheck(LocalCharacterFolder)) return nullptr;
		if (!FDazToUnrealUtils::MakeDirectoryAndCheck(LocalCharacterTexturesFolder)) return nullptr;
		if (!FDazToUnrealUtils::MakeDirectoryAndCheck(LocalCharacterMaterialFolder)) return nullptr;
		if (AssetType != DazAssetType::R2x) {
			if (!FDazToUnrealUtils::MakeDirectoryAndCheck(LocalDAZAnimationImportFolder)) return nullptr;
			if (!FDazToUnrealUtils::MakeDirectoryAndCheck(LocalDAZMLDeformerImportFolder)) return nullptr;
		}
	}
#endif
	// Make asset type specific folders.  If any of these fail, don't continue

	bool placeholder_cachesetting_installcommon = true;
	if (AssetType == DazAssetType::R2x || placeholder_cachesetting_installcommon) {
		FDazToUnrealUtils::InstallPluginContentToProject();
	}


	 // If there's an HD FBX File, that's the source
	 if (FPaths::FileExists(HDFBXFile))
	 {
		 FBXFile = HDFBXFile;
	 }

	 // Setup Import Data
	 DazToUnrealImportData ImportData;
	 ImportData.SourcePath = FBXFile;
	 ImportData.ImportLocation = CharacterFolder;
	 ImportData.AssetType = AssetType;
	 ImportData.CharacterTypeName = AssetID;
	 JsonObject->TryGetBoolField(TEXT("CreateUniqueSkeleton"), ImportData.bCreateUniqueSkeleton);
	 JsonObject->TryGetBoolField(TEXT("FixTwistBones"), ImportData.bFixTwistBones);
	 JsonObject->TryGetBoolField(TEXT("ConvertToEpicSkeleton"), ImportData.bConvertToEpicSkeleton);
	 if (ImportData.bConvertToEpicSkeleton)
	 {
		 ImportData.bCreateUniqueSkeleton = true;
		 ImportData.bFixTwistBones = true;
	 }
	 if (!JsonObject->TryGetBoolField(TEXT("FaceCharacterRight"), ImportData.bFaceCharacterRight))
	 {
		 ImportData.bFaceCharacterRight = CachedSettings->ZeroRootRotationOnImport;
	 }

	 if (AssetType == DazAssetType::Environment)
	 {
		 FString LevelPath = CharacterFolder / AssetName + FString("_Level");
		 FString TemplatePath = TEXT("/Engine/Content/Maps/Templates/Template_Default");
#if UE_VERSION_NEWER_THAN(5, 0, 99)
		 if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
		 {
			 LevelEditorSubsystem->NewLevelFromTemplate(LevelPath, TemplatePath);
			 //LevelEditorSubsystem->NewLevel(LevelPath);

			 // DB - UE 5.x appears to need LoadLevel() after using one of the NewLevel___() functions
			 LevelEditorSubsystem->LoadLevel(LevelPath);
		 }
#else
		 UEditorLevelLibrary::NewLevelFromTemplate(LevelPath, TemplatePath);
		 //UEditorLevelLibrary::NewLevel(LevelPath);
#endif
		 FDazToUnrealEnvironment::ImportEnvironment(JsonObject);
#if UE_VERSION_NEWER_THAN(5, 0, 99)
		 if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
		 {
			 LevelEditorSubsystem->SaveCurrentLevel();
		 }
#else
		 UEditorLevelLibrary::SaveCurrentLevel();
#endif
		 return nullptr;
	 }

	 // Get a list of Pose name mappings
	 TArray<FString> PoseNameList;
	 const TArray<TSharedPtr<FJsonValue>>* PoseList;
	 if (JsonObject->TryGetArrayField(TEXT("Poses"), PoseList))
	 {
		 PoseNameList.Add(TEXT("ReferencePose"));
		 for (int32 i = 0; i < PoseList->Num(); i++)
		 {
			 TSharedPtr<FJsonObject> Pose = (*PoseList)[i]->AsObject();
			 FString PoseName = Pose->GetStringField(TEXT("Name"));
			 FString PoseLabel = Pose->GetStringField(TEXT("Label"));

			 PoseNameList.Add(PoseLabel);
		 }
	 }

	 FDazToUnrealMLDeformerParams DazToUnrealMLDeformerParams;
	 if (((AssetType == DazAssetType::Animation || AssetType == DazAssetType::Pose) && UseExperimentalAnimationTransfer) || AssetType == DazAssetType::MLDeformer)
	 {
		 DazCharacterType CharacterType = DazCharacterType::Unknown;
		 if (AssetID.StartsWith(TEXT("Genesis3")))
		 {
			 CharacterType = DazCharacterType::Genesis3Male;
		 }
		 else if (AssetID.StartsWith(TEXT("Genesis8")))
		 {
			 CharacterType = DazCharacterType::Genesis8Male;
		 }
		 else if (AssetID == TEXT("Genesis"))
		 {
			 CharacterType = DazCharacterType::Genesis1;
		 }
		 ImportData.SourcePath = FBXPath;
		 ImportData.ImportLocation = DAZAnimationImportFolder;
		 ImportData.CharacterType = CharacterType;
		 UObject* NewAnimation = ImportFBXAsset(ImportData);

		 // If this is a Pose transfer, an AnimSequence was created.  Make a PoseAsset from it.
		 if (AssetType == DazAssetType::Pose)
		 {
			 if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(NewAnimation))
			 {
				 UPoseAsset* NewPoseAsset = FDazToUnrealPoses::CreatePoseAsset(AnimSequence, PoseNameList);

				 FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				 TArray<UObject*> AssetsToSelect;
				 AssetsToSelect.Add((UObject*)NewPoseAsset);
				 ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSelect);
				 return Cast<UObject>(NewPoseAsset);
			 }
		 }

		 if (AssetType != DazAssetType::MLDeformer)
		 {
			 return NewAnimation;
		 }
		 DazToUnrealMLDeformerParams.AnimationAsset = Cast<UAnimSequence>(NewAnimation);
	 }

	 if (AssetType == DazAssetType::MLDeformer)
	 {
		 DazToUnrealMLDeformerParams.JsonImportData = JsonObject;
		 DazToUnrealMLDeformerParams.ImportData = ImportData;
		 FDazToUnrealMLDeformer::ImportMLDeformerAssets(DazToUnrealMLDeformerParams);

		 DazToUnrealMLDeformerParams.JsonImportData = JsonObject;
		 if (DazToUnrealMLDeformerParams.OutAsset)
		 {
			 FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			 TArray<UObject*> AssetsToSelect;
			 AssetsToSelect.Add(DazToUnrealMLDeformerParams.OutAsset);
			 ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSelect);
			 return DazToUnrealMLDeformerParams.OutAsset;
		 }
		 return nullptr;
	 }

	 // If there isn't an FBX file, stop
	 if (!FPaths::FileExists(FBXFile))
	 {
		 	UE_LOG(LogDazToUnreal, Warning, TEXT("DazToUnreal: ERROR: Unable to load FBXFile: %s"), *FBXFile);
		  return nullptr;
	 }

	 // Load subdivision info
	 TMap<FString, int> SubdivisionLevels;
	 TArray<TSharedPtr<FJsonValue>> sdList = JsonObject->GetArrayField(TEXT("Subdivisions"));
	 for (int32 i = 0; i < sdList.Num(); i++)
	 {
		  TSharedPtr<FJsonObject> subdivision = sdList[i]->AsObject();
		  SubdivisionLevels.Add(subdivision->GetStringField(TEXT("Asset Name")), subdivision->GetIntegerField(TEXT("Value")));
	 }

	 // Load dforce and strand hair info
	 TMap<FString, FString> StrandAssetNameToParentNodeMap;
	 TMap<FString, FString> StrandAssetNameToLabelMap;
	 TMap<FString, TArray<TSharedPtr<FJsonValue>>> DforceAssetNameToDforceMaterialsMap;
	 TArray<TSharedPtr<FJsonValue>> strandHairInfoList = JsonObject->GetArrayField(TEXT("Strand Hair Info"));
	 for (int32 i=0; i < strandHairInfoList.Num(); i++)
	 {
		 TSharedPtr<FJsonObject> strandHairInfo = strandHairInfoList[i]->AsObject();
		 // Process strand hair info
		 FString StrandHairFile = strandHairInfo->GetStringField(TEXT("File"));
		 TArray<TSharedPtr<FJsonValue>> strandHairNodeInfo = strandHairInfo->GetArrayField(TEXT("Node Info"));
		 for (int32 j = 0; j < strandHairNodeInfo.Num(); j++)
		 {
			 TSharedPtr<FJsonObject> strandHairNode = strandHairNodeInfo[j]->AsObject();
			 FString NodeName = strandHairNode->GetStringField(TEXT("Asset Name"));
			 FString NodeLabel = strandHairNode->GetStringField(TEXT("Asset Label"));
			 FString ParentNodeName = strandHairNode->GetStringField(TEXT("Parent Name"));
			 FString ParentNodeLabel = strandHairNode->GetStringField(TEXT("Parent Label"));
			 StrandAssetNameToParentNodeMap.Add(NodeName, ParentNodeName);
			 StrandAssetNameToLabelMap.Add(FDazToUnrealUtils::SanitizeName(NodeName), NodeLabel);
		 }
	 }
	 TArray<TSharedPtr<FJsonValue>> dforceList = JsonObject->GetArrayField(TEXT("dForce"));
	 for (int32 i = 0; i < dforceList.Num(); i++)
	 {
		 TSharedPtr<FJsonObject> dforceInfo = dforceList[i]->AsObject();
		 // Process dforce info
		 FString DforceAssetName = dforceInfo->GetStringField(TEXT("Asset Name"));
		 FString DforceAssetLabel = dforceInfo->GetStringField(TEXT("Asset Label"));
		 TArray<TSharedPtr<FJsonValue>> DForceMaterialList = dforceInfo->GetArrayField(TEXT("dForce-Materials"));
		 DforceAssetNameToDforceMaterialsMap.Add(FDazToUnrealUtils::SanitizeName(DforceAssetName), DForceMaterialList);
	 }

	 // Use the maps file to find the textures to load
	 TMap<FString, FString> TextureFileSourceToTarget;
	 TArray<FString> BaseMaterialNamesList;
	 m_sourceTextureLookupTable.Reset();

	 // Find duplicate materials
	 TMap<TSharedPtr<FJsonValue>, TSharedPtr<FJsonValue>> DuplicateMaterials;
	 TArray<TSharedPtr<FJsonValue>> matList = JsonObject->GetArrayField(TEXT("Materials"));

	 if (AssetType != DazAssetType::SkeletalMesh_v2)
	 {
		if (MaterialCombineMethod == DazMaterialCombineType::CombineIdentical)
		{
			DuplicateMaterials = FDazToUnrealMaterials::FindDuplicateMaterials(matList);
		}

		// Combine All Materials
		if (MaterialCombineMethod == DazMaterialCombineType::CombineAll)
		{
			DuplicateMaterials = FDazToUnrealMaterials::CombineToOneMaterial(matList);
		}
	 }

	 // Load material values
	 for (int32 i = 0; i < matList.Num(); i++)
	 {
		 // Skip Duplicates
		 if (DuplicateMaterials.Contains(matList[i]))
		 {
			 continue;
		 }

		  TSharedPtr<FJsonObject> material = matList[i]->AsObject();

		  int32 Version = material->GetIntegerField(TEXT("Version"));

		  // Version 1 "Version, Material, Type, Color, Opacity, File"
		  if (Version == 1)
		  {
				FString MaterialName = AssetName + TEXT("_") + material->GetStringField(TEXT("Material Name"));
				MaterialName = FDazToUnrealUtils::SanitizeName(MaterialName);
				FString TexturePath = material->GetStringField(TEXT("Texture"));
				FString TextureName = FDazToUnrealUtils::SanitizeName(FPaths::GetBaseFilename(TexturePath));

				if (!DtuMaterialsTable.Contains(MaterialName))
				{
					 DtuMaterialsTable.Add(MaterialName, TArray<FDUFTextureProperty>());
				}
				FDUFTextureProperty Property;
				Property.Name = material->GetStringField(TEXT("Name"));
				Property.Type = material->GetStringField(TEXT("Data Type"));
				Property.Value = material->GetStringField(TEXT("Value"));
				if (Property.Type == TEXT("Texture"))
				{
					 Property.Type = TEXT("Color");
				}

				DtuMaterialsTable[MaterialName].Add(Property);
				if (!TextureName.IsEmpty())
				{
					 // If a texture is attached add a texture property
					 FDUFTextureProperty TextureProperty;
					 TextureProperty.Name = material->GetStringField(TEXT("Name")) + TEXT(" Texture");
					 TextureProperty.Type = TEXT("Texture");

					 if (!TextureFileSourceToTarget.Contains(TexturePath))
					 {
						  int32 TextureCount = 0;
						  FString NewTextureName = FString::Printf(TEXT("%s_%02d_%s"), *TextureName, TextureCount, *AssetName);
						  while (TextureFileSourceToTarget.FindKey(NewTextureName) != nullptr)
						  {
								TextureCount++;
								NewTextureName = FString::Printf(TEXT("%s_%02d_%s"), *TextureName, TextureCount, *AssetName);
						  }
						  TextureFileSourceToTarget.Add(TexturePath, NewTextureName);
					 }

					 TextureProperty.Value = TextureFileSourceToTarget[TexturePath];
					 DtuMaterialsTable[MaterialName].Add(TextureProperty);
					 //TextureFiles.AddUnique(TexturePath);

					 // and a switch property for things like Specular that could come from different channels
					 FDUFTextureProperty SwitchProperty;
					 SwitchProperty.Name = material->GetStringField(TEXT("Name")) + TEXT(" Texture Active");
					 SwitchProperty.Type = TEXT("Switch");
					 SwitchProperty.Value = TEXT("true");
					 DtuMaterialsTable[MaterialName].Add(SwitchProperty);
				}
		  }

		  // Version 2 "Version, ObjectName, Material, Type, Color, Opacity, File"
		  if (Version == 2)
		  {
			  FString ObjectName = material->GetStringField(TEXT("Asset Name"));
			  ObjectName = FDazToUnrealUtils::SanitizeName(ObjectName);
			  BaseMaterialNamesList.AddUnique(ObjectName + TEXT("_BaseMat"));
			  FString ShaderName = material->GetStringField(TEXT("Material Type"));
			  FString MaterialName;
			  if (CachedSettings->UseOriginalMaterialName)
			  {
				  MaterialName = material->GetStringField(TEXT("Material Name"));
			  }
			  else
			  {
				  MaterialName = AssetName + TEXT("_") + material->GetStringField(TEXT("Material Name"));
			  }

			  MaterialName = FDazToUnrealUtils::SanitizeName(MaterialName);
			  FString TexturePath = material->GetStringField(TEXT("Texture"));
			  FString TextureName = FDazToUnrealUtils::SanitizeName(FPaths::GetBaseFilename(TexturePath));

			  if (!DtuMaterialsTable.Contains(MaterialName))
			  {
				  DtuMaterialsTable.Add(MaterialName, TArray<FDUFTextureProperty>());
			  }
			  FDUFTextureProperty Property;
			  Property.Name = material->GetStringField(TEXT("Name"));
			  Property.Type = material->GetStringField(TEXT("Data Type"));
			  Property.Value = material->GetStringField(TEXT("Value"));
			  Property.ObjectName = ObjectName;
			  Property.ShaderName = ShaderName;
			  if (Property.Type == TEXT("Texture"))
			  {
				  Property.Type = TEXT("Color");
			  }

			  // Properties that end with Enabled are switches for functionality
			  if (Property.Name.EndsWith(TEXT(" Enable")))
			  {
				  Property.Type = TEXT("Switch");
				  if (Property.Value == TEXT("0"))
				  {
					  Property.Value = TEXT("false");
				  }
				  else
				  {
					  Property.Value = TEXT("true");
				  }
			  }

			  DtuMaterialsTable[MaterialName].Add(Property);
			  if (!TextureName.IsEmpty())
			  {
				  // If a texture is attached add a texture property
				  FDUFTextureProperty TextureProperty;
				  TextureProperty.Name = material->GetStringField(TEXT("Name")) + TEXT(" Texture");
				  TextureProperty.Type = TEXT("Texture");
				  TextureProperty.ObjectName = ObjectName;
				  TextureProperty.ShaderName = ShaderName;

				  if (!TextureFileSourceToTarget.Contains(TexturePath))
				  {
					  int32 TextureCount = 0;
					  FString NewTextureName = FString::Printf(TEXT("%s_%02d_%s"), *TextureName, TextureCount, *AssetName);
					  while (TextureFileSourceToTarget.FindKey(NewTextureName) != nullptr)
					  {
						  TextureCount++;
						  NewTextureName = FString::Printf(TEXT("%s_%02d_%s"), *TextureName, TextureCount, *AssetName);
					  }
					  TextureFileSourceToTarget.Add(TexturePath, NewTextureName);
				  }

				  TextureProperty.Value = TextureFileSourceToTarget[TexturePath];
				  DtuMaterialsTable[MaterialName].Add(TextureProperty);
				  //TextureFiles.AddUnique(TexturePath);

				  // and a switch property for things like Specular that could come from different channels
				  FDUFTextureProperty SwitchProperty;
				  SwitchProperty.Name = material->GetStringField(TEXT("Name")) + TEXT(" Texture Active");
				  SwitchProperty.Type = TEXT("Switch");
				  SwitchProperty.Value = TEXT("true");
				  SwitchProperty.ObjectName = ObjectName;
				  SwitchProperty.ShaderName = ShaderName;
				  DtuMaterialsTable[MaterialName].Add(SwitchProperty);
			  }
		  }

		  // Version 3 "Version, ObjectName, Material, [Type, Color, Opacity, File]"
		  // DB 2022-July-8: Version 4 is backward compatible with Unreal plugin but further
		  // review is needed to review remapped records and integrate new features.
		  if (Version == 3 || Version == 4)
		  {
				FString ObjectName = material->GetStringField(TEXT("Asset Label"));
				ObjectName = FDazToUnrealUtils::SanitizeName(ObjectName);
				BaseMaterialNamesList.AddUnique(ObjectName + TEXT("_BaseMat"));
				FString ShaderName = material->GetStringField(TEXT("Material Type"));
				FString MaterialName;
				if (CachedSettings->UseOriginalMaterialName)
				{
					 MaterialName = material->GetStringField(TEXT("Material Name"));
				}
				else
				{
					 MaterialName = ObjectName + TEXT("_") + material->GetStringField(TEXT("Material Name"));
				}

				MaterialName = FDazToUnrealUtils::SanitizeName(MaterialName);

				// Check Strand Hair and Dforce
				FString AssetNameLookup = FDazToUnrealUtils::SanitizeName(material->GetStringField(TEXT("Asset Name")));
				bool bIsStrandAsset = (StrandAssetNameToLabelMap.Contains(AssetNameLookup));
				bool bHasDForceInfo = (DforceAssetNameToDforceMaterialsMap.Contains(AssetNameLookup));

				// DB 2021-Dec-16: TexturePath and TextureName moved to "per property" execution below
//				FString TexturePath = material->GetStringField(TEXT("Texture"));
//				FString TextureName = FDazToUnrealUtils::SanitizeName(FPaths::GetBaseFilename(TexturePath));

				if (!DtuMaterialsTable.Contains(MaterialName))
				{
					 DtuMaterialsTable.Add(MaterialName, TArray<FDUFTextureProperty>());
				}

				// DB 2021-Dec-16: Nested Properties Array Change
				TArray<TSharedPtr<FJsonValue>> propList = material->GetArrayField(TEXT("Properties"));
				for (int32 propIndex = 0; propIndex < propList.Num(); propIndex++)
				{
					TSharedPtr<FJsonObject> propListElement = propList[propIndex]->AsObject();

					FDUFTextureProperty Property;
					Property.Name = propListElement->GetStringField(TEXT("Name"));
					Property.Type = propListElement->GetStringField(TEXT("Data Type"));
					Property.Value = propListElement->GetStringField(TEXT("Value"));
					// Moved from "per material" execution above
					FString TexturePath = propListElement->GetStringField(TEXT("Texture"));
					FString TextureName = FDazToUnrealUtils::SanitizeName(FPaths::GetBaseFilename(TexturePath));

					Property.ObjectName = ObjectName;
					Property.ShaderName = ShaderName;
					FString sMaterialAssetName = material->GetStringField(TEXT("Asset Name"));
					Property.MaterialAssetName = FDazToUnrealUtils::SanitizeName(sMaterialAssetName);
					Property.bHasDForceInfo = bHasDForceInfo;
					Property.bIsStrandAsset = bIsStrandAsset;

					if (Property.Type == TEXT("Texture"))
					{
						Property.Type = TEXT("Color");
					}
                    if (Property.Name == TEXT("Cutout Opacity") )
					{
						TextureLookupInfo lookupInfo;
						lookupInfo.sSourceFullPath = TexturePath;
						lookupInfo.bIsCutOut = true;
						m_sourceTextureLookupTable.Add(TextureName, lookupInfo);
                    }

					// Properties that end with Enabled are switches for functionality
					if (Property.Name.EndsWith(TEXT(" Enable")))
					{
						Property.Type = TEXT("Switch");
						if (Property.Value == TEXT("0"))
						{
							Property.Value = TEXT("false");
						}
						else
						{
							Property.Value = TEXT("true");
						}
					}


					DtuMaterialsTable[MaterialName].Add(Property);
					if (!TextureName.IsEmpty())
					{
						// If a texture is attached add a texture property
						FDUFTextureProperty TextureProperty;
						TextureProperty.Name = propListElement->GetStringField(TEXT("Name")) + TEXT(" Texture");
						TextureProperty.Type = TEXT("Texture");
						TextureProperty.ObjectName = ObjectName;
						TextureProperty.ShaderName = ShaderName;

						if (!TextureFileSourceToTarget.Contains(TexturePath))
						{
							int32 TextureCount = 0;
							FString NewTextureName = FString::Printf(TEXT("%s_%02d_%s"), *TextureName, TextureCount, *AssetName);
							if (BatchConversionMode != 0)
							{
								NewTextureName = FString::Printf(TEXT("%s_%02d"), *TextureName, TextureCount);
							}
							while (TextureFileSourceToTarget.FindKey(NewTextureName) != nullptr)
							{
								TextureCount++;
								NewTextureName = FString::Printf(TEXT("%s_%02d_%s"), *TextureName, TextureCount, *AssetName);
								if (BatchConversionMode != 0)
								{
									NewTextureName = FString::Printf(TEXT("%s_%02d"), *TextureName, TextureCount);
								}
							}
							TextureFileSourceToTarget.Add(TexturePath, NewTextureName);
						}

						TextureProperty.Value = TextureFileSourceToTarget[TexturePath];
						DtuMaterialsTable[MaterialName].Add(TextureProperty);
						//TextureFiles.AddUnique(TexturePath);

						// and a switch property for things like Specular that could come from different channels
						FDUFTextureProperty SwitchProperty;
						SwitchProperty.Name = propListElement->GetStringField(TEXT("Name")) + TEXT(" Texture Active");
						SwitchProperty.Type = TEXT("Switch");
						SwitchProperty.Value = TEXT("true");
						SwitchProperty.ObjectName = ObjectName;
						SwitchProperty.ShaderName = ShaderName;
						DtuMaterialsTable[MaterialName].Add(SwitchProperty);
					}

				}
		  }
	 }

	 // DB 2025-06-11 added to arguments
	 FString RootBoneName = TEXT("");
	 TArray<FString> MaterialSlotNames;

	 DtuMaterialsTable.GenerateKeyArray(MaterialSlotNames);

	 // If this is a character, determine the type.
	 DazCharacterType CharacterType = DazCharacterType::Unknown;
	 FString CharacterTypeName = RootBoneName.Replace(TEXT("\0"), TEXT(""));
	 if (RootBoneName == TEXT("Genesis3Male"))
	 {
		  CharacterType = DazCharacterType::Genesis3Male;
	 }
	 else if (RootBoneName == TEXT("Genesis3Female"))
	 {
		  CharacterType = DazCharacterType::Genesis3Female;
	 }
	 else if (RootBoneName == TEXT("Genesis8Male"))
	 {
		  CharacterType = DazCharacterType::Genesis8Male;
	 }
	 else if (RootBoneName == TEXT("Genesis8Female"))
	 {
		  CharacterType = DazCharacterType::Genesis8Female;
	 }
	 else if (RootBoneName == TEXT("Genesis"))
	 {
		  CharacterType = DazCharacterType::Genesis1;
	 }
	 ImportData.CharacterType = CharacterType;
	 ImportData.CharacterTypeName = CharacterTypeName;

	 // Import Textures
	 Progress.EnterProgressFrame(1, LOCTEXT("ImportingTextures", "Importing Textures"));
	 if (AssetType == DazAssetType::SkeletalMesh || AssetType == DazAssetType::StaticMesh || AssetType == DazAssetType::R2x || AssetType == DazAssetType::SkeletalMesh_v2 || AssetType == DazAssetType::UNKNOWN)
	 {
		  TArray<FString> TexturesFilesToImport;
		  m_targetTextureLookupTable.Reset();
		  for (auto TexturePair : TextureFileSourceToTarget)
		  {
				FString SourceFileName = TexturePair.Key;
				FString TargetFileName = ImportCharacterTexturesFolder / TexturePair.Value + FPaths::GetExtension(SourceFileName, true);

				// Map m_sourceTextureLookupTable to m_targetTextureLookupTable
				// TODO: convert in place: m_sourceTextureLookupTable to m_targetTextureLookupTable
				FString sSearchString = FDazToUnrealUtils::SanitizeName(FPaths::GetBaseFilename(SourceFileName));
				if (m_sourceTextureLookupTable.Contains(sSearchString))
				{
					TextureLookupInfo lookupData = m_sourceTextureLookupTable[sSearchString];
					m_targetTextureLookupTable.Add(TargetFileName, lookupData);
				}
				PlatformFile.CopyFile(*TargetFileName, *SourceFileName);
				TexturesFilesToImport.Add(TargetFileName);
		  }
		  ImportTextureAssets(TexturesFilesToImport, CharacterTexturesFolder);
	 }

	 // Create Intermediate Materials
	 Progress.EnterProgressFrame(1, LOCTEXT("CreatingMaterials", "Creating Materials"));
	 if (AssetType == DazAssetType::SkeletalMesh || AssetType == DazAssetType::StaticMesh || AssetType == DazAssetType::R2x || AssetType == DazAssetType::SkeletalMesh_v2 || AssetType == DazAssetType::UNKNOWN)
	 {
		 // Create a default Master Subsurface Profile if needed
		 USubsurfaceProfile* MasterSubsurfaceProfile = FDazToUnrealMaterials::CreateSubsurfaceBaseProfileForCharacter(CharacterMaterialFolder, DtuMaterialsTable);

		for (FString BaseMaterialName : BaseMaterialNamesList)
		{
			// Find all materials that are related to this base material
			TArray<FString> RelatedMaterialNamesList;
			FString ChildMaterialFolder = CharacterMaterialFolder;
			for (FString ChildMaterialName : MaterialSlotNames)
			{
				if (DtuMaterialsTable.Contains(ChildMaterialName))
				{
					for (FDUFTextureProperty ChildProperty : DtuMaterialsTable[ChildMaterialName])
					{
						if ((ChildProperty.ObjectName + TEXT("_BaseMat")) == BaseMaterialName)
						{
							ChildMaterialFolder = CharacterMaterialFolder / ChildProperty.ObjectName;
							if (BatchConversionMode != 0)
							{
								ChildMaterialFolder = CharacterMaterialFolder;
							}
							RelatedMaterialNamesList.AddUnique(ChildMaterialName);
							break;
						}
					}
				}
			}

			// Create Related Materials
			if (RelatedMaterialNamesList.Num() == 1)
			{
				FString UnrealMaterialName = RelatedMaterialNamesList[0];
				USubsurfaceProfile* SubsurfaceProfile = FDazToUnrealMaterials::CreateSubsurfaceProfileForMaterial(UnrealMaterialName, CharacterMaterialFolder / UnrealMaterialName, DtuMaterialsTable[UnrealMaterialName]);
				FDazToUnrealMaterials::CreateMaterial(ChildMaterialFolder, CharacterTexturesFolder, UnrealMaterialName, DtuMaterialsTable, CharacterType, nullptr, SubsurfaceProfile);
			}
			else if (RelatedMaterialNamesList.Num() > 1)
			{
				CreateRelatedMaterials(
					RelatedMaterialNamesList,
					MasterSubsurfaceProfile,
					DtuMaterialsTable,
					BaseMaterialName,
					ChildMaterialFolder,
					CharacterMaterialFolder,
					CharacterTexturesFolder,
					CharacterType
				);

			}
		}
	}

	 // Import FBX
	 Progress.EnterProgressFrame(1, LOCTEXT("ImportingFBX", "Importing the FBX"));
	 bool bSetPostProcessAnimation = !FDazToUnrealMorphs::IsAutoJCMImport(JsonObject);
	 ImportData.bSetPostProcessAnimation = bSetPostProcessAnimation;
	 UObject* NewObject = ImportFBXAsset(ImportData);

	 // If this is a Pose transfer, an AnimSequence was created.  Make a PoseAsset from it.
	 if (AssetType == DazAssetType::Pose)
	 {
		 if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(NewObject))
		 {
			 UPoseAsset* NewPoseAsset = FDazToUnrealPoses::CreatePoseAsset(AnimSequence, PoseNameList);

			 FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			 TArray<UObject*> AssetsToSelect;
			 AssetsToSelect.Add((UObject*)NewPoseAsset);
			 ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSelect);
		 }
	 }

	 Progress.EnterProgressFrame(1, LOCTEXT("CreatingAutoJCMControlRig", "Creating AutoJCM Control Rig"));
	 // Create and attach the Joint Control Anim
	 if (AssetType == DazAssetType::SkeletalMesh && CachedSettings->CreateAutoJCMControlRig && FDazToUnrealMorphs::IsAutoJCMImport(JsonObject))
	 {
		 if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(NewObject))
		 {
#if UE_VERSION_NEWER_THAN(4, 26, 99)
			 USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
#else
			 USkeleton* Skeleton = SkeletalMesh->Skeleton;
#endif

			 if (UAnimBlueprint* JointControlAnimBlueprint = FDazToUnrealMorphs::CreateJointControlAnimation(JsonObject, CharacterFolder, AssetName, Skeleton, SkeletalMesh))
			 {
				 //if (FDazToUnrealMorphs::IsAutoJCMImport(JsonObject))
				 {
					 FString SkeletalMeshPackagePath = NewObject->GetOutermost()->GetPathName() + TEXT(".") + NewObject->GetName();
					 FString PostProcessAnimPackagePath = JointControlAnimBlueprint->GetOutermost()->GetPathName() + TEXT(".") + JointControlAnimBlueprint->GetName();
					 FString CreateJCMControlRigCommand = FString::Format(TEXT("py CreateAutoJCMControlRig.py --skeletalMesh={0} --animBlueprint={1} --dtuFile=\"{2}\""), { SkeletalMeshPackagePath, PostProcessAnimPackagePath, FileName });
					 UE_LOG(LogDazToUnreal, Log, TEXT("Creating AutoJCM Control Rig with command: %s"), *CreateJCMControlRigCommand);
					 GEngine->Exec(NULL, *CreateJCMControlRigCommand);
				 }
#if UE_VERSION_NEWER_THAN(4, 26, 99)
				 SkeletalMesh->SetPostProcessAnimBlueprint(JointControlAnimBlueprint->GetAnimBlueprintGeneratedClass());
#else
				 UAnimInstance* JointControlAnim = Cast<UAnimInstance>(JointControlAnimBlueprint->GetAnimBlueprintGeneratedClass()->ClassDefaultObject);
				 SkeletalMesh->PostProcessAnimBlueprint = JointControlAnim->GetClass();
#endif
			 }
		 }
	 }

	 Progress.EnterProgressFrame(1, LOCTEXT("CreatingFullBodyIKControlRig", "Creating Full Body IK Control Rig"));
#if ENGINE_MAJOR_VERSION > 4
	 // Create a control rig for the character
	 if (AssetType == DazAssetType::SkeletalMesh && CachedSettings->CreateFullBodyIKControlRig && !ImportData.bConvertToEpicSkeleton && NewObject)
	 {
		 FString SkeletalMeshPackagePath = NewObject->GetOutermost()->GetPathName() + TEXT(".") + NewObject->GetName();
		 FString CreateControlRigCommand = FString::Format(TEXT("py CreateControlRig.py --skeletalMesh={0} --dtuFile=\"{1}\""), { SkeletalMeshPackagePath, FileName });
		 UE_LOG(LogDazToUnreal, Log, TEXT("Creating Control Rig with command: %s"), *CreateControlRigCommand);
		 GEngine->Exec(NULL, *CreateControlRigCommand);
	 }
#endif

	 // Rename Material Slots
	 if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(NewObject))
	 {
		 //for (FSkeletalMaterial& SkeletalMaterial : SkeletalMesh->GetMaterials())
		 {
			 //SkeletalMaterial.MaterialSlotName = *ImportData.MaterialSlotNameToMaterialName.Find(SkeletalMaterial.MaterialSlotName);
		 }

		 //TArray<FSkeletalMaterial>& MaterialsToSort = SkeletalMesh->GetMaterials();
		 //MaterialsToSort.Sort([](const FSkeletalMaterial& A, const FSkeletalMaterial& B) { return A.MaterialSlotName.ToString() < B.MaterialSlotName.ToString(); });
		 //SkeletalMesh->SetMaterials((SkeletalMesh->GetMaterials().Sort([](const FSkeletalMaterial& A, const FSkeletalMaterial& B) { return A.MaterialSlotName.ToString() < B.MaterialSlotName.ToString(); }));
	 }

	 if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(NewObject))
	 {
		 if (ImportData.bConvertToEpicSkeleton)
		 {
			 UDazToUnrealBlueprintUtils::ConvertToEpicSkeleton(SkeletalMesh, nullptr);
		 }
	 }

	 if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(NewObject))
	 {
		 FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		 TArray<UObject*> AssetsToSelect;
		 AssetsToSelect.Add(SkeletalMesh);
		 ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSelect);
	 }

	 // DB 2023-Aug-15: Auto-generate LOD meshes
#define LOD_METHOD_UNREAL_BUILTIN 2
	 TSharedPtr<FJsonObject> LodSettingsObject = JsonObject->GetObjectField(TEXT("LOD Settings"));
	 bool bGenerateLODs = LodSettingsObject->GetBoolField(TEXT("Generate LODs"));
	 int nLodMethod = LodSettingsObject->GetIntegerField(TEXT("LOD Method"));
	 int targetNumLODs = LodSettingsObject->GetIntegerField(TEXT("Number of LODs"));

	 if (bGenerateLODs == true && nLodMethod == LOD_METHOD_UNREAL_BUILTIN)
	 {
		 Progress.EnterProgressFrame(0.1, LOCTEXT("GeneratingLODs", "Generating LOD Meshes..."));
		 if (USkeletalMesh* SkeletalMeshAsset = Cast<USkeletalMesh>(NewObject))
		 {
			 int numLOD = SkeletalMeshAsset->GetLODNum();
			 if (numLOD > 1)
			 {
				 // remove all LODs above base (lod 0)
				 SkeletalMeshAsset->GetImportedModel()->LODModels.RemoveAt(1, numLOD-1);
				 for (int i = numLOD-1; i > 0; i--)
				 {
					 SkeletalMeshAsset->RemoveLODInfo(i);
				 }
				 SkeletalMeshAsset->PostEditChange();
				 numLOD = SkeletalMeshAsset->GetLODNum();
			 }
			 UE_LOG(LogTemp, Warning, TEXT(">> LOD Num: %d"), numLOD);

			 IMeshReductionModule* meshReducer = (IMeshReductionModule*)FModuleManager::Get().LoadModule(TEXT("MeshReductionInterface"));
			 IMeshReduction* skMeshReducer = meshReducer->GetSkeletalMeshReductionInterface();
			 if (skMeshReducer != nullptr)
			 {
				 bool isSupported = skMeshReducer->IsSupported();
				 FString versionString = skMeshReducer->GetVersionString();
				 if (isSupported)
				 {
					 ITargetPlatform* pcPlatform = nullptr;
#if PLATFORM_WINDOWS
					 pcPlatform = GetTargetPlatformManager()->FindTargetPlatform(TEXT("Windows"));
#elif PLATFORM_MAC
					 pcPlatform = GetTargetPlatformManager()->FindTargetPlatform(TEXT("Mac"));
#endif
					 if (pcPlatform == nullptr)
					 {
						 // Handle the case where the target platform was not found
						 UE_LOG(LogTemp, Warning, TEXT("Target platform not found!"));
					 }

					 TArray<TSharedPtr<FJsonValue>> LodInfoArray = LodSettingsObject->GetArrayField(TEXT("LOD Info Array"));
					 while (numLOD < targetNumLODs)
					 {

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 26
						 if (LodInfoArray.Num() > numLOD)
						 {
							 TSharedPtr<FJsonObject> lodInfo = LodInfoArray[numLOD]->AsObject();
							 if (lodInfo)
							 {
								 int MaxNumOfVerts = 0;

								 MaxNumOfVerts = SkeletalMeshAsset->GetImportedModel()->LODModels[0].NumVertices;

								 float NewScreenSize = lodInfo->GetNumberField(TEXT("Threshold Screen Height"));
								 float NewQualityPercent = lodInfo->GetNumberField(TEXT("Quality Percent"));
								 int NewQualityVertex = (int) lodInfo->GetIntegerField(TEXT("Quality Vertex"));
								 FSkeletalMeshLODInfo NewLODInfo;
								 NewLODInfo.ReductionSettings.TerminationCriterion = SkeletalMeshTerminationCriterion::SMTC_NumOfVerts;
								 NewLODInfo.ScreenSize = NewScreenSize;
								 if (NewQualityPercent != -1)
								 {
									 NewLODInfo.ReductionSettings.NumOfVertPercentage = NewQualityPercent;
								 }
								 else if (NewQualityVertex > 0)
								 {
									 NewLODInfo.ReductionSettings.NumOfVertPercentage = (double) NewQualityVertex / MaxNumOfVerts;
								 }
								 if (NewQualityVertex > 0)
								 {
									 NewLODInfo.ReductionSettings.MaxNumOfVerts = NewQualityVertex;
								 }
								 SkeletalMeshAsset->AddLODInfo(NewLODInfo);
							 }
						 }
#endif

						 FText LodProgressText = FText::Format(LOCTEXT("GeneratingLOD_XofX", "Generating LOD Mesh: {numLOD} of {targetNumLODs}"),
							 FText::AsNumber(numLOD),
							 FText::AsNumber(targetNumLODs - 1));
						 Progress.EnterProgressFrame(1.0 / targetNumLODs, LodProgressText);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
						 skMeshReducer->ReduceSkeletalMesh(SkeletalMeshAsset, numLOD, pcPlatform);
#else
						 skMeshReducer->ReduceSkeletalMesh(SkeletalMeshAsset, numLOD);
#endif
						 numLOD++;
					 }
				 }
			 }
		 }
	 }

	 // DB 2025-08-12: Groom Asset Import
	 TArray<TSharedPtr<FJsonValue>> StrandHairInfoArray = JsonObject->GetArrayField(TEXT("Strand Hair Info"));
	 for (int i=0; i < StrandHairInfoArray.Num(); i++)
	 {
		 TSharedPtr<FJsonObject> StrandHairInfo = StrandHairInfoArray[i]->AsObject();
		 if (StrandHairInfo)
		 {
			 FString sGroomFilePath = StrandHairInfo->GetStringField(TEXT("File"));
			 ImportGroom(sGroomFilePath, ImportData.ImportLocation, JsonObject);
		 }
	 }

	FString SkeletalMeshPackagePath = NewObject->GetOutermost()->GetPathName() + TEXT(".") + NewObject->GetName();
	if (BatchConversionMode != 0) {
		FixForFab(DAZImportFolder, SkeletalMeshPackagePath);
	}

	return NewObject;
}

// Modified from the FColor::FromHex function
FLinearColor FDazToUnrealModule::FromHex(const FString& HexString)
{
	 int32 StartIndex = (!HexString.IsEmpty() && HexString[0] == TCHAR('#')) ? 1 : 0;

	 if (HexString.Len() == 3 + StartIndex)
	 {
		  const int32 R = FParse::HexDigit(HexString[StartIndex++]);
		  const int32 G = FParse::HexDigit(HexString[StartIndex++]);
		  const int32 B = FParse::HexDigit(HexString[StartIndex]);

		  return FLinearColor(((R << 4) + R) / 255.0, ((G << 4) + G) / 355.0, ((B << 4) + B) / 255.0, 1.0);
	 }

	 if (HexString.Len() == 6 + StartIndex)
	 {
		  FLinearColor Result;

		  Result.R = ((FParse::HexDigit(HexString[StartIndex + 0]) << 4) + FParse::HexDigit(HexString[StartIndex + 1])) / 255.0;
		  Result.G = ((FParse::HexDigit(HexString[StartIndex + 2]) << 4) + FParse::HexDigit(HexString[StartIndex + 3])) / 255.0;
		  Result.B = ((FParse::HexDigit(HexString[StartIndex + 4]) << 4) + FParse::HexDigit(HexString[StartIndex + 5])) / 255.0;
		  Result.A = 1.0f;

		  return Result;
	 }

	 if (HexString.Len() == 8 + StartIndex)
	 {
		  FLinearColor Result;

		  Result.R = ((FParse::HexDigit(HexString[StartIndex + 0]) << 4) + FParse::HexDigit(HexString[StartIndex + 1])) / 255.0;
		  Result.G = ((FParse::HexDigit(HexString[StartIndex + 2]) << 4) + FParse::HexDigit(HexString[StartIndex + 3])) / 255.0;
		  Result.B = ((FParse::HexDigit(HexString[StartIndex + 4]) << 4) + FParse::HexDigit(HexString[StartIndex + 5])) / 255.0;
		  Result.A = ((FParse::HexDigit(HexString[StartIndex + 6]) << 4) + FParse::HexDigit(HexString[StartIndex + 7])) / 255.0;

		  return Result;
	 }

	 return FLinearColor(ForceInitToZero);
}


bool FDazToUnrealModule::ImportTextureAssets(TArray<FString>& SourcePaths, FString& ImportLocation)
{
	 FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	 //TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(SourcePaths, ImportLocation);

	 UTextureFactory* TextureFactory = NewObject<UTextureFactory>(UTextureFactory::StaticClass());
	 UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>(UAutomatedAssetImportData::StaticClass());
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
#else
	 ImportData->FactoryName = TEXT("TextureFactory");
	 ImportData->Factory = TextureFactory;
#endif
	 ImportData->Filenames = SourcePaths;
	 ImportData->DestinationPath = ImportLocation;
	 if (BatchConversionMode != 0)
		 ImportData->bReplaceExisting = false;
	 else
		 ImportData->bReplaceExisting = true;
	 if (ImportData->IsValid() == false)
	 {
		 return false;
	 }
	 TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);

	 // Texture Corrections: sRGB
#if ENGINE_MAJOR_VERSION > 4
	// NOTE: UE5 ImportedAssets count is not 1:1 compatible with SourcePaths count,
	// so we can't assume index based on iteration through loop. Instead, we need
	// to search through the ImportedAssets array to find the matching asset based
	// on source path as retrieved from the
	// Texture->AssetImportData->GetFirstFilename() function.
	FDazToUnrealModule::TextureListToDisableSRGB.Empty();
	for (auto ImportedAsset : ImportedAssets)
	{
		if (ImportedAsset->IsA(UTexture::StaticClass()))
		{
			UTexture* Texture = Cast<UTexture>(ImportedAsset);
			FString TextureName = Texture->GetName();
			FString TexturePath = Texture->AssetImportData->GetFirstFilename();
			if (m_targetTextureLookupTable.Contains(TexturePath))
			{
				TextureLookupInfo lookupData = m_targetTextureLookupTable[TexturePath];
				if (lookupData.bIsCutOut == true)
				{
					// DB 2023-May-23: Disable SRGB in a delayed step after importing textures is done to avoid engine crash (please see Tick() )
					//UE_LOG(LogTemp, Display, TEXT("DazToUnreal: ImportTextureAssets() Texture %s is a cutout texture. Setting sRGB to false."), *TextureName);
					FDazToUnrealModule::TextureListToDisableSRGB.Add(Texture);
				}
			}
		}
	}

#else
	if (ImportedAssets.Num() != SourcePaths.Num())
	 {
		 UE_LOG(LogDazToUnreal, Error, TEXT("DazToUnreal: ImportTextureAssets() ERROR: ImportedAssets count is not equal to SourcePaths count. Texture Lookup Correction will likely fail..."));
	 }
	 else
	 {
	 	int textureIndex = 0;
		// Pseudocode: For each sourcepath, check if it is in the lookup table, and if it is, check if it is a cutout texture. If it is, set the texture to sRGB = false.
	 	for (FString SourcePath : SourcePaths)
		 {
			 if (m_targetTextureLookupTable.Contains(SourcePath))
			 {
				 TextureLookupInfo lookupData = m_targetTextureLookupTable[SourcePath];
				 if (lookupData.bIsCutOut == true)
				 {
					 if (textureIndex >= ImportedAssets.Num())
					 {
						 UE_LOG(LogDazToUnreal, Warning, TEXT("DazToUnreal: ERROR: sRGB-corection texture-index lookup procedure returned invalid texture index. Skipping..."));
					 }
					 else
					 {
						 if (UTexture* texture = Cast<UTexture>(ImportedAssets[textureIndex]))
						 {
							 texture->SRGB = false;
						 }
					 }
				 }
			 }
			 textureIndex++;
		 }
	 }
#endif

	 if (ImportedAssets.Num() > 0)
	 {
		  return true;
	 }
	 return false;
}

UObject* FDazToUnrealModule::ImportFBXAsset(const DazToUnrealImportData& DazImportData)
{
	 static FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	 UDazToUnrealSettings* CachedSettings = GetMutableDefault<UDazToUnrealSettings>();

	 FString NewFBXPath = DazImportData.SourcePath;
	 TArray<FString> FileNames;
	 FileNames.Add(NewFBXPath);

	 UFbxFactory* FbxFactory = NewObject<UFbxFactory>(UFbxFactory::StaticClass());
	 FbxFactory->AddToRoot();

	 FSoftObjectPath SkeletonPath = FDazToUnrealUtils::GetSkeletonForImport(DazImportData);
	 USkeleton* Skeleton = Cast<USkeleton>(SkeletonPath.TryLoad());

	 UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
	 FbxFactory->SetDetectImportTypeOnImport(false);
	 FbxFactory->ImportUI->TextureImportData->MaterialSearchLocation = EMaterialSearchLocation::UnderRoot;
	 FbxFactory->ImportUI->bImportMaterials = false;
	 FbxFactory->ImportUI->bImportTextures = false;

	 if (DazImportData.AssetType == DazAssetType::SkeletalMesh)
	 {
		  FbxFactory->ImportUI->bImportAsSkeletal = true;
		  FbxFactory->ImportUI->Skeleton = Skeleton;
		  FbxFactory->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
		  FbxFactory->ImportUI->bImportAnimations = false;
		  FbxFactory->ImportUI->SkeletalMeshImportData->bUseT0AsRefPose = CachedSettings->FrameZeroIsReferencePose;
		  FbxFactory->ImportUI->SkeletalMeshImportData->bConvertScene = true;
		  FbxFactory->ImportUI->SkeletalMeshImportData->bForceFrontXAxis = DazImportData.bFaceCharacterRight;
		  // DB 2023-May-26: ReEnabling to support bone attached props, until alternative is 100% working
	 	  FbxFactory->ImportUI->SkeletalMeshImportData->bImportMeshesInBoneHierarchy = true;
		  FbxFactory->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
	 }
	 if (DazImportData.AssetType == DazAssetType::StaticMesh || DazImportData.AssetType == DazAssetType::UNKNOWN)
	 {
		  FbxFactory->ImportUI->bImportAsSkeletal = false;
		  FbxFactory->ImportUI->bImportMaterials = true;
		  FbxFactory->ImportUI->StaticMeshImportData->bForceFrontXAxis = false;
		  FbxFactory->ImportUI->MeshTypeToImport = FBXIT_StaticMesh;
	 }
	 if (DazImportData.AssetType == DazAssetType::Animation || DazImportData.AssetType == DazAssetType::Pose || DazImportData.AssetType == DazAssetType::MLDeformer)
	 {
		  FbxFactory->ImportUI->bImportAsSkeletal = true;
		  FbxFactory->ImportUI->Skeleton = Skeleton;
		  FbxFactory->ImportUI->bImportMesh = false;
		  FbxFactory->ImportUI->bImportMaterials = false;
		  FbxFactory->ImportUI->bImportTextures = false;
		  FbxFactory->ImportUI->bImportAnimations = true;
		  FbxFactory->ImportUI->AnimSequenceImportData->bConvertScene = true;
		  FbxFactory->ImportUI->AnimSequenceImportData->bForceFrontXAxis = DazImportData.bFaceCharacterRight;
#if UE_VERSION_NEWER_THAN(5,2,99)
		  FbxFactory->ImportUI->AnimSequenceImportData->bAddCurveMetadataToSkeleton = true;
#endif
		  FbxFactory->ImportUI->MeshTypeToImport = FBXIT_Animation;
	 }
	 if (DazImportData.AssetType == DazAssetType::R2x || DazImportData.AssetType== DazAssetType::SkeletalMesh_v2)
	 {
		 FbxFactory->ImportUI->bImportAsSkeletal = true;
		 FbxFactory->ImportUI->Skeleton = Skeleton;
		 FbxFactory->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
		 FbxFactory->ImportUI->bImportAnimations = false;
		 //FbxFactory->ImportUI->SkeletalMeshImportData->bUseT0AsRefPose = CachedSettings->FrameZeroIsReferencePose;
		 //FbxFactory->ImportUI->SkeletalMeshImportData->bConvertScene = false;
		 //FbxFactory->ImportUI->SkeletalMeshImportData->bForceFrontXAxis = DazImportData.bFaceCharacterRight;
		 FbxFactory->ImportUI->SkeletalMeshImportData->bImportMeshesInBoneHierarchy = true;
		 FbxFactory->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
	 }
	 //UFbxFactory::EnableShowOption();
	 UAutomatedAssetImportData* FbxImportData = NewObject<UAutomatedAssetImportData>(UAutomatedAssetImportData::StaticClass());
	 FbxImportData->FactoryName = TEXT("FbxFactory");
	 FbxImportData->Factory = FbxFactory;
	 FbxImportData->Filenames = FileNames;
	 FbxImportData->DestinationPath = DazImportData.ImportLocation;
	 if (BatchConversionMode != 0)
		 FbxImportData->bReplaceExisting = false;
	 else
		 FbxImportData->bReplaceExisting = true;
	 if (DazImportData.AssetType == DazAssetType::Animation || DazImportData.AssetType == DazAssetType::Pose)
	 {
		 FbxImportData->DestinationPath = CachedSettings->AnimationImportDirectory.Path;
	 }

	 TArray<UObject*> ImportedAssets;
	 if (CachedSettings->ShowFBXImportDialog)
	 {
		  UAssetImportTask* AssetImportTask = NewObject<UAssetImportTask>();
		  AssetImportTask->Filename = FbxImportData->Filenames[0];
		  AssetImportTask->DestinationPath = FbxImportData->DestinationPath;
		  AssetImportTask->Options = FbxFactory->ImportUI;
		  AssetImportTask->Factory = FbxFactory;
		  AssetImportTask->bAutomated = false;
		  TArray< UAssetImportTask* > ImportTasks;
		  ImportTasks.Add(AssetImportTask);
		  AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
		  for (FString ImportedPath : AssetImportTask->ImportedObjectPaths)
		  {
				FSoftObjectPath SoftObjectPath(ImportedPath);
				ImportedAssets.Add(SoftObjectPath.TryLoad());
		  }
	 }
	 else
	 {
		ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(FbxImportData);
		// ERROR CHECK
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogDazToUnreal, Warning, TEXT("DazToUnreal: ERROR: automated FBX import failed, attempting interactive import..."));
			UAssetImportTask* AssetImportTask = NewObject<UAssetImportTask>();
			AssetImportTask->Filename = FbxImportData->Filenames[0];
			AssetImportTask->DestinationPath = FbxImportData->DestinationPath;
			AssetImportTask->Options = FbxFactory->ImportUI;
			AssetImportTask->Factory = FbxFactory;
			AssetImportTask->bAutomated = false;
			TArray< UAssetImportTask* > ImportTasks;
			ImportTasks.Add(AssetImportTask);
			AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
			for (FString ImportedPath : AssetImportTask->ImportedObjectPaths)
			{
					FSoftObjectPath SoftObjectPath(ImportedPath);
					ImportedAssets.Add(SoftObjectPath.TryLoad());
			}
		}
	 }

	 FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	 ContentBrowserModule.Get().SyncBrowserToAssets(ImportedAssets);

	 for (UObject* ImportedAsset : ImportedAssets)
	 {
		  if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(ImportedAsset))
		  {
				if (DazImportData.bSetPostProcessAnimation && CachedSettings->SkeletonPostProcessAnimation.Contains(SkeletonPath))
				{
#if UE_VERSION_NEWER_THAN(4, 26, 99)
					SkeletalMesh->SetPostProcessAnimBlueprint(CachedSettings->SkeletonPostProcessAnimation[SkeletonPath].TryLoadClass<UAnimInstance>());
#else
					SkeletalMesh->PostProcessAnimBlueprint = CachedSettings->SkeletonPostProcessAnimation[SkeletonPath].TryLoadClass<UAnimInstance>();
#endif
				}

				//Get the new skeleton
				if (!Skeleton)
				{
#if UE_VERSION_NEWER_THAN(4, 26, 99)
					Skeleton = SkeletalMesh->GetSkeleton();
#else
					 Skeleton = SkeletalMesh->Skeleton;
#endif

#if VODSVERSION
					 // Update skeleton retargeting options
					 int32 HipBoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(TEXT("hip")));
					 if (HipBoneIndex != -1)
					 {
						 Skeleton->SetBoneTranslationRetargetingMode(HipBoneIndex, EBoneTranslationRetargetingMode::AnimationScaled, false);
					 }

					 int32 PelvisBoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(TEXT("pelvis")));
					 if (PelvisBoneIndex != -1)
					 {
						 Skeleton->SetBoneTranslationRetargetingMode(PelvisBoneIndex, EBoneTranslationRetargetingMode::Skeleton, true);
					 }

					 int32 Spine1BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(TEXT("spine1")));
					 if (Spine1BoneIndex != -1)
					 {
						 Skeleton->SetBoneTranslationRetargetingMode(Spine1BoneIndex, EBoneTranslationRetargetingMode::Skeleton, true);
					 }

					 int32 AbdomenLowerBoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(TEXT("abdomenLower")));
					 if (AbdomenLowerBoneIndex != -1)
					 {
						 Skeleton->SetBoneTranslationRetargetingMode(AbdomenLowerBoneIndex, EBoneTranslationRetargetingMode::Skeleton, true);
					 }

					 int32 HeadBoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(TEXT("head")));
					 if (HeadBoneIndex != -1)
					 {
						 Skeleton->SetBoneTranslationRetargetingMode(HeadBoneIndex, EBoneTranslationRetargetingMode::AnimationRelative, true);
						 Skeleton->SetBoneTranslationRetargetingMode(HeadBoneIndex, EBoneTranslationRetargetingMode::Skeleton, false);
					 }
#else
					 int32 PelvisBoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(FName(TEXT("pelvis")));
					 if (PelvisBoneIndex != -1)
					 {
						 Skeleton->SetBoneTranslationRetargetingMode(PelvisBoneIndex, EBoneTranslationRetargetingMode::AnimationRelative, true);
					 }
#endif

					 // Some character types share a skeleton.  Get the mapped name.
					 FString MappedSkeletonName = DazImportData.CharacterTypeName;
					 if (CachedSettings->CharacterTypeMapping.Contains(DazImportData.CharacterTypeName))
					 {
						 MappedSkeletonName = CachedSettings->CharacterTypeMapping[DazImportData.CharacterTypeName];
					 }
					 
					 // Add this skeleton as the default for this character type
					 if (DazImportData.bFixTwistBones)
					 {
						 if (!CachedSettings->SkeletonsWithTwistFix.Contains(MappedSkeletonName))
						 {
							 CachedSettings->SkeletonsWithTwistFix.Add(MappedSkeletonName, Skeleton);
						 }
					 }
					 else
					 {
						 if (!CachedSettings->OtherSkeletons.Contains(MappedSkeletonName))
						 {
							 CachedSettings->OtherSkeletons.Add(MappedSkeletonName, Skeleton);
						 }
					 }
					 CachedSettings->SaveConfig(CPF_Config, *CachedSettings->GetDefaultConfigFilename());
				}
		  }
	 }

	 if (ImportedAssets.Num() > 0)
	 {
		  return ImportedAssets[0];
	 }
	 return nullptr;
}

void FDazToUnrealModule::InstallDazStudioPlugin()
{
	 FString InstallerPath = IPluginManager::Get().FindPlugin("DazToUnreal")->GetBaseDir() / TEXT("Resources") / TEXT("DazToUnrealSetup.exe");
	 FString InstallerAbsolutePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*InstallerPath);
	 FPlatformProcess::LaunchFileInDefaultExternalApplication(*InstallerAbsolutePath, NULL, ELaunchVerb::Open);
}

void FDazToUnrealModule::AddCreateRetargeterMenu()
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
	// Create a new context menu item for Skeletal Meshes
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh");
	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

	Section.AddSubMenu(
		"CreateIKRetargeterSubMenu",
		LOCTEXT("CreateIKRetargeterSubMenu_Label", "Create IK Retargeter"),
		LOCTEXT("CreateIKRetargeterSubMenu_ ToolTip", "Create or update and IKRetargeter for this mesh."),
		FNewToolMenuDelegate::CreateRaw(this, &FDazToUnrealModule::AddCreateRetargeterSubMenu),
		false);
#endif
}

void FDazToUnrealModule::AddCreateRetargeterSubMenu(UToolMenu* Menu)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
	// Get selected SkeletalMesh
	USkeletalMesh* TargetSkeletalMesh = nullptr;
	if (const UContentBrowserAssetContextMenuContext* CBContext = Menu->Context.FindContext<UContentBrowserAssetContextMenuContext>())
	{
		TargetSkeletalMesh = CBContext->LoadFirstSelectedObject<USkeletalMesh>();
	}


	FToolMenuSection& Section = Menu->AddSection("SourceMesh", LOCTEXT("SourceMesh_Label", "Source Mesh"));

	// Find all SkeletalMeshes
	TArray<FAssetData> Assets;
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.GetAssetsByClass(USkeletalMesh::StaticClass()->GetClassPathName(), Assets);

	// Add a menu entry for each SkeletalMesh
	for (FAssetData Asset : Assets)
	{
		const TAttribute<FText> Label = FText::FromString(Asset.AssetName.ToString());
		FName Name = FName(Asset.AssetName.ToString());

		Section.AddMenuEntry(
			Name,
			Label,
		LOCTEXT("CreateSkeletalMeshToolSubMenuItemTip", "Choose this as the source asset for creating a retargeter."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FDazToUnrealModule::OnCreateRetargeterClicked, Asset.GetSoftObjectPath(), TargetSkeletalMesh))
		);
 
	}
#endif
}

void FDazToUnrealModule::OnCreateRetargeterClicked(FSoftObjectPath SourceObjectPath, USkeletalMesh* TargetSkeletalMesh)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
	USkeletalMesh* SourceSkeletalMesh = Cast<USkeletalMesh>(SourceObjectPath.TryLoad());
	if (!SourceSkeletalMesh || !TargetSkeletalMesh) return;

	// Find or Create the Source IKRig
	UIKRigDefinition* SourceIKRig = FindIKRigForSkeletalMesh(SourceSkeletalMesh);
	if (!SourceIKRig)
	{
		FString SkeletalMeshPackagePath = SourceSkeletalMesh->GetOutermost()->GetPathName() + TEXT(".") + SourceSkeletalMesh->GetName();
		FString CreateIKRigCommand = FString::Format(TEXT("py CreateIKRig.py --skeletalMesh={0}"), { SkeletalMeshPackagePath });
		UE_LOG(LogDazToUnreal, Log, TEXT("Creating Source IK Rig with command: %s"), *CreateIKRigCommand);
		GEngine->Exec(NULL, *CreateIKRigCommand);
		SourceIKRig = FindIKRigForSkeletalMesh(SourceSkeletalMesh);
	}

	// Find or Create the Target IKRig
	UIKRigDefinition* TargetIKRig = FindIKRigForSkeletalMesh(TargetSkeletalMesh);
	if (!TargetIKRig)
	{
		FString SkeletalMeshPackagePath = TargetSkeletalMesh->GetOutermost()->GetPathName() + TEXT(".") + TargetSkeletalMesh->GetName();
		FString CreateIKRigCommand = FString::Format(TEXT("py CreateIKRig.py --skeletalMesh={0}"), { SkeletalMeshPackagePath });
		UE_LOG(LogDazToUnreal, Log, TEXT("Creating Source IK Rig with command: %s"), *CreateIKRigCommand);
		GEngine->Exec(NULL, *CreateIKRigCommand);
		TargetIKRig = FindIKRigForSkeletalMesh(TargetSkeletalMesh);
	}

	// Create or Update the IKRetargeter
	if (SourceIKRig && TargetIKRig)
	{
		TargetSkeletalMesh->GetSkeleton()->UpdateReferencePoseFromMesh(TargetSkeletalMesh);
		FString SourceIKRigPackagePath = SourceIKRig->GetOutermost()->GetPathName() + TEXT(".") + SourceIKRig->GetName();
		FString TargetIKRigPackagePath = TargetIKRig->GetOutermost()->GetPathName() + TEXT(".") + TargetIKRig->GetName();
		FString CreateIKRetargeterCommand = FString::Format(TEXT("py CreateIKRetargeter.py --sourceIKRig={0} --targetIKRig={1}"), { SourceIKRigPackagePath, TargetIKRigPackagePath });
		UE_LOG(LogDazToUnreal, Log, TEXT("Creating IK Retargeter with command: %s"), *CreateIKRetargeterCommand);
		GEngine->Exec(NULL, *CreateIKRetargeterCommand);
	}
#endif
}

UIKRigDefinition* FDazToUnrealModule::FindIKRigForSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
	TArray<FAssetData> Assets;
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.GetAssetsByClass(UIKRigDefinition::StaticClass()->GetClassPathName(), Assets);

	UIKRigDefinition* SourceIKRig = nullptr;

	// First try to find an IKRig that has this mesh as the preview.
	for (FAssetData Asset : Assets)
	{
		if (UIKRigDefinition* IKRigDefinition = Cast<UIKRigDefinition>(Asset.GetAsset()))
		{
			if (IKRigDefinition->PreviewSkeletalMesh == SkeletalMesh)
			{
				return IKRigDefinition;
			}
		}
	}
#endif
	return nullptr;
}

void FDazToUnrealModule::AddCreateFullBodyIKControlRigMenu()
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	// Create a new context menu item for Skeletal Meshes
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh");
	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

	//USkeletalMesh* TargetSkeletalMesh = nullptr;
	//if (const UContentBrowserAssetContextMenuContext* CBContext = Menu->Context.FindContext<UContentBrowserAssetContextMenuContext>())
	//{
	//	TargetSkeletalMesh = CBContext->LoadFirstSelectedObject<USkeletalMesh>();
	//}

	Section.AddDynamicEntry("CreateFBIKControlRig", FNewToolMenuSectionDelegate::CreateLambda(
		[this](FToolMenuSection& Section)
		{

			if (UContentBrowserAssetContextMenuContext* Context = Section.FindContext<UContentBrowserAssetContextMenuContext>())
			{
				if (Context->SelectedAssets.Num() > 0)
				{
					Section.AddMenuEntry(
						FName(TEXT("CreateFullBodyIKControlRigMenu")),
						LOCTEXT("CreateFullBodyIKControlRigLabel", "Create FBIK Control Rig"),
						LOCTEXT("CreateFullBodyIKControlRigLabelTip", "Creates a Control Rig based around a Full Body IK Node"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateRaw(this, &FDazToUnrealModule::OnCreateFullBodyIKControlRigClicked, Context->SelectedAssets[0].GetSoftObjectPath()))
					);
				}
			}
		}
	));

#endif
}

void FDazToUnrealModule::OnCreateFullBodyIKControlRigClicked(FSoftObjectPath SourceObjectPath)
{
	FString SkeletalMeshPackagePath = SourceObjectPath.ToString();//SourceSkeletalMesh->GetOutermost()->GetPathName() + TEXT(".") + SourceSkeletalMesh->GetName();
	FString DTUPath = FDazToUnrealUtils::GetDTUPathForModel(SourceObjectPath);
	FString CreateControlRigCommand = FString::Format(TEXT("py CreateControlRig.py --skeletalMesh={0} --dtuFile=\"{1}\""), { SkeletalMeshPackagePath, DTUPath });
	UE_LOG(LogDazToUnreal, Log, TEXT("Creating FBIK Control Rig with command: %s"), *CreateControlRigCommand);
	GEngine->Exec(NULL, *CreateControlRigCommand);
}

void FDazToUnrealModule::AddCreateIKLimbBasedControlRigMenu()
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	// Create a new context menu item for Skeletal Meshes
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh");
	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

	//USkeletalMesh* TargetSkeletalMesh = nullptr;
	//if (const UContentBrowserAssetContextMenuContext* CBContext = Menu->Context.FindContext<UContentBrowserAssetContextMenuContext>())
	//{
	//	TargetSkeletalMesh = CBContext->LoadFirstSelectedObject<USkeletalMesh>();
	//}

	Section.AddDynamicEntry("CreateIKLimbBasedControlRig", FNewToolMenuSectionDelegate::CreateLambda(
		[this](FToolMenuSection& Section)
		{

			if (UContentBrowserAssetContextMenuContext* Context = Section.FindContext<UContentBrowserAssetContextMenuContext>())
			{
				if (Context->SelectedAssets.Num() > 0)
				{
					Section.AddMenuEntry(
						FName(TEXT("CreateIKLimbBasedControlRigMenu")),
						LOCTEXT("CreateIKLimbBasedControlRigLabel", "Create IK Limb Based Control Rig"),
						LOCTEXT("CreateIKLimbBasedControlRigLabelTip", "Creates a Control Rig with per limb IK"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateRaw(this, &FDazToUnrealModule::OnCreateIKLimbBasedControlRigClicked, Context->SelectedAssets[0].GetSoftObjectPath()))
					);
				}
			}
		}
	));

#endif
}

void FDazToUnrealModule::OnCreateIKLimbBasedControlRigClicked(FSoftObjectPath SourceObjectPath)
{
	FString SkeletalMeshPackagePath = SourceObjectPath.ToString();//SourceSkeletalMesh->GetOutermost()->GetPathName() + TEXT(".") + SourceSkeletalMesh->GetName();
	FString DTUPath = FDazToUnrealUtils::GetDTUPathForModel(SourceObjectPath);
	FString CreateControlRigCommand = FString::Format(TEXT("py CreateIKLimbBasedControlRig.py --skeletalMesh={0} --dtuFile=\"{1}\""), { SkeletalMeshPackagePath, DTUPath });
	UE_LOG(LogDazToUnreal, Log, TEXT("Creating IK Limb Based Control Rig with command: %s"), *CreateControlRigCommand);
	GEngine->Exec(NULL, *CreateControlRigCommand);
}

void FDazToUnrealModule::AddConvertToEpicSkeletonMenu()
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	// Create a new context menu item for Skeletal Meshes
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.SkeletalMesh");
	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

	Section.AddSubMenu(
		FName(TEXT("ConvertToEpicSkeletonMenu")),
		LOCTEXT("ConvertToEpicSkeletonLabel", "Convert To Epic Skeleton"),
		LOCTEXT("ConvertToEpicSkeletonLabelTip", "Converts the skeletal mesh to use the Epic Skeleton"),
		FNewToolMenuDelegate::CreateRaw(this, &FDazToUnrealModule::AddConvertToEpicSkeletonSubMenu),
		false);

#endif
}

void FDazToUnrealModule::AddConvertToEpicSkeletonSubMenu(UToolMenu* Menu)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
	// Get selected SkeletalMesh
	USkeletalMesh* TargetSkeletalMesh = nullptr;
	if (const UContentBrowserAssetContextMenuContext* CBContext = Menu->Context.FindContext<UContentBrowserAssetContextMenuContext>())
	{
		TargetSkeletalMesh = CBContext->LoadFirstSelectedObject<USkeletalMesh>();
	}


	FToolMenuSection& Section = Menu->AddSection("SourceMesh", LOCTEXT("RetargetToEpicSkeletonSourceMesh_Label", "Source Mesh"));

	// Find all SkeletalMeshes
	TArray<FAssetData> Assets;
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.GetAssetsByClass(USkeletalMesh::StaticClass()->GetClassPathName(), Assets);

	// Add a menu entry for each SkeletalMesh
	for (FAssetData Asset : Assets)
	{
		const TAttribute<FText> Label = FText::FromString(Asset.AssetName.ToString());
		FName Name = FName(Asset.AssetName.ToString());

		Section.AddMenuEntry(
			Name,
			Label,
			LOCTEXT("RetargetToEpicSkeletonSubMenuItemTip", "Choose this as the target Epic Skeleton."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDazToUnrealModule::OnConvertToEpicSkeletonClicked, Asset.GetSoftObjectPath(), TargetSkeletalMesh))
		);

	}
#endif
}

void FDazToUnrealModule::OnConvertToEpicSkeletonClicked(FSoftObjectPath EpicMeshObjectPath, class USkeletalMesh* SkeletalMeshToUpdate)
{
	USkeletalMesh* TargetEpicSkeletalMesh = Cast<USkeletalMesh>(EpicMeshObjectPath.TryLoad());
	UDazToUnrealBlueprintUtils::ConvertToEpicSkeleton(SkeletalMeshToUpdate, TargetEpicSkeletalMesh);
}

bool FDazToUnrealModule::PreProcessFbxFile(
	FScopedSlowTask &Progress,
	FString &FBXFile,
	DazAssetType &AssetType, 
	const UDazToUnrealSettings* CachedSettings,
	FString &AssetName,
	DazToUnrealImportData &ImportData,
	TSharedPtr<FJsonObject> &JsonObject,
	DazMaterialCombineType &MaterialCombineMethod,
	TMap<TSharedPtr<FJsonValue>, TSharedPtr<FJsonValue>> &DuplicateMaterials,
	TMap<FString, TArray<FDUFTextureProperty>> &DtuMaterialsTable,
	FString &FBXPath,
	FString &RootBoneName, TArray<FString> &MaterialSlotNames)
{
	///////////////////////////////////////////////////////////////////////////////
	//// Start of FBX preprocessing before actual import
	///////////////////////////////////////////////////////////////////////////////

	// Load the FBX file
	FbxManager* SdkManager = FbxManager::Create();

	// create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(SdkManager, IOSROOT);
	SdkManager->SetIOSettings(ios);

	// Create the geometry converter
	Progress.EnterProgressFrame(1, LOCTEXT("LoadingFBX", "Loading FBX for Updating"));
	FbxGeometryConverter* GeometryConverter = new FbxGeometryConverter(SdkManager);

	FbxImporter* Importer = FbxImporter::Create(SdkManager, "");
	const bool bImportStatus = Importer->Initialize(TCHAR_TO_UTF8(*FBXFile));
	FbxScene* Scene = FbxScene::Create(SdkManager, "");
	Importer->Import(Scene);

	FbxNode* RootNode = Scene->GetRootNode();

	// Find the root bone.  There should only be one bone off the scene root
	FbxNode* RootBone = nullptr;

	if (AssetType != DazAssetType::R2x) 
	{
		RootBone = FDazToUnrealFbx::FindRootBone(RootBoneName, RootNode, Scene, AssetType, CachedSettings, AssetName);

		// FDazToUnrealFbx::RenameDuplicateBones(RootBone);

		// FDazToUnrealFbx::DetachGeometryFromSkeleton(RootNode, Scene);

		// FDazToUnrealFbx::AddIKBones(RootBone, Scene, CachedSettings);

		// Take twist bones out of the chain
		if (AssetType == DazAssetType::SkeletalMesh && ImportData.bFixTwistBones)
		{
			// FDazToUnrealFbx::FixTwistBones(RootBone);
		}

		Progress.EnterProgressFrame(1, LOCTEXT("CombiningMorphs", "Combining Morphs")); 
		// FDazToUnrealFbx::ProcessMorphs(Scene, CachedSettings, JsonObject);
	}

/**

	// Get FBX scene materials
	FbxArray<FbxSurfaceMaterial*> FbxMaterialArray;
	Scene->FillMaterialArray(FbxMaterialArray);

	// Create a mapping of the names of duplicate (identical) materials
	if (MaterialCombineMethod != DazMaterialCombineType::NoCombine)
	{
		TMap<FString, FString> DuplicateToOriginalName;
		for (auto DuplicateMaterialPair : DuplicateMaterials)
		{
			TSharedPtr<FJsonObject> DuplicateMaterial = DuplicateMaterialPair.Key->AsObject();
			FString DuplicateMaterialName = DuplicateMaterial->GetStringField(TEXT("Material Name"));

			TSharedPtr<FJsonObject> OriginalMaterial = DuplicateMaterialPair.Value->AsObject();
			FString OriginalMaterialName = OriginalMaterial->GetStringField(TEXT("Material Name"));

			DuplicateToOriginalName.Add(DuplicateMaterialName, OriginalMaterialName);
		}

		// Remap FBX Surfaces to remove references to duplicate materials
		TMap<FString, FbxSurfaceMaterial*> MaterialNameToFbxMaterial;
		for (int32 MaterialIndex = FbxMaterialArray.Size() - 1; MaterialIndex >= 0; --MaterialIndex)
		{
			FbxSurfaceMaterial* Material = FbxMaterialArray[MaterialIndex];
			FString OriginalMaterialName = UTF8_TO_TCHAR(Material->GetName());
			MaterialNameToFbxMaterial.Add(OriginalMaterialName, Material);
		}

		for (int32 MeshIndex = Scene->GetGeometryCount() - 1; MeshIndex >= 0; --MeshIndex)
		{
			FbxArray<FbxSurfaceMaterial*> NewMaterialArray;
			FbxGeometry* Geometry = Scene->GetGeometry(MeshIndex);
			FbxNode* GeometryNode = Geometry->GetNode();
			int32 MaterialCount = GeometryNode->GetMaterialCount();
			for (int32 AddIndex = 0; AddIndex < MaterialCount; AddIndex++)
			{
				FbxSurfaceMaterial* MaterialToReplace = GeometryNode->GetMaterial(AddIndex);
				FString MaterialToReplaceName = UTF8_TO_TCHAR(MaterialToReplace->GetName());
				if (DuplicateToOriginalName.Contains(MaterialToReplaceName) && MaterialNameToFbxMaterial.Contains(DuplicateToOriginalName[MaterialToReplaceName]))
				{
					NewMaterialArray.Add(MaterialNameToFbxMaterial[DuplicateToOriginalName[MaterialToReplaceName]]);
				}
				else
				{
					NewMaterialArray.Add(MaterialToReplace);
				}

			}

			GeometryNode->RemoveAllMaterials();
			for (int32 AddIndex = 0; AddIndex < MaterialCount; AddIndex++)
			{
				GeometryNode->AddMaterial(NewMaterialArray[AddIndex]);
			}
		}
	}

	// Rename Materials
// DB 2025-06-11 added to arguments
//	TArray<FString> MaterialSlotNames;
	for (int32 MaterialIndex = FbxMaterialArray.Size() - 1; MaterialIndex >= 0; --MaterialIndex)
	{
		FbxSurfaceMaterial* FbxMaterial = FbxMaterialArray[MaterialIndex];
		FString OriginalMaterialName = UTF8_TO_TCHAR(FbxMaterial->GetName());
		FString MaterialFbxObjectName = FDazToUnrealFbx::GetObjectNameForMaterial(FbxMaterial);
		FString MaterialObjectName = FDazToUnrealMaterials::GetFriendlyObjectName(FDazToUnrealUtils::SanitizeName(MaterialFbxObjectName), DtuMaterialsTable);

		FString NewMaterialName;
		if (CachedSettings->UseOriginalMaterialName)
		{
			NewMaterialName = OriginalMaterialName;
		}
		else
		{
			NewMaterialName = MaterialObjectName + TEXT("_") + OriginalMaterialName;
		}

		NewMaterialName = FDazToUnrealUtils::SanitizeName(NewMaterialName);
		FbxMaterial->SetName(TCHAR_TO_UTF8(*NewMaterialName));
		// if (DtuMaterialsTable.Contains(NewMaterialName))
		if (DtuMaterialsTable.Contains(OriginalMaterialName))
		{
			MaterialSlotNames.Add(NewMaterialName);
			ImportData.MaterialSlotNameToMaterialName.Add(FName(NewMaterialName), FName(FDazToUnrealUtils::SanitizeName(OriginalMaterialName)));
		}
		else
		{
			// TODO: Not sure this is needed anymore
			// search all materialproperties for partial match
			bool bPartialMatchFound = false;
			for (auto keyvalPair : DtuMaterialsTable)
			{
				if (keyvalPair.Key.Contains(TEXT("_") + OriginalMaterialName))
				{
					MaterialSlotNames.Add(keyvalPair.Key);
					ImportData.MaterialSlotNameToMaterialName.Add(FName(keyvalPair.Key), FName(FDazToUnrealUtils::SanitizeName(OriginalMaterialName)));
					bPartialMatchFound = true;
					break;
				}
			}
			if (bPartialMatchFound == false)
			{
				for (int32 MeshIndex = Scene->GetGeometryCount() - 1; MeshIndex >= 0; --MeshIndex)
				{
					FbxGeometry* Geometry = Scene->GetGeometry(MeshIndex);
					FbxNode* GeometryNode = Geometry->GetNode();
					if (GeometryNode->GetMaterialIndex(TCHAR_TO_UTF8(*NewMaterialName)) != -1)
					{
						UE_LOG(LogDazToUnreal, Warning, TEXT("Material %s not found in material properties, removing geometry..."), *NewMaterialName);
						Scene->RemoveGeometry(Geometry);
					}
				}
				Scene->RemoveMaterial(FbxMaterial);
			}
		}

	}

**/

	Progress.EnterProgressFrame(1, LOCTEXT("WritingUpdatedFBX", "Writing Updated FBX"));
	if (FDazToUnrealFbx::SaveUpdatedFbxFile(SdkManager, Scene, RootBone,
		FBXFile, FBXPath, AssetName, CachedSettings, ImportData) == false) 
	{
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	//// End of FBX preprocessing before actual import
	///////////////////////////////////////////////////////////////////////////////

	return true;
}

bool FDazToUnrealModule::ImportGroom(FString sGroomFilename, FString sDestinationGamePath, TSharedPtr<FJsonObject> JsonObject)
{
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AlembicHairTranslatorModule")))
	{
		if (!FModuleManager::Get().LoadModule(TEXT("AlembicHairTranslatorModule")))
		{
			UE_LOG(LogTemp, Error, TEXT("DazToUnreal: AlembicHairTranslatorModule (Alembic Groom Importer) module is not loaded and could not be loaded. Enable the plugin in the Editor before importing."));
			return false;
		}
	}

	//// Import assets
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FString> FileNames;
	FileNames.Add(sGroomFilename);

	UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>(UAutomatedAssetImportData::StaticClass());
	ImportData->FactoryName = TEXT("GroomImportFactory");
	ImportData->Filenames = FileNames;
	ImportData->DestinationPath = sDestinationGamePath;
	ImportData->bReplaceExisting = true;

	TArray<UObject*> ImportedAssets;
	try
	{
		ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		// ImportedAssets = AssetToolsModule.Get().ImportAssets(FileNames, sDestinationGamePath);
	}
	catch (...)
	{
		UE_LOG(LogTemp, Warning, TEXT(">> Importing Groom Assets failed."));
		return false;
	}

	return true;
}

bool FDazToUnrealModule::CreateRelatedMaterials(
	TArray<FString> RelatedMaterialNamesList,
	USubsurfaceProfile* MasterSubsurfaceProfile,
	TMap<FString, TArray<FDUFTextureProperty>>& DtuMaterialsTable,
	FString BaseMaterialName,
	FString ChildMaterialFolder,
	FString CharacterMaterialFolder,
	FString CharacterTexturesFolder,
	DazCharacterType CharacterType)
{
	// Create Base Material Properties
	TArray<FDUFTextureProperty> MostCommonProperties = FDazToUnrealMaterials::GetMostCommonProperties(RelatedMaterialNamesList, DtuMaterialsTable);
	DtuMaterialsTable.Add(BaseMaterialName, MostCommonProperties);
	//DtuMaterialsTable[BaseMaterialName] = DtuMaterialsTable[RelatedMaterialNamesList[0]];

	// Create Base Material
	FSoftObjectPath BaseMaterialPath = FDazToUnrealMaterials::GetMostCommonBaseMaterial(RelatedMaterialNamesList, DtuMaterialsTable);//FDazToUnrealMaterials::GetBaseMaterial(RelatedMaterialNamesList[0], DtuMaterialsTable[BaseMaterialName]);
	UObject* BaseMaterial = BaseMaterialPath.TryLoad();
	UMaterialInstanceConstant* UnrealMaterialConstant = FDazToUnrealMaterials::CreateMaterial(CharacterMaterialFolder, CharacterTexturesFolder, BaseMaterialName, DtuMaterialsTable, CharacterType, Cast<UMaterialInterface>(BaseMaterial), MasterSubsurfaceProfile);
	//UnrealMaterialConstant->SetParentEditorOnly((UMaterial*)BaseMaterial);

	for (FString UnrealMaterialName : RelatedMaterialNamesList)
	{
		USubsurfaceProfile* SubsurfaceProfile = MasterSubsurfaceProfile;
		if (!FDazToUnrealMaterials::SubsurfaceProfilesWouldBeIdentical(MasterSubsurfaceProfile, DtuMaterialsTable[UnrealMaterialName]))
		{
			SubsurfaceProfile = FDazToUnrealMaterials::CreateSubsurfaceProfileForMaterial(UnrealMaterialName, ChildMaterialFolder, DtuMaterialsTable[UnrealMaterialName]);
		}

		bool bIsChildOfBaseMaterial = (FDazToUnrealMaterials::GetBaseMaterial(UnrealMaterialName, DtuMaterialsTable[UnrealMaterialName]) == BaseMaterialPath);
		if (!bIsChildOfBaseMaterial)
		{
			FDazToUnrealMaterials::CreateMaterial(ChildMaterialFolder, CharacterTexturesFolder, UnrealMaterialName, DtuMaterialsTable, CharacterType, nullptr, SubsurfaceProfile);
		}
		else if (bIsChildOfBaseMaterial)
		{
			// Iterate through and remove duplicate properties from child material
			int32 NumMaterialProperties = DtuMaterialsTable[UnrealMaterialName].Num();
			for (int32 PropertyIndex = NumMaterialProperties-1; PropertyIndex >= 0; PropertyIndex--)
			{
				FDUFTextureProperty ChildMaterialProperty = DtuMaterialsTable[UnrealMaterialName][PropertyIndex];
				if (ChildMaterialProperty.Name == TEXT("Asset Type")) continue;
				for (FDUFTextureProperty BaseMaterialProperty : DtuMaterialsTable[BaseMaterialName])
				{
					bool bIsDuplicateProperty = (BaseMaterialProperty.Name == ChildMaterialProperty.Name && BaseMaterialProperty.Value == ChildMaterialProperty.Value);
					if (bIsDuplicateProperty)
					{
						// 2025-07-03: DO NOT REMOVE IF PROPERTY IS RELATED TO ALPHA / CUTOUT / OPACITY
						if (ChildMaterialProperty.Name.Contains(TEXT("Cutout")) || 
							ChildMaterialProperty.Name.Contains(TEXT("Opacity")) ||
							ChildMaterialProperty.Name.Contains(TEXT("Refraction")) )
						{
							continue;
						}
						// 2025-07-03: DESIGN FLAW: PROPERTIES ARE REMOVED FROM CHILD MATERIALS BEFORE LOGIC TO CHOOSE BETWEEN USING PARENT MATERIAL VS BASE MATERIAL (See CreateMaterial() )
						UE_LOG(LogDazToUnreal, Warning, TEXT("Removing Material Property [%i] of %s-%s to keep parent property: %s ..."), PropertyIndex, *UnrealMaterialName, *ChildMaterialProperty.Name, *BaseMaterialName);
						DtuMaterialsTable[UnrealMaterialName].RemoveAt(PropertyIndex);
						break;
					}
				}
			}
			FDazToUnrealMaterials::CreateMaterial(ChildMaterialFolder, CharacterTexturesFolder, UnrealMaterialName, DtuMaterialsTable, CharacterType, UnrealMaterialConstant, SubsurfaceProfile);
		}
	}

	return true;
}

void FDazToUnrealModule::FixForFab(FString sTargetPath, FString sSkeletalMeshPath)
{
	const UDazToUnrealSettings* CachedSettings = GetDefault<UDazToUnrealSettings>();
	FString DazCommonFolder = CachedSettings->ImportDirectory.Path + TEXT("/Common");

	FString GameDemoFolder = sTargetPath + TEXT("/Demo");
	FString GameMapFolder = sTargetPath + TEXT("/Map");

	FString ProjectContentDir = FPaths::ProjectContentDir();
	FString FullGameDemoFolder = GameDemoFolder.Replace(TEXT("/Game/"), *ProjectContentDir);
	FString FullGameMapFolder = GameMapFolder.Replace(TEXT("/Game/"), *ProjectContentDir);

	if (!FDazToUnrealUtils::MakeDirectoryAndCheck(FullGameDemoFolder)) {
		UE_LOG(LogDazToUnreal, Error, TEXT("Could not create directory %s"), *FullGameDemoFolder);
		return;
	}
	if (!FDazToUnrealUtils::MakeDirectoryAndCheck(FullGameMapFolder)) {
		UE_LOG(LogDazToUnreal, Error, TEXT("Could not create directory %s"), *FullGameMapFolder);
		return;
	}

	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/SK_Mannequin"), GameDemoFolder);
	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/MM_Idle"), GameDemoFolder);
	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/MM_Walk_Fwd"), GameDemoFolder);
	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/MM_Run_Fwd"), GameDemoFolder);
	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/MM_Jump"), GameDemoFolder);
	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/MM_Fall_Loop"), GameDemoFolder);
	FDazToUnrealUtils::MoveSingleAsset(TEXT("/Game/MM_Land"), GameDemoFolder);

	// FDazToUnrealUtils::ReplaceSkeleton(GameDemoFolder / TEXT("MM_Idle"), GameDemoFolder / TEXT("SK_Mannequin"));
	// FDazToUnrealUtils::ReplaceSkeleton(GameDemoFolder / TEXT("MM_Walk_Fwd"), GameDemoFolder / TEXT("SK_Mannequin"));
	// FDazToUnrealUtils::ReplaceSkeleton(GameDemoFolder / TEXT("MM_Run_Fwd"), GameDemoFolder / TEXT("SK_Mannequin"));
	// FDazToUnrealUtils::ReplaceSkeleton(GameDemoFolder / TEXT("MM_Jump"), GameDemoFolder / TEXT("SK_Mannequin"));
	// FDazToUnrealUtils::ReplaceSkeleton(GameDemoFolder / TEXT("MM_Fall_Loop"), GameDemoFolder / TEXT("SK_Mannequin"));
	// FDazToUnrealUtils::ReplaceSkeleton(GameDemoFolder / TEXT("MM_Land"), GameDemoFolder / TEXT("SK_Mannequin"));

	FDazToUnrealUtils::MakeNewFabLevel(GameMapFolder / TEXT("Overview"));


	FSoftObjectPath sUESkeletonPath = FSoftObjectPath(GameDemoFolder / TEXT("SK_Mannequin"));
	USkeleton* pUESkeleton = Cast<USkeleton>(sUESkeletonPath.TryLoad());

	FSoftObjectPath MeshPath = FSoftObjectPath(sSkeletalMeshPath);
	USkeletalMesh* pMesh = Cast<USkeletalMesh>(MeshPath.TryLoad());
#if UE_VERSION_NEWER_THAN(4, 26, 99)
	USkeleton* pMeshSkeleton = pMesh ? pMesh->GetSkeleton() : nullptr;
#else
	USkeleton* pMeshSkeleton = pMesh ? pMesh->Skeleton : nullptr;
#endif
	UE_LOG(LogTemp, Warning, TEXT("Mesh path: %s, Mesh: %s"), *sSkeletalMeshPath, pMesh ? *pMesh->GetName() : TEXT("NULL"));

	FDazToUnrealUtils::AddCompatibleSkeleton(pMeshSkeleton, pUESkeleton);

	FDazToUnrealUtils::AssignSkeletalMeshToActor("Character", pMesh);
	FDazToUnrealUtils::AssignSkeletalMeshToActor("Idle", pMesh);
	FDazToUnrealUtils::AssignSkeletalMeshToActor("Run", pMesh);
	FDazToUnrealUtils::AssignSkeletalMeshToActor("Walk", pMesh);
	FDazToUnrealUtils::AssignSkeletalMeshToActor("Fall", pMesh);
	FDazToUnrealUtils::AssignSkeletalMeshToActor("Jump", pMesh);
	FDazToUnrealUtils::AssignSkeletalMeshToActor("Land", pMesh);

	FSoftObjectPath IdleAnimPath = FSoftObjectPath(GameDemoFolder / TEXT("MM_Idle.MM_Idle"));
	FSoftObjectPath WalkAnimPath = FSoftObjectPath(GameDemoFolder / TEXT("MM_Walk_Fwd.MM_Walk_Fwd"));
	FSoftObjectPath RunAnimPath = FSoftObjectPath(GameDemoFolder / TEXT("MM_Run_Fwd.MM_Run_Fwd"));
	FSoftObjectPath JumpAnimPath = FSoftObjectPath(GameDemoFolder / TEXT("MM_Jump.MM_Jump"));
	FSoftObjectPath FallAnimPath = FSoftObjectPath(GameDemoFolder / TEXT("MM_Fall_Loop.MM_Fall_Loop"));
	FSoftObjectPath LandAnimPath = FSoftObjectPath(GameDemoFolder / TEXT("MM_Land.MM_Land"));
	UAnimSequence* IdleAnim = Cast<UAnimSequence>(IdleAnimPath.TryLoad());
	UAnimSequence* WalkAnim = Cast<UAnimSequence>(WalkAnimPath.TryLoad());
	UAnimSequence* RunAnim = Cast<UAnimSequence>(RunAnimPath.TryLoad());
	UAnimSequence* JumpAnim = Cast<UAnimSequence>(JumpAnimPath.TryLoad());
	UAnimSequence* FallAnim = Cast<UAnimSequence>(FallAnimPath.TryLoad());
	UAnimSequence* LandAnim = Cast<UAnimSequence>(LandAnimPath.TryLoad());

	UE_LOG(LogTemp, Warning, TEXT("Idle path: %s, Idle: %s"), *(GameDemoFolder / TEXT("MM_Idle.MM_Idle")), IdleAnim ? *IdleAnim->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Walk path: %s, Walk: %s"), *(GameDemoFolder / TEXT("MM_Walk_Fwd.MM_Walk_Fwd")), WalkAnim ? *WalkAnim->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Run path: %s, Run: %s"), *(GameDemoFolder / TEXT("MM_Run_Fwd.MM_Run_Fwd")), RunAnim ? *RunAnim->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Jump path: %s, Jump: %s"), *(GameDemoFolder / TEXT("MM_Jump.MM_Jump")), JumpAnim ? *JumpAnim->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Fall path: %s, Fall: %s"), *(GameDemoFolder / TEXT("MM_Fall_Loop.MM_Fall_Loop")), FallAnim ? *FallAnim->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("Land path: %s, Land: %s"), *(GameDemoFolder / TEXT("MM_Land.MM_Land")), LandAnim ? *LandAnim->GetName() : TEXT("NULL"));

	FDazToUnrealUtils::AssignAnimSequenceToActor("Idle", IdleAnim);
	FDazToUnrealUtils::AssignAnimSequenceToActor("Walk", WalkAnim);
	FDazToUnrealUtils::AssignAnimSequenceToActor("Run", RunAnim);
	FDazToUnrealUtils::AssignAnimSequenceToActor("Jump", JumpAnim);
	FDazToUnrealUtils::AssignAnimSequenceToActor("Fall", FallAnim);
	FDazToUnrealUtils::AssignAnimSequenceToActor("Land", LandAnim);

	if (FDazToUnrealUtils::BakeLightingForCurrentMap()) {
		UE_LOG(LogDazToUnreal, Log, TEXT("Lighting bake started successfully."));
	} else {
		UE_LOG(LogDazToUnreal, Error, TEXT("Lighting bake failed."));
	}

	FDazToUnrealUtils::SaveCurrentLevel();

}

#if UE_VERSION_NEWER_THAN(5,2,99)
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
#include "Materials/MaterialExpressionOneMinus.h"
#include "Engine/StaticMeshActor.h"
bool FDazToUnrealModule::ImportFbxForFab(FString sFbxPath, FString sDestinationGamePath)
{
	 static FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	// --- Create subdirectories ---
	const FString sMeshPath = sDestinationGamePath / TEXT("Mesh");
	const FString sMaterialPath = sDestinationGamePath / TEXT("Materials");
	const FString sTexturePath = sDestinationGamePath / TEXT("Textures");
	const FString sMapPath = sDestinationGamePath / TEXT("Map");

	UEditorAssetLibrary::MakeDirectory(sMeshPath);
	UEditorAssetLibrary::MakeDirectory(sMaterialPath);
	UEditorAssetLibrary::MakeDirectory(sTexturePath);
	UEditorAssetLibrary::MakeDirectory(sMapPath);

	FString sProjectContentDir = FPaths::ProjectContentDir();
	FString sFullMeshPath = sMeshPath.Replace(TEXT("/Game/"), *sProjectContentDir);
	FString sFullTexturePath = sTexturePath.Replace(TEXT("/Game/"), *sProjectContentDir);
	FString sFullMaterialPath = sMaterialPath.Replace(TEXT("/Game/"), *sProjectContentDir);

	FDazToUnrealUtils::MakeNewFabLevel(sMapPath / TEXT("Overview"));

	 TArray<FString> FileNames;
	 FileNames.Add(sFbxPath);

	 UFbxFactory* FbxFactory = NewObject<UFbxFactory>(UFbxFactory::StaticClass());
	 FbxFactory->AddToRoot();

	 UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
	 FbxFactory->SetDetectImportTypeOnImport(false);
	 FbxFactory->ImportUI->TextureImportData->MaterialSearchLocation = EMaterialSearchLocation::Local;
	 FbxFactory->ImportUI->bImportMaterials = true;
	 FbxFactory->ImportUI->bImportTextures = true;
	FbxFactory->ImportUI->bImportAsSkeletal = false;

	FbxFactory->ImportUI->StaticMeshImportData->bForceFrontXAxis = false;
	FbxFactory->ImportUI->StaticMeshImportData->bCombineMeshes = true;
	FbxFactory->ImportUI->StaticMeshImportData->bAutoGenerateCollision = true;
	FbxFactory->ImportUI->StaticMeshImportData->bConvertSceneUnit = true;

	FbxFactory->ImportUI->MeshTypeToImport = FBXIT_StaticMesh;

	 UAutomatedAssetImportData* FbxImportData = NewObject<UAutomatedAssetImportData>(UAutomatedAssetImportData::StaticClass());
	 FbxImportData->FactoryName = TEXT("FbxFactory");
	 FbxImportData->Factory = FbxFactory;
	 FbxImportData->Filenames = FileNames;
	 FbxImportData->DestinationPath = sDestinationGamePath;

	 FbxImportData->bReplaceExisting = false;

	 TArray<UObject*> ImportedAssets;

	// 1. Snapshot existing assets
	FAssetRegistryModule& arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> beforeAssets;
	arm.Get().GetAssetsByPath(*sDestinationGamePath, beforeAssets, true);

	TSet<FName> beforeNames;
	for (const FAssetData& a : beforeAssets) {
		beforeNames.Add(a.ObjectPath);
	}

	// 2. Import the FBX
	ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(FbxImportData);
	// ERROR CHECK
	if (ImportedAssets.Num() == 0)
	{
		UE_LOG(LogDazToUnreal, Warning, TEXT("DazToUnreal: ERROR: automated FBX import failed, attempting interactive import..."));
		UAssetImportTask* AssetImportTask = NewObject<UAssetImportTask>();
		AssetImportTask->Filename = FbxImportData->Filenames[0];
		AssetImportTask->DestinationPath = FbxImportData->DestinationPath;
		AssetImportTask->Options = FbxFactory->ImportUI;
		AssetImportTask->Factory = FbxFactory;
		AssetImportTask->bAutomated = false;
		TArray< UAssetImportTask* > ImportTasks;
		ImportTasks.Add(AssetImportTask);
		AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
		for (FString ImportedPath : AssetImportTask->ImportedObjectPaths)
		{
				FSoftObjectPath SoftObjectPath(ImportedPath);
				ImportedAssets.Add(SoftObjectPath.TryLoad());
		}
	}

	 FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	 ContentBrowserModule.Get().SyncBrowserToAssets(ImportedAssets);

	 for (UObject* ImportedAsset : ImportedAssets)
	 {
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(ImportedAsset))
		{
			UE_LOG(LogDazToUnreal, Log, TEXT("DazToUnreal: Imported Static Mesh: %s"), *StaticMesh->GetName());
		}
	 }


	// 3. Snapshot again
	TArray<FAssetData> afterAssets;
	arm.Get().GetAssetsByPath(*sDestinationGamePath, afterAssets, true);

	// 4. Identify new assets
	TArray<FAssetData> newMeshAssets;
	TArray<FAssetData> newMaterialAssets;
	TArray<FAssetData> newTextureAssets;

	for (const FAssetData& a : afterAssets)
	{
		if (!beforeNames.Contains(a.ObjectPath))
		{
			if (a.AssetClass == UStaticMesh::StaticClass()->GetFName())
				newMeshAssets.Add(a);
			else if (a.AssetClass == UMaterial::StaticClass()->GetFName() ||
					 a.AssetClass == UMaterialInstanceConstant::StaticClass()->GetFName())
				newMaterialAssets.Add(a);
			else if (a.AssetClass == UTexture::StaticClass()->GetFName() ||
					 a.AssetClass == UTexture2D::StaticClass()->GetFName())
				newTextureAssets.Add(a);
		}
	}

	// 5. move new assets
	for (const FAssetData& a : newTextureAssets)
	{
		FString sPackagePath = a.PackagePath.ToString();   // e.g. /Game/MyFolder
		FString sAssetName = a.AssetName.ToString();     // e.g. MyMaterial
		FString sOldAssetPath = sPackagePath + TEXT("/") + sAssetName;  // e.g. /Game/MyFolder/MyMaterial

		if (FDazToUnrealUtils::MoveSingleAsset(sOldAssetPath, sTexturePath)) {
			UE_LOG(LogDazToUnreal, Log, TEXT("Moved %s -> %s"), *sOldAssetPath, *sTexturePath);
		}
		else {
			UE_LOG(LogDazToUnreal, Error, TEXT("Failed to move %s -> %s"), *sOldAssetPath, *sTexturePath);
		}
	}

	for (const FAssetData& a : newMaterialAssets)
	{
		FString sPackagePath = a.PackagePath.ToString();   // e.g. /Game/MyFolder
		FString sAssetName = a.AssetName.ToString();     // e.g. MyMaterial
		FString sOldAssetPath = sPackagePath + TEXT("/") + sAssetName;  // e.g. /Game/MyFolder/MyMaterial
		FString sNewAssetPath = sMaterialPath / sAssetName;

		if (FDazToUnrealUtils::MoveSingleAsset(sOldAssetPath, sMaterialPath)) {
			UE_LOG(LogDazToUnreal, Log, TEXT("Moved %s -> %s"), *sOldAssetPath, *sMaterialPath);

			// Load material after move
			UMaterial* pMaterial = Cast<UMaterial>(a.GetAsset());
			if (!pMaterial)
			{
				pMaterial = LoadObject<UMaterial>(nullptr, *sNewAssetPath);
			}
			if (!pMaterial)
			{
				UE_LOG(LogDazToUnreal, Error, TEXT("Failed to load material asset: %s"), *sNewAssetPath);
				continue;
			}

			FDazToUnrealUtils::ModifyMaterial_InvertOpacity(pMaterial, /* bSaveChanges */ false);
			FDazToUnrealUtils::ModifyMaterial_TranslucentToMasked(pMaterial, /* bSaveChanges */ false);
			// save changes only when done modifying
			pMaterial->Modify();
			pMaterial->PostEditChange();
			pMaterial->MarkPackageDirty();

		}
		else {
			UE_LOG(LogDazToUnreal, Error, TEXT("Failed to move %s -> %s"), *sOldAssetPath, *sMaterialPath);
		}
	}

	for (const FAssetData& a : newMeshAssets)
	{
		FString sPackagePath = a.PackagePath.ToString();   // e.g. /Game/MyFolder
		FString sAssetName = a.AssetName.ToString();     // e.g. MyMaterial
		FString sOldAssetPath = sPackagePath + TEXT("/") + sAssetName;  // e.g. /Game/MyFolder/MyMaterial
		FString sNewAssetPath = sMeshPath / sAssetName;

		if (FDazToUnrealUtils::MoveSingleAsset(sOldAssetPath, sMeshPath)) {
			UE_LOG(LogDazToUnreal, Log, TEXT("Moved %s -> %s"), *sOldAssetPath, *sMeshPath);

			// Load mesh
			UStaticMesh* pMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *sNewAssetPath));
			if (!pMesh)
			{
				UE_LOG(LogDazToUnreal, Error, TEXT("Failed to load static mesh %s"), *sNewAssetPath);
				continue;
			}

			// Compute bottom of mesh
			const FBoxSphereBounds MeshBounds = pMesh->GetBounds();
			double fBottomZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
			if (std::abs(fBottomZ) < DBL_EPSILON*2) fBottomZ = 0.0;

			FVector vSpawnLoc(0.0f, 0.0f, -fBottomZ); // move bottom to ground
			FRotator vSpawnRot(0.0f, 90.0f, 0.0f);

			FDazToUnrealUtils::PlaceAssetInLevel(pMesh, sAssetName, vSpawnLoc, vSpawnRot);

		}
		else {
			UE_LOG(LogDazToUnreal, Error, TEXT("Failed to move %s -> %s"), *sOldAssetPath, *sMeshPath);
		}
	}

	if (FDazToUnrealUtils::BakeLightingForCurrentMap()) {
		UE_LOG(LogDazToUnreal, Log, TEXT("Lighting bake started successfully."));
	} else {
		UE_LOG(LogDazToUnreal, Error, TEXT("Lighting bake failed."));
	}

	FDazToUnrealUtils::SaveCurrentLevel();

	 if (ImportedAssets.Num() > 0)
	 {
		  return true;
	 }

	 return false;
}



#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDazToUnrealModule, DazToUnreal)
