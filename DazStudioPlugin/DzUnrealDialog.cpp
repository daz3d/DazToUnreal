#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QToolTip>
#include <QtGui/QWhatsThis>
#include <QtGui/qlineedit.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qfiledialog.h>
#include <QtCore/qsettings.h>
#include <QtGui/qformlayout.h>
#include <QtGui/qcombobox.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qcheckbox.h>
#include <QtGui/qlistwidget.h>
#include <QtGui/qgroupbox.h>

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"

#include "DzBridgeAction.h"
#include "DzUnrealDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"
#include "version.h"

/*****************************
Local definitions
*****************************/
#define DAZ_TO_UNREAL_PLUGIN_NAME "DazToUnreal"


DzUnrealDialog::DzUnrealDialog(QWidget *parent) :
	DzBridgeDialog(parent, DAZ_TO_UNREAL_PLUGIN_NAME)
{
	portEdit = nullptr;
	intermediateFolderEdit = nullptr;
	intermediateFolderButton = nullptr;

	// Set the dialog title
	int revision = PLUGIN_REV % 1000;
#ifdef _DEBUG
	setWindowTitle(tr("Daz To Unreal v%1.%2 Build %3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(revision).arg(PLUGIN_BUILD));
#else
	setWindowTitle(tr("Daz To Unreal v%1.%2").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR));
#endif
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	settings = new QSettings("Daz 3D", "DazToUnreal");

	// Intermediate Folder
	QHBoxLayout* intermediateFolderLayout = new QHBoxLayout();
	intermediateFolderEdit = new QLineEdit(this);
	intermediateFolderButton = new QPushButton("...", this);
	intermediateFolderLayout->addWidget(intermediateFolderEdit);
	intermediateFolderLayout->addWidget(intermediateFolderButton);
	connect(intermediateFolderButton, SIGNAL(released()), this, SLOT(HandleSelectIntermediateFolderButton()));

	// Ports
	portEdit = new QLineEdit("32345");
	connect(portEdit, SIGNAL(textChanged(const QString &)), this, SLOT(HandlePortChanged(const QString &)));

	// Export Material Property CSV option
	exportMaterialPropertyCSVCheckBox = new QCheckBox("", this);
	connect(exportMaterialPropertyCSVCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleExportMaterialPropertyCSVCheckBoxChange(int)));

	QFormLayout* advancedLayout = qobject_cast<QFormLayout*>(advancedWidget->layout());
	if (advancedLayout)
	{
		advancedLayout->addRow("Port", portEdit);
		advancedLayout->addRow("Intermediate Folder", intermediateFolderLayout);
		advancedLayout->addRow("Export Material CSV", exportMaterialPropertyCSVCheckBox);
	}

	// Help pop-ups
	intermediateFolderEdit->setWhatsThis("DazToUnreal will collect the assets in a subfolder under this folder.  Unreal will import them from here.");
	intermediateFolderButton->setWhatsThis("DazToUnreal will collect the assets in a subfolder under this folder.  Unreal will import them from here.");
	portEdit->setWhatsThis("The UDP port used to talk to the DazToUnreal Unreal plugin.\nThis needs to match the port set in the Project Settings in Unreal.\nDefault is 32345.");
	exportMaterialPropertyCSVCheckBox->setWhatsThis("Checking this will write out a CSV of all the material properties.  Useful for reference when changing materials.");

	// Set Defaults
	resetToDefaults();

	// Load Settings
	loadSavedSettings();

}

bool DzUnrealDialog::loadSavedSettings()
{
	DzBridgeDialog::loadSavedSettings();

	if (!settings->value("IntermediatePath").isNull())
	{
		intermediateFolderEdit->setText(settings->value("IntermediatePath").toString());
	}
	else
	{
		QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnreal";
		intermediateFolderEdit->setText(DefaultPath);
	}
	if (!settings->value("Port").isNull())
	{
		portEdit->setText(settings->value("Port").toString());
	}
	if (!settings->value("ExportMaterialPropertyCSV").isNull())
	{
		exportMaterialPropertyCSVCheckBox->setChecked(settings->value("ExportMaterialPropertyCSV").toBool());
	}

	return true;
}

void DzUnrealDialog::resetToDefaults()
{
	DzBridgeDialog::resetToDefaults();

	QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnreal";
	intermediateFolderEdit->setText(DefaultPath);

	portEdit->setText("32345");
	exportMaterialPropertyCSVCheckBox->setChecked(false);

}

void DzUnrealDialog::HandleSelectIntermediateFolderButton()
{
	QString directoryName = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
		"/home",
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (directoryName != NULL)
	{
		intermediateFolderEdit->setText(directoryName);
		if (settings != nullptr)
		{
			settings->setValue("IntermediatePath", directoryName);
		}
	}
}

void DzUnrealDialog::HandlePortChanged(const QString& port)
{
	if (settings == nullptr) return;
	settings->setValue("Port", port);
}

#include "moc_DzUnrealDialog.cpp"
