#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qcombobox.h>
#include <QtGui/qcheckbox.h>
#include <QtCore/qsettings.h>
#include "DzBridgeDialog.h"

class QPushButton;
class QLineEdit;
class QComboBox;
class QGroupBox;

#include "dzbridge.h"

class DzBridgeAction;

class DzUnrealDialog : public DZ_BRIDGE_NAMESPACE::DzBridgeDialog {
	Q_OBJECT
	Q_PROPERTY(QWidget* wIntermediateFolderEdit READ getIntermediateFolderEdit)
	Q_PROPERTY(QWidget* wPortEdit READ getPortEdit)
public:
	Q_INVOKABLE QLineEdit* getIntermediateFolderEdit() { return intermediateFolderEdit; }
	Q_INVOKABLE QLineEdit* getPortEdit() { return portEdit; }
	Q_INVOKABLE QLineEdit* getMLDeformerPoseCountEdit() { return mlDeformerPoseCountEdit; }

	/** Constructor **/
	 DzUnrealDialog(QWidget *parent=nullptr);

	/** Destructor **/
	virtual ~DzUnrealDialog() {}

	// MLDeformer getters
	bool getMLDeformerIncludeFingers() {
		return mlDeformerIncludeFingersCheckBox ? mlDeformerIncludeFingersCheckBox->isChecked() : false;
	}

	bool getMLDeformerIncludeToes() {
		return mlDeformerIncludeToesCheckBox ? mlDeformerIncludeToesCheckBox->isChecked() : false;
	}

	bool getMLDeformerIncludeFace() {
		return mlDeformerIncludeFaceCheckBox ? mlDeformerIncludeFaceCheckBox->isChecked() : false;
	}

	// SkeletalMesh getters
	bool getUniqueSkeletonPerCharacter() { 
		return skeletalMeshUniqueSkeletonPerCharacterCheckBox ? skeletalMeshUniqueSkeletonPerCharacterCheckBox->isChecked() : false;
	}

	bool getFixTwistBones() {
		return skeletalMeshFixTwistBonesCheckBox ? skeletalMeshFixTwistBonesCheckBox->isChecked() : false;
	}

	bool getFaceCharacterRight() {
		return skeletalMeshFaceCharacterRightCheckBox ? skeletalMeshFaceCharacterRightCheckBox->isChecked() : false;
	}

	// Settings
	Q_INVOKABLE void resetToDefaults() override;
	Q_INVOKABLE void saveSettings() override;

protected slots:
	void HandleSelectIntermediateFolderButton();
	void HandlePortChanged(const QString& port);
	void HandleTargetPluginInstallerButton() override;
	void HandleOpenIntermediateFolderButton(QString sFolderPath = "") override;
	void HandleAssetTypeComboChange(int state) override;
	void HandlePdfButton() override;
	void HandleYoutubeButton() override;
	void HandleSupportButton() override;

// DB 2024-10-02: overrides new base class behaviors
#ifdef VODSVERSION
	void HandleMorphsCheckBoxChange(int state) override;
	void HandleSubdivisionCheckBoxChange(int state) override;
	void HandleFBXVersionChange(const QString& fbxVersion) override;
	void HandleShowFbxDialogCheckBoxChange(int state) override;
	void HandleExportMaterialPropertyCSVCheckBoxChange(int state) override;
	void HandleConvertBumpToNormalCheckBoxChange(int state) override;
	void HandleEnableLodCheckBoxChange(int state) override;
#endif VODSVERSION


protected:
	virtual void whatsThis() override;

	Q_INVOKABLE bool loadSavedSettings() override;

	QLineEdit* portEdit = nullptr;
	QLineEdit* intermediateFolderEdit = nullptr;
	QPushButton* intermediateFolderButton = nullptr;

	// MLDeformer settings
	QGroupBox* mlDeformerSettingsGroupBox = nullptr;
	QLineEdit* mlDeformerPoseCountEdit = nullptr;
	QCheckBox* mlDeformerIncludeFingersCheckBox = nullptr;
	QCheckBox* mlDeformerIncludeToesCheckBox = nullptr;
	QCheckBox* mlDeformerIncludeFaceCheckBox = nullptr;

	// SkeletalMesh settings
	QGroupBox* skeletalMeshSettingsGroupBox = nullptr;
	QCheckBox* skeletalMeshUniqueSkeletonPerCharacterCheckBox = nullptr;

	// Common settings
	QGroupBox* commonSettingsGroupBox = nullptr;
	QCheckBox* skeletalMeshFixTwistBonesCheckBox = nullptr;
	QCheckBox* skeletalMeshFaceCharacterRightCheckBox = nullptr;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzUnrealDialog;
#endif

};
