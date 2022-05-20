#include "dzplugin.h"
#include "dzapp.h"

#include "version.h"
#include "DzUnrealAction.h"
#include "DzUnrealDialog.h"

#include "dzbridge.h"

CPP_PLUGIN_DEFINITION("Daz To Unreal Bridge");

DZ_PLUGIN_AUTHOR("Daz 3D, Inc");

DZ_PLUGIN_VERSION(PLUGIN_MAJOR, PLUGIN_MINOR, PLUGIN_REV, PLUGIN_BUILD);

DZ_PLUGIN_DESCRIPTION(QString(
"<b>Pre-Release DazToUnreal Bridge v%1.%2.%3.%4 </b><br>\
Bridge Collaboration Project<br><br>\
<a href = \"https://github.com/danielbui78-bridge-collab/DazToRuntime/tree/unreal-main\">Github</a><br><br>"
).arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(PLUGIN_REV).arg(PLUGIN_BUILD));

DZ_PLUGIN_CLASS_GUID(DzUnrealAction, 99F42CAE-CD02-49BC-A7CE-C0CF4EDD7609);
NEW_PLUGIN_CUSTOM_CLASS_GUID(DzUnrealDialog, b7c0b573-bd61-452c-92c1-9560459b4e89);

#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzUnrealAction.h"
#include "UnitTest_DzUnrealDialog.h"

DZ_PLUGIN_CLASS_GUID(UnitTest_DzUnrealAction, a56200a2-165d-4256-bf58-76f14877873b);
DZ_PLUGIN_CLASS_GUID(UnitTest_DzUnrealDialog, 20040432-2662-488c-9b23-e50c09d6dc67);

#endif