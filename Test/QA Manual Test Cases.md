# QA Manual Test Cases: Unreal script support #

## TC1. Load and Export Genesis 8 Basic Female to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis8Female" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis8Female" asset in the Content Browser Pane.
10. Confirm that a "Genesis8Female" subfolder was generated in the Intermediate Folder, with "Genesis8Female.dtu" and "Genesis8Female.fbx" files.
11. Confirm that "ExportTextures" subfolder is present in the "Genesis8Female" folder, with 6 normal map files for: arms, eyes, face, legs, mouth, torso.
12. Confirm normal maps are valid images by opening each one in an image viewer.
13. In Daz Studio, go to the Surfaces pane and click the "Editor" tab.
14. Select "Genesis 8 Female", type "normal" in the search/filter bar text box which is found next to the magnifying glass icon.
15. Confirm that pane shows "(16): Normal Map" property with "Choose Map" text displayed.

## TC2. Load and Export Additional Genesis 8.1 Basic Female to Unreal
1. Continue from previous Daz Studio and Unreal Engine session (test case 1).
2. Deselect "Genesis 8 Female" from Scene.
3. Load Genesis 8.1 Basic Female.
4. Confirm new Scene node is created named "Genesis 8.1 Female".
5. Select File->Send To->Daz To Unreal.
6. Confirm Asset Name is "Genesis81Female" in the Daz To Unreal dialog window.
7. Click "Accept".
8. Confirm UnrealEngine has successfully generated a new "Genesis81Female" asset in the Content Browser Pane.
9. Confirm that a "Genesis81Female" subfolder was generated in the Intermediate Folder, with "Genesis81Female.dtu" and "Genesis81Female.fbx" files.
10. Confirm that "ExportTextures" subfolder is present in the "Genesis8Female" folder, with 5 normal map files for: body, face, head, arms, legs.
11. Confirm normal maps are valid images by opening each one in an image viewer.
12. In Daz Studio, go to the Surfaces pane and click the "Editor" tab.
13. Open the "Genesis 8.1 Female" tree and select "Skin-Lips-Nails", type "normal" in the search/filter bar text box which is found next to the magnifying glass icon.
14. Confirm that pane shows "(10): Normal Map" property with "Choose Map" text displayed.

## TC3. Load and Export Genesis 8.1 Basic Female with Custom Scene Node Label.
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8.1 Basic Female.
6. Select the Genesis 8.1 Basic Female from the Scene Pane.
7. Rename the node to "CustomSceneLabel".
8. Select File->Send To->Daz To Unreal.
9. Confirm Asset Name is "CustomSceneLabel" in the Daz To Unreal dialog window.
10. Click "Accept".
11. Confirm Unreal Engine has successfully generated a new "CustomSceneLabel" asset in the Content Browser Pane.
12. Confirm that a "CustomSceneLabel" subfolder was generated in the Intermediate Folder, with "CustomSceneLabel.dtu" and "CustomSceneLabel.fbx" files.

## TC4. Load and Export Genesis 8.1 Basic Female with Custom Asset Name to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8.1 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis81Female" in the Daz To Unreal dialog window.
8. Change Asset Name to "CustomAssetName".
9. Click "Accept".
10. Confirm Unreal Engine has successfully generated a new "CustomAssetName" asset in the Content Browser Pane.
11. Confirm that a "CustomAssetName" subfolder was generated in the Intermediate Folder, with "CustomAssetName.dtu" and "CustomAssetName.fbx" files.

## TC5. Load and Export Genesis 8.1 Basic Female with Custom Intermediate Folder to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8.1 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Intermediate Folder is "C:\Users\<username>\Documents\DazToUnreal" in the Daz To Unreal dialog window.
8. Change Intermediate Folder to "C:\CustomRoot".
9. Click "Accept".
10. Confirm Unreal Engine has successfully generated a new "Genesis81Female" asset in the Content Browser Pane.
11. Confirm there is a "C:\CustomRoot" folder with "Genesis81Female" subfolder containing "Genesis81Female.dtu" and "Genesis81Female.fbx".

## TC6. Load and Export Genesis 8.1 Basic Female with Enable Morphs to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8.1 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Check "Enable Morphs", then Click "Choose Morphs".
8. Select a Morph such as "Genesis 8.1 Female -> Actor -> Bodybuilder" from the left and middle panes. Then click "Add For Export" so that it appears in the right pane.
9. Click "Accept" for the Morph Selection dialog.
10. Click "Accept" for the Daz To Unreal dialog.
11. Confirm Unreal Engine has successfully generated a new "Genesis81Female" asset in the Content Browser Pane.
12. Double-click the "Genesis81Female" asset to show the asset viewer window.
13. Confirm that the exported morph appears in the "Morph Target Preview" pane on the right side of the asset viewer window.
14. Confirm that moving the slider to 1.0 fully applies the morph.
15. Confirm that moving the slider to 0.0 fully removes the morph.

## TC7. Load and Export Genesis 8.1 Basic Female with Enable Subdivisions to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8.1 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Check "Enable Subdivision", then Click "Choose Subdivisions".
8. Select the drop-down for Genesis 8.1 Female, and change to Subdivision Level 2.
9. Click "Accept" for the Subdivision Levels dialog.
10. Click "Accept" for the Daz To Unreal dialog.
11. Confirm Unreal Engine has successfully generated a new "Genesis81Female" asset in the Content Browser Pane.
12. Double-click the "Genesis81Female" asset to show the asset viewer window.
13. Confirm that the Vertices info printed in the top left corner of the preview window shows 271,418 instead of 19,775.
14. Confirm that a "Genesis81Female" subfolder was generated in the Intermediate Folder, with "Genesis81Female.dtu", "Genesis81Female.fbx", "Genesis81Female_base.fbx" and "Genesis81Female_HD.fbx" files.

## TC8. Load and Export Custom Scene File to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Select File->Open... and load "QA-Test-Scene-01.duf".
6. Select File->Send To->Daz To Unreal.
7. Confirm the Asset Name is "QATestScene01".
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "QATestScene01" asset in the Content Browser Pane.
10. Confirm that a "QATestScene01" subfolder was generated in the Intermediate Folder, with "QATestScene01.dtu" and "QATestScene01.fbx" files.

## TC9. Load and Export Victoria 8.1 to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Victoria 8.1.
6. Select File->Send To->Daz To Unreal.
7. Confirm the Asset Name is "Victoria81".
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Victoria81" asset in the Content Browser Pane.
10. Confirm that a "Victoria81" subfolder was generated in the Intermediate Folder, with "Victoria81.dtu" and "Victoria81.fbx" files.
11. Confirm that "ExportTextures" subfolder is NOT present in the "Victoria81" folder.

## TC10. Load and Export Victoria 8.1 with "Victoria 8.1 Tattoo All - Add" to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Victoria 8.1.
6. Open the Materials section and load "Victoria 8.1 Tattoo All - Add" onto Victoria 8.1.
7. Wait for the L.I.E. textures to be baked and updated in the Viewport.  This can take a few minutes.
8. Select File->Send To->Daz To Unreal.
9. Confirm the Asset Name is "Victoria81".
10. Click "Accept".
11. Confirm Unreal Engine has successfully generated a new "Victoria81" asset in the Content Browser Pane.
12. Double-click the "Victoria81" asset to show the asset viewer window.
13. Confirm that the asset has full body tattoos visible in the asset viewer.
14. Confirm that a "Victoria81" subfolder was generated in the Intermediate Folder, with "Victoria81.dtu" and "Victoria81.fbx" files, and additional "ExportTextures" folder with 8 PNG texture files (d10.png to d17.png).

## TC11. Load and Export Genesis 8 Basic Male to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8 Basic Male.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis8Male" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis8Male" asset in the Content Browser Pane.
10. Confirm that a "Genesis8Male" subfolder was generated in the Intermediate Folder, with "Genesis8Male.dtu" and "Genesis8Male.fbx" files.

## TC12. Load and Export Genesis 8.1 Basic Male to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8.1 Basic Male.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis81Male" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis81Male" asset in the Content Browser Pane.
10. Confirm that a "Genesis81Male" subfolder was generated in the Intermediate Folder, with "Genesis81Male.dtu" and "Genesis81Male.fbx" files.

## TC13. Load and Export Genesis 3 Basic Female to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 3 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis3Female" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis3Female" asset in the Content Browser Pane.
10. Confirm that a "Genesis3Female" subfolder was generated in the Intermediate Folder, with "Genesis3Female.dtu" and "Genesis3Female.fbx" files.

## TC14. Load and Export Genesis 3 Basic Male to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 3 Basic Male.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis3Male" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis3Male" asset in the Content Browser Pane.
10. Confirm that a "Genesis3Male" subfolder was generated in the Intermediate Folder, with "Genesis3Male.dtu" and "Genesis3Male.fbx" files.

## TC15. Load and Export Genesis 2 Basic Female to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 2 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis2Female" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis2Female" asset in the Content Browser Pane.
10. Confirm that a "Genesis2Female" subfolder was generated in the Intermediate Folder, with "Genesis2Female.dtu" and "Genesis2Female.fbx" files.

## TC16. Load and Export Genesis 2 Basic Male to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 2 Basic Male.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis2Male" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis2Male" asset in the Content Browser Pane.
10. Confirm that a "Genesis2Male" subfolder was generated in the Intermediate Folder, with "Genesis2Male.dtu" and "Genesis2Male.fbx" files.

## TC17. Load and Export Moonshine Drive-In Movie Theatre Set to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load “!Pre_DriveIn.duf” Environment Set from the Moonshine’s Drive-In Movie Theatre product package.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is “MSD_Ground” in the Daz To Unreal dialog window.
8. Change Asset Type to “Environment”.
9. Click “Accept”.
10. Confirm Unreal Engine has successfully generated multiple assets in the Content Browser Pane.
11. Confirm Unreal Engine has generated a “MSD_Ground” map level in the main Level Editor window, which has all assets placed in similar position as the Daz Studio Viewport pane.
12. Confirm that a “MSD_Ground” subfolder was generated in the Intermediate Folder, with multiple “DTU” and “FBX” files generated, including “MSD_Ground.dtu”.

## TC18. Load and Export Genesis 8 Basic Female with Pose to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis8Female" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis8Female" asset in the Content Browser Pane.
10. Return to Daz Studio and apply “Basic Pose Standing A.duf” to the selected Genesis 8 Female figure.
11. Select File->Send To->Daz To Unreal.
12. Change Asset Name to “BasicPoseStandingA”.
13. Change Asset Type to “Pose”.
14. Click "Accept".
15. Confirm Unreal Engine now has a new Pose Asset named “BasicPoseStandingA” in the Pose folder.

## TC19. Load and Export Genesis 8 Basic Female with Aniblock Animation to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8 Basic Female.
6. Select File->Send To->Daz To Unreal.
7. Confirm Asset Name is "Genesis8Female" in the Daz To Unreal dialog window.
8. Click "Accept".
9. Confirm Unreal Engine has successfully generated a new "Genesis8Female" asset in the Content Browser Pane.
10. Return to Daz Studio and apply “bow-g8f.ds” to the selected Genesis 8 Female figure.
11. Open the aniMate2 Pane, right-click in an empty part of the pane and select “Bake To Studio Keyframes”.
12. Click “Yes” to continue.
13. Open the Timeline Pane and confirm that it is populated with multiple keyframes.
14. Select File->Send To->Daz To Unreal.
15. Change Asset Name to “BowG8F”.
16. Change Asset Type to “Animation”.
17. Click “Accept”.
18. Confirm Unreal Engine now has a new Animation Asset named “BowG8F” in the Animation folder.

## TC20. Load and Export Genesis 8 Basic Female with hair, clothing and props to Unreal
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unreal Engine.
4. Confirm correct version number of pre-release UnrealEngine bridge plugin.
5. Load Genesis 8 Basic Female.
6. Apply “Toulouse Hair.duf” to Genesis 8 Female.
7. Apply “Shadow Thief !!Outfit.duf” from the “Shadow Thief Outfit for Genesis 8 Female(s)” product.
8. Apply “Dark Fantasy Long Blade Hande Left.duf” for Genesis 8 Female from the “Dark Fantasy Weapons for Genesis 3 and 8 Female” product.
9. Apply “Dark Fantasy Long Blade Hande Right.duf” for Genesis 8 Female from the “Dark Fantasy Weapons for Genesis 3 and 8 Female” product.
10. Select File->Send To->Daz To Unreal.
11. Confirm Asset Name is "Genesis8Female" in the Daz To Unreal dialog window.
12. Click "Accept".
13. Confirm Unreal Engine has successfully generated a new "Genesis8Female" asset in the Content Browser Pane.
14. Double-click the asset in the Content Browser Pane to view the new asset and confirm that hair, clothing and props are transferred correctly to Unreal Engine.

## TC21. Open Daz Bridge Dialog with Empty Scene
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Select File->Send To->Daz Bridges.
4. Confirm either pop-up notification to select figure or prop, or Daz Bridge dialog window.
5. If Daz Bridge dialog appears, confirm Asset Name, Asset Type, Morphs and Subdivision options are disabled.
6. Click “Accept”.
7. Confirm pop-up notification to select figure or prop.

## TC22. Install Target Bridge Software (Unity) via Daz Bridge Plugin (Daz To Unity plugin)
1. Start Daz Studio.
2. Confirm correct version number of pre-release Daz Studio bridge plugin.
3. Start Unity Editor with a new HDRP Project.
4. Load Genesis 8 Basic Female.
5. Select File->Send To->Daz To Unity.
6. Confirm Asset Name is "Genesis8Female" in the Daz To Unity dialog window.
7. Select Unity Assets Folder for the newly created project.
8. Confirm that “Install Unity Files” is automatically enabled with checkmark.
9. Click “Accept”.
10. Switch to Unity Editor and confirm Unity Package Import Window is loaded with DazToUnity installer unity package.
11. Click “Import”.
12. Confirm creation of “Assets/Daz3D” folder in Project Pane of Unity Editor.
13. Confirm correct version number of pre-release Unity bridge plugin.
