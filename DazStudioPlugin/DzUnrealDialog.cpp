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

	// Welcome String for Setup/Welcome Mode
	QString sSetupModeString = tr("<h4>\
If this is your first time using the Daz To Unreal Bridge, please be sure to read or watch \
the tutorials or videos below to install and enable the Unreal Engine Plugin for the bridge:</h4>\
<ul>\
<li><a href=\"https://github.com/daz3d/DazToUnreal/releases\">Download latest Build dependencies, updates and bugfixes (Github)</a></li>\
<li><a href=\"https://github.com/daz3d/DazToUnreal#2-how-to-install\">How To Install and Configure the Bridge (Github)</a></li>\
<li><a href=\"https://www.daz3d.com/unreal-bridge#faq\">Daz To Unreal FAQ (Daz 3D)</a></li>\
<li><a href=\"https://youtu.be/njPE-5EXrIc\">Installing the Daz to Unreal Bridge (Youtube)</a></li>\
<li><a href=\"https://www.daz3d.com/forums/discussion/574891/official-daztounreal-bridge-what-s-new-and-how-to-use-it/p1\">What's New and How To Use It (Daz 3D Forums)</a></li>\
</ul>\
<b>NOTE:</b> In order to Package a Project, you will need to download and install the PackageProject-Dependencies.  \
Please see <a href=\"https://github.com/daz3d/DazToUnreal#package-project-dependencies\">Github instructions</a> for instructions to do this.<br><br>\
Once the Unreal Engine plugin is enabled, please add a Character or Prop to the Scene to transfer assets using the Daz To Unreal Bridge.<br><br>\
To find out more about Daz Bridges, go to <a href=\"https://www.daz3d.com/daz-bridges\">https://www.daz3d.com/daz-bridges</a><br>\
");
	m_WelcomeLabel->setText(sSetupModeString);
	QString sBridgeVersionString = tr("Daz To Unreal Bridge %1.%2 revision %3.%4").arg(PLUGIN_MAJOR).arg(PLUGIN_MINOR).arg(revision).arg(PLUGIN_BUILD);
	setBridgeVersionStringAndLabel(sBridgeVersionString);

	layout()->setSizeConstraint(QLayout::SetFixedSize);

	settings = new QSettings("Daz 3D", "DazToUnreal");

	// add new asset type to assetTypeCombo widget ("MLDeformer")
	assetTypeCombo->addItem("MLDeformer");

	// Morph Settings
	mlDeformerSettingsGroupBox = new QGroupBox("MLDeformer Settings", this);
	QFormLayout* mlDeformerSettingsLayout = new QFormLayout();
	mlDeformerSettingsGroupBox->setLayout(mlDeformerSettingsLayout);
	mlDeformerPoseCountEdit = new QLineEdit("500", mlDeformerSettingsGroupBox);
	mlDeformerPoseCountEdit->setValidator(new QIntValidator());
	mlDeformerSettingsLayout->addRow("Pose Count", mlDeformerPoseCountEdit);
	mlDeformerSettingsGroupBox->setVisible(false);

	// Add ML Deformer settings to the mainLayout as a new row without header
	mainLayout->addRow(mlDeformerSettingsGroupBox);

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

	// Add Port and Intermediate Folder to Advanced Settings container as a new row with specific headers
	QFormLayout* advancedLayout = qobject_cast<QFormLayout*>(advancedWidget->layout());
	if (advancedLayout)
	{
		advancedLayout->addRow("Port", portEdit);
		advancedLayout->addRow("Intermediate Folder", intermediateFolderLayout);
		// reposition the Open Intermediate Folder button so it aligns with the center section
		advancedLayout->removeWidget(m_OpenIntermediateFolderButton);
		advancedLayout->addRow("", m_OpenIntermediateFolderButton);
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

	// Animation settings
	if (!settings->value("MLDeformerPoseCount").isNull())
	{
		mlDeformerPoseCountEdit->setText(settings->value("MLDeformerPoseCount").toString());
	}

	return true;
}

void DzUnrealDialog::saveSettings()
{
	if (settings == nullptr || m_bDontSaveSettings) return;

	DzBridgeDialog::saveSettings();

	// Animation settings
	settings->setValue("MLDeformerPoseCount", mlDeformerPoseCountEdit->text().toInt());
}

void DzUnrealDialog::resetToDefaults()
{
	m_bDontSaveSettings = true;
	DzBridgeDialog::resetToDefaults();

	QString DefaultPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnreal";
	intermediateFolderEdit->setText(DefaultPath);

	portEdit->setText("32345");
	exportMaterialPropertyCSVCheckBox->setChecked(false);

	m_bDontSaveSettings = false;
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
		// User hit cancel: return without addition popups
		return;
	}

	// fix path separators
	directoryName = directoryName.replace("\\", "/");
	// load with default values
	QString sRootPath = directoryName;
	QString sPluginsPath = sRootPath + "/plugins";
	// Check for plugins name
	if (QRegExp(".*/engine$").exactMatch(directoryName.toLower()) == true)
	{
		sRootPath = directoryName;
		sPluginsPath = sRootPath + "/plugins/marketplace";
	}
	else if (QRegExp(".*/engine/plugins$").exactMatch(directoryName.toLower()) == true)
	{
		sRootPath = directoryName;
		sPluginsPath = sRootPath + "/marketplace";
	}
	else if (QRegExp(".*/engine/plugins/marketplace$").exactMatch(directoryName.toLower()) == true)
	{
		sPluginsPath = directoryName;
		sRootPath = QFileInfo(sPluginsPath).dir().path();
	}
	else if (QRegExp(".*/plugins$").exactMatch(directoryName.toLower()) == true)
	{
		sPluginsPath = directoryName;
		sRootPath = QFileInfo(sPluginsPath).dir().path();
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
				sPluginsPath = sRootPath + "/plugins/marketplace";
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

	bool bInstallSuccessful = false;
	bInstallSuccessful = installEmbeddedArchive(sBaseFile, sPluginsPath);
	bInstallSuccessful = installEmbeddedArchive(sBinariesFile, sPluginsPath);

	// UnrealPlugin-specific validation for Installation Success
	// Check for Binaries and Content folders
	QString sCheckPathBinaries = sPluginsPath + "/DazToUnreal/Binaries";
	QString sCheckPathContent = sPluginsPath + "/DazToUnreal/Content";
	if (QDir(sCheckPathBinaries).exists() &&
		QDir(sCheckPathContent).exists())
	{
		bInstallSuccessful = false;
		// Check for DLL/DYLIB present in Binaries folder
#if __APPLE__
		QStringList fileList = QDir(sCheckPathBinaries + "/Mac").entryList(QDir::Files);
#else
		QStringList fileList = QDir(sCheckPathBinaries + "/Win64").entryList(QDir::Files);
#endif
		for (QString filename : fileList)
		{
			if (filename.toLower().contains("daztounreal.dll") ||
				filename.toLower().contains("daztounreal.dylib"))
			{
				bInstallSuccessful = true;
				break;
			}
		}
	}

	if (bInstallSuccessful)
	{
		QMessageBox msgBox;
		msgBox.setTextFormat(Qt::RichText);
		msgBox.setWindowTitle("Unreal Engine Bridge Plugin Installer");
		msgBox.setText(tr("<h4>Unreal Plugin was successfully installed to:</h4>") +
			"<h4><center>" + sPluginsPath + "</center></h4>" +
			tr("<h4>If the Unreal Editor is open, please quit and restart Unreal to continue \
the Bridge Export process.</h4>") +
			tr("<h4><b>NOTE:</b> In order to Package a Project, you will also need \
to download and install the PackageProject-Dependencies file from the \
<a href=\"https://github.com/daz3d/DazToUnreal/releases\">DazToUnreal Github Releases</a> page.</h4>"));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	}
	else
	{
		QMessageBox msgBox;
		msgBox.setTextFormat(Qt::RichText);
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle("Unreal Engine Bridge Plugin Installer");
		msgBox.setText(tr("<h4>Sorry, an unknown error occured. Unable to install \
Unreal Plugin to:</h4>") +
			"<h4><center>" + sPluginsPath + "</center></h4>" +
			tr("<h4>If you were trying to install into the Unreal \
Engine plugins folder, please make sure you have write permissions to that folder.</h4>") +
			tr("<h4>For further help, you can go to the \
<a href=\"https://www.daz3d.com/forums/categories/unreal-discussion\">Daz Forums</a> or the \
<a href=\"https://github.com/daz3d/DazToUnreal/issues\">Github Issues</a> page.")
		);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();

		return;
	}

	return;
}

void DzUnrealDialog::HandleOpenIntermediateFolderButton(QString sFolderPath)
{
	QString sIntermediateFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnreal";
	if (intermediateFolderEdit != nullptr)
	{
		sIntermediateFolder = intermediateFolderEdit->text();
	}
	sIntermediateFolder = sIntermediateFolder.replace("\\", "/");
	DzBridgeDialog::HandleOpenIntermediateFolderButton(sIntermediateFolder);
}

void DzUnrealDialog::HandleAssetTypeComboChange(const QString& assetType)
{
	mlDeformerSettingsGroupBox->setVisible(assetType == "MLDeformer");
	DzBridgeDialog::HandleAssetTypeComboChange(assetType);
}


#include "moc_DzUnrealDialog.cpp"
