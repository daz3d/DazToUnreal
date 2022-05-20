# Daz To Unreal - Daz Studio Plugin
This is the Daz Studio side plugin for the Daz To Unreal tool.
## Building the Plugin for Windows
To buid this plugin you need the Daz Studio SDK https://www.daz3d.com/daz-studio-4-5-sdk

1) This project requires cmake to build. You can download cmake at https://cmake.org/download/ or you can use Visual Studio 2017+ or other IDEs that support cmake.
2) If using Cmake-GUI instructions:
    a) Click "Browse Source" and browse to DazToRuntime folder location.
    b) Click "Browse Build" and select where you would like your output build location to be.
    c) Click "Configure" and choose what generation you would like. Then click "Finish". This should start the generation and eventually fail.
    d) Select "DAZ_SDK_DIR" and browse to the location that you install the Daz Studio 4.5 SDK. IE: "C:\Daz 3D\Applications\Data\DAZ 3D\My DAZ 3D Library\DAZStudio4.5+ SDK"
    e) Optional: Select "DAZ_STUDIO_EXE_DIR" to be the location that you install Daz Studio. IE: "C:\Daz 3D\Applications\64-bit\DAZ 3D\DAZStudio4". This will copy the .DLL output into the Daz Studio\plugins folder for you when you build.
    e) Select "Generate". You should now have your project setup for building. 
3) If using VS2019 instructions:
    a) Open VS2019 and click on File->Open->CMake and browse to DazToRuntime folder location.
    b) Right click CMakeList.txt and select "CMake settings for DazRuntimePlugins". 
    c) Select "DAZ_SDK_DIR" and browse to the location that you install the Daz Studio 4.5 SDK. IE: "C:\Daz 3D\Applications\Data\DAZ 3D\My DAZ 3D Library\DAZStudio4.5+ SDK"
    d) Optional: Select "DAZ_STUDIO_EXE_DIR" to be the location that you install Daz Studio. IE: "C:\Daz 3D\Applications\64-bit\DAZ 3D\DAZStudio4". This will copy the .DLL output into the Daz Studio\plugins folder for you when you build.
    e) In VS2019 Change build output from "Current Document" up at the top to be "dzunrealbridge.dll".