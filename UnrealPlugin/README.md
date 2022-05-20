# Daz To Unreal - Unreal Engine Plugin
This is the Unreal Engine side plugin for the Daz To Unreal tool.
## Building the Plugin for Windows
To build the plugin with the Unreal Engine source you should first have the Unreal Engine building on your system.  You can get it at https://github.com/EpicGames
1) Copy the DazToUnreal folder found next to this README to either
   a) the Engine/Plugins folder of your Engine project
   b) the Games/Plugins folder of your Game project (you might have to make this folder)
2) Run GenerateProjectFiles.bat to update your Visual Studio Solution.
3) Now when you build the project, the plugin should build with it.
