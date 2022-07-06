#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qcombobox.h>
#include <QtCore/qsettings.h>
#include "DzBridgeDialog.h"

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QGroupBox;

#include "dzbridge.h"

class DzUnrealDialog : public DZ_BRIDGE_NAMESPACE::DzBridgeDialog {
	Q_OBJECT
	Q_PROPERTY(QWidget* wIntermediateFolderEdit READ getIntermediateFolderEdit)
	Q_PROPERTY(QWidget* wPortEdit READ getPortEdit)
public:
	Q_INVOKABLE QLineEdit* getIntermediateFolderEdit() { return intermediateFolderEdit; }
	Q_INVOKABLE QLineEdit* getPortEdit() { return portEdit; }

	/** Constructor **/
	 DzUnrealDialog(QWidget *parent=nullptr);

	/** Destructor **/
	virtual ~DzUnrealDialog() {}

	Q_INVOKABLE void resetToDefaults();

protected slots:
	void HandleSelectIntermediateFolderButton();
	void HandlePortChanged(const QString& port);
	void HandleTargetPluginInstallerButton();
	void HandleAssetTypeComboChange(int state);
	virtual void HandleOpenIntermediateFolderButton(QString sFolderPath = "");

protected:
	Q_INVOKABLE bool loadSavedSettings();

	QLineEdit* portEdit;
	QLineEdit* intermediateFolderEdit;
	QPushButton* intermediateFolderButton;

#ifdef UNITTEST_DZBRIDGE
	friend class UnitTest_DzUnrealDialog;
#endif

};
