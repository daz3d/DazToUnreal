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
	 virtual bool postProcessFbx(QString fbxFilePath) override;

	 enum ELodMethod {
		 Undefined = -1,
		 PreGenerated = 0,
		 Decimator = 1,
		 Unreal_Builtin = 2,
	 } override;
	 virtual int getELodMethodMin() override { return 0; } 
	 virtual int getELodMethodMax() override { return 2; } 
	 // ELodMethod m_eLodMethod = ELodMethod::Unreal_Builtin; // WARNING: Do NOT Use this line, only here to prevent people from using it. This will hide the base class member instead of over-riding it.
	 virtual DzBridgeAction::ELodMethod getLodMethod() const override { return (DzBridgeAction::ELodMethod) m_eLodMethod; }

#ifdef UNITTEST_DZBRIDGE
	 friend class UnitTest_DzUnrealAction;
#endif

};
