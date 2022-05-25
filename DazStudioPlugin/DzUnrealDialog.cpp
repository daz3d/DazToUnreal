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

#include "qmessagebox.h"
#include "zip.h"

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

	QFormLayout* advancedLayout = qobject_cast<QFormLayout*>(advancedWidget->layout());
	if (advancedLayout)
	{
		advancedLayout->addRow("Port", portEdit);
		advancedLayout->addRow("Intermediate Folder", intermediateFolderLayout);
	}

	// Configure Target Plugin Installer
	renameTargetPluginInstaller("Unreal Plugin Installer");
	m_TargetSoftwareVersionCombo->clear();
	m_TargetSoftwareVersionCombo->addItem("Select Unreal Version");
	m_TargetSoftwareVersionCombo->addItem("Unreal Engine 4.25");
	m_TargetSoftwareVersionCombo->addItem("Unreal Engine 4.26");
	m_TargetSoftwareVersionCombo->addItem("Unreal Engine 4.27");
	m_TargetSoftwareVersionCombo->addItem("Unreal Engine 5.0");
	showTargetPluginInstaller(true);

	// Help pop-ups
	intermediateFolderEdit->setWhatsThis("DazToUnreal will collect the assets in a subfolder under this folder.  Unreal will import them from here.");
	intermediateFolderButton->setWhatsThis("DazToUnreal will collect the assets in a subfolder under this folder.  Unreal will import them from here.");
	portEdit->setWhatsThis("The UDP port used to talk to the DazToUnreal Unreal plugin.\nThis needs to match the port set in the Project Settings in Unreal.\nDefault is 32345.");
	m_wTargetPluginInstaller->setWhatsThis("You can install the Unreal Plugin by selecting the desired Unreal Engine version and the selecting either the Unreal Engine folder or an Unreal Project folder.");

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

	return true;
}

void DzUnrealDialog::resetToDefaults()
{
	m_DontSaveSettings = true;
	DzBridgeDialog::resetToDefaults();

	QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnreal";
	intermediateFolderEdit->setText(DefaultPath);

	portEdit->setText("32345");
	m_DontSaveSettings = false;
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

void DzUnrealDialog::HandleTargetPluginInstallerButton()
{
	// Get Software Versio
	DzBridgeDialog::m_sEmbeddedFilesPath = ":/DazBridgeUnreal";
	QString sBaseFile = "/UEpluginbase.zip";
	QString sBinariesFile = "";
	QString softwareVersion = m_TargetSoftwareVersionCombo->currentText();
	if (softwareVersion.contains("4.25"))
	{
		sBinariesFile = "/UE4.25.zip";
	}
	else if (softwareVersion.contains("4.26"))
	{
		sBinariesFile = "/UE4.26.zip";
	}
	else if (softwareVersion.contains("4.27"))
	{
		sBinariesFile = "/UE4.27.zip";
	}
	else if (softwareVersion.contains("5.0"))
	{
		sBinariesFile = "/UE5.0.zip";
	}
	else
	{
		// Warning, not a valid plugins folder path
		QMessageBox::information(0, "DazToUnreal Bridge",
			tr("Please select an Unreal Engine version."));
		return;
	}

	// For the first run, Display help / explanation popup dialog...
	// TODO

	// Get Destination Folder
	QString directoryName = QFileDialog::getExistingDirectory(this,
		tr("Choose the Unreal Engine Folder or an Unreal Project Folder"),
		"/home",
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (directoryName == NULL)
	{
		return;
	}

	// fix path separators
	directoryName = directoryName.replace("\\", "/");
	// load with default values
	QString sRootPath = directoryName;
	QString sPluginsPath = sRootPath + "/plugins";
	// Check for plugins name
	if (QRegExp(".*/plugins$").exactMatch(directoryName.toLower()) == true)
	{
		sPluginsPath = directoryName;
		sRootPath = QFileInfo(sPluginsPath).dir().path();
	}
	else if (QRegExp(".*/engine$").exactMatch(directoryName.toLower()) == true)
	{
		sRootPath = directoryName;
		sPluginsPath = sRootPath + "/plugins";
	}
	else
	{
		// check to see if this is an unreal engine root folder
		QStringList childFolders = QDir(directoryName).entryList(QDir::AllDirs);
		for (QString foldername : childFolders)
		{
			if (foldername.toLower() == "engine")
			{
				sRootPath = directoryName + "/engine";
				sPluginsPath = sRootPath + "/plugins";
				break;
			}
		}
	}

	bool bIsEnginePath = false;
	// Check for Engine in sRootPath
	if (sRootPath.toLower().contains("engine"))
	{
		bIsEnginePath = true;
	}

	bool bIsProjectPath = false;
	// Check for uproject file
	if (bIsEnginePath == false)
	{
		// get files in root folder
		QStringList fileList = QDir(sRootPath).entryList(QDir::Files);
		for (QString filename : fileList)
		{
			if (filename.toLower().contains(".uproject"))
			{
				bIsProjectPath = true;
			}
		}
		// if file has .uproject, then isProjectFolder = true
	}

	if (bIsEnginePath == false && bIsProjectPath == false)
	{
		// Warning, not a valid plugins folder path
		auto userChoice = QMessageBox::warning(0, "DazToUnreal Bridge",
			tr("The selected folder is not a valid plugins folder.  Please select an \
Unreal Engine Plugins folder, ex: \"UE_5.0\\Engine\\Plugins\", or an Unreal Project \
folder to install the Unreal Plugin.\n\nYou can choose to Abort and select a new folder, \
or Ignore this error and install the plugin anyway."),
			QMessageBox::Ignore | QMessageBox::Abort,
			QMessageBox::Abort);
		if (userChoice == QMessageBox::StandardButton::Abort)
			return;
	}

	// create plugins folder if does not exist
	if (QDir(sPluginsPath).exists() == false)
	{
		QDir().mkdir(sPluginsPath);
	}

	////////////////////////////////
	// TODO:
	// 1. Copy zip to Daz temp folder
	// 2. Validate selected Folder to sPluginsPath
	// 3. Validate software version to resource zip file(s)
	// 4. Extract to sPluginsPath
	// 5. Migrate to DazBridge all of the above
	//////////////////////////////////

//	// Copy to temp
//	bool replace = true;
//	QFile srcFileBase(sBaseFile);
//	QString destPathBase = dzApp->getTempPath() + "/UEpluginbase.zip";
//	DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFileBase, &destPathBase, replace);
//	srcFileBase.close();
//
//	QFile srcFileBinaries(sBinariesFile);
//	QString destPathBinaries = dzApp->getTempPath() + "/UEbinaries.zip";
//	DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFileBinaries, &destPathBinaries, replace);
//	srcFileBinaries.close();
//
//	// Extract Files to Destination
//	::zip_extract(destPathBase.toAscii().data(), sPluginsPath.toAscii().data(), nullptr, nullptr);
//	::zip_extract(destPathBinaries.toAscii().data(), sPluginsPath.toAscii().data(), nullptr, nullptr);
//
//	// Check to make sure files were properly unzipped
//	QString sCheckPathBinaries = sPluginsPath + "/DazToUnreal/Binaries";
//	QString sCheckPathContent = sPluginsPath + "/DazToUnreal/Content";
//	if (QDir(sCheckPathBinaries).exists() &&
//		QDir(sCheckPathContent).exists())
//	{
//		bool bInstallSuccessful = false;
//		// Check if files present in Binaries folder
//#if __APPLE__
//		QStringList fileList = QDir(sCheckPathBinaries + "/Mac").entryList(QDir::Files);
//#else
//		QStringList fileList = QDir(sCheckPathBinaries + "/Win64").entryList(QDir::Files);
//#endif
//		for (QString filename : fileList)
//		{
//			if (filename.toLower().contains("daztounreal.dll") ||
//				filename.toLower().contains("daztounreal.dylib"))
//			{
//				bInstallSuccessful = true;
//				break;
//			}
//		}
//
//		if (bInstallSuccessful)
//		{
//			QMessageBox::information(0, "Daz To Unreal",
//				tr("Unreal Plugin successfully installed to: ") + sPluginsPath +
//tr("\n\nIf the Unreal Editor is open, please quit and restart Unreal to continue \
//Bridge Export process."));
//		}
//		else
//		{
//			QMessageBox::warning(0, "Daz To Unreal",
//				tr("Sorry, an unknown error occured. Unable to install \
//Unreal Plugin to: ") + sPluginsPath);
//			return;
//		}
//	}
//	// Remove zip files
//	QFile(destPathBase).remove();
//	QFile(destPathBinaries).remove();

	bool bInstallSuccessful = false;
	bInstallSuccessful = installEmbeddedArchive(sBaseFile, sPluginsPath);
	bInstallSuccessful = installEmbeddedArchive(sBinariesFile, sPluginsPath);

	if (bInstallSuccessful)
	{
		QMessageBox::information(0, "Daz To Unreal",
			tr("Unreal Plugin successfully installed to: ") + sPluginsPath +
tr("\n\nIf the Unreal Editor is open, please quit and restart Unreal to continue \
Bridge Export process."));
	}
	else
	{
		QMessageBox::warning(0, "Daz To Unreal",
			tr("Sorry, an unknown error occured. Unable to install \
Unreal Plugin to: ") + sPluginsPath);
		return;
	}


	return;
}


#include "moc_DzUnrealDialog.cpp"
