#pragma once
#include <dzaction.h>
#include <dznode.h>
#include <dzgeometry.h>
#include <dzfigure.h>
#include <dzjsonwriter.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <QUuid.h>
#include <DzBridgeAction.h>

class DzUnrealDialog;
class UnitTest_DzUnrealAction;

#include "dzbridge.h"

class DzUnrealAction : public DZ_BRIDGE_NAMESPACE::DzBridgeAction {
	Q_OBJECT
public:
	 DzUnrealAction();

	 Q_INVOKABLE bool setBridgeDialog(DzBasicDialog* arg_dlg);

	 Q_INVOKABLE void resetToDefaults();
	 QString readGuiRootFolder();

protected:
	 int m_nPort;

	 void executeAction();
	 Q_INVOKABLE void writeConfiguration();
	 void setExportOptions(DzFileIOSettings& ExportOptions);

	 virtual void exportNode(DzNode* Node) override;
	 virtual void exportAnimation() override;
	 virtual void exportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale) override;

#ifdef UNITTEST_DZBRIDGE
	 friend class UnitTest_DzUnrealAction;
#endif

};
