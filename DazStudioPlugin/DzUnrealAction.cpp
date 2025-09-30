#include <QtGui/qcheckbox.h>
#include <QtGui/QMessageBox>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qabstractsocket.h>
#include <QUuid.h>

#include <dzapp.h>
#include <dzscene.h>
#include <dzmainwindow.h>
#include <dzshape.h>
#include <dzproperty.h>
#include <dzobject.h>
#include <dzpresentation.h>
#include <dznumericproperty.h>
#include <dzfloatproperty.h>
#include <dzimageproperty.h>
#include <dzcolorproperty.h>
#include <dpcimages.h>
#include <dzfigure.h>
#include <dzfacetmesh.h>
#include <dzbone.h>
#include <dzcontentmgr.h>
//#include <dznodeinstance.h>
#include "idzsceneasset.h"
#include "dzuri.h"
#include "dzprogress.h"
#include "dzscript.h"

#include "DzUnrealAction.h"
#include "DzUnrealDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#include "MLDeformer.h"
#include "FbxTools.h"
#include "OpenFBXInterface.h"

DzUnrealAction::DzUnrealAction() :
	 DzBridgeAction(tr("Send to &Unreal..."), tr("Send the selected node to Unreal."))
{
	 this->setObjectName("DzBridge_DazToUnreal_Action");

	 m_nPort = 0;
     m_nNonInteractiveMode = 0;
	 m_sAssetType = QString("SkeletalMesh");

	 //Setup Icon
#ifndef VODS_LOCAL_BUILD // getEmbeddedImage specifically won't link for me locally
	 QString iconName = "icon";
	 QPixmap basePixmap = QPixmap::fromImage(getEmbeddedImage(iconName.toLatin1()));
	 QIcon icon;
	 icon.addPixmap(basePixmap, QIcon::Normal, QIcon::Off);
	 QAction::setIcon(icon);
#endif

	 m_bGenerateNormalMaps = true;
	 m_bPostProcessFbx = true;
	 m_bRemoveDuplicateGeografts = true;

	 // DB, 2022-Aug-29: Force LOD method to Unreal Built-in
	 m_eLodMethod = (DzBridgeAction::ELodMethod) ELodMethod::Unreal_Builtin;

	m_sEmbeddedFolderPath = ":/DazBridgeUnreal";
	m_bDetachGeometry = true;
}

void DzUnrealAction::executeAction()
{
	m_nExecuteActionResult = DZ_OPERATION_FAILED_ERROR;
	DzNode* pPreviousSelection = m_pSelectedNode;
	DZ_BRIDGE_NAMESPACE::EAssetType ePreviousAssetType = m_eSelectedNodeAssetType;
	
	// Check if the main window has been created yet.
	 // If it hasn't, alert the user and exit early.
	 DzMainWindow* mw = dzApp->getInterface();
	 if (!mw)
	 {
         if (m_nNonInteractiveMode == 0)
		 {
             QMessageBox::warning(0, tr("Error"),
                 tr("The main window has not been created yet."), QMessageBox::Ok);
         }
		 return;
	 }

	 if (m_nNonInteractiveMode != DZ_BRIDGE_NAMESPACE::eNonInteractiveMode::DzExporterMode) {
		 m_eSelectedNodeAssetType = SelectBestRootNodeForTransfer(true);
		 m_pSelectedNode = dzScene->getPrimarySelection();
	 }

    // Create the dialog
	if (m_bridgeDialog == nullptr)
	{
		m_bridgeDialog = new DzUnrealDialog(mw);
		m_bridgeDialog->setBridgeActionObject(this);
	}
	else
	{
		if (m_nNonInteractiveMode == 0)
		{
			m_bridgeDialog->resetToDefaults();
			m_bridgeDialog->loadSavedSettings();
		}
	}

	// Prepare member variables when not using GUI
	if (m_nNonInteractiveMode == 1)
	{
//		if (m_sRootFolder != "") m_bridgeDialog->getIntermediateFolderEdit()->setText(m_sRootFolder);

		if (m_aMorphListOverride.isEmpty() == false)
		{
			m_bEnableMorphs = true;
			m_sMorphSelectionRule = m_aMorphListOverride.join("\n1\n");
			m_sMorphSelectionRule += "\n1\n.CTRLVS\n2\nAnything\n0";
			if (m_morphSelectionDialog == nullptr)
			{
				m_morphSelectionDialog = DZ_BRIDGE_NAMESPACE::DzBridgeMorphSelectionDialog::Get(m_bridgeDialog);
			}
			m_MorphNamesToExport.clear();
			foreach(QString morphName, m_aMorphListOverride)
			{
				//QString label = MorphTools::GetMorphLabelFromName(morphName, m_pSelectedNode);
				m_MorphNamesToExport.append(morphName);
			}
		}
		else
		{
			m_bEnableMorphs = false;
			m_sMorphSelectionRule = "";
			m_MorphNamesToExport.clear();
		}

	}

	if (m_nNonInteractiveMode != DZ_BRIDGE_NAMESPACE::eNonInteractiveMode::DzExporterMode) {
		m_bridgeDialog->setEAssetType(m_eSelectedNodeAssetType);
	}

    // If the Accept button was pressed, start the export
    int dialog_choice = -1;
	if (m_nNonInteractiveMode == 0)
	{
		dialog_choice = m_bridgeDialog->exec();
	}
    if (m_nNonInteractiveMode == 1 || dialog_choice == QDialog::Accepted)
    {
		// Read GUI values
		if (readGui(m_bridgeDialog) == false)
		{
			m_nExecuteActionResult = DZ_OPERATION_FAILED_ERROR;
			return;
		}

		DzProgress* exportProgress = new DzProgress("Sending to Unreal...", 10, false, true);

		DzError result = doPromptableObjectBaking();
		if (result != DZ_NO_ERROR) {
			exportProgress->finish();
			exportProgress->cancel();
			m_nExecuteActionResult = result;
			return;
		}
		exportProgress->step();

		exportHD(exportProgress);

		// DB, 2022-June-4: Hotfix for Corrupted Imports due to UDP Packet before UpgradeToHD
		if (m_EnableSubdivisions)
		{
			QString DTUfilename = m_sDestinationPath + m_sAssetName + ".dtu";
			// Send a message to Unreal telling it to start an import
			QUdpSocket* sendSocket = new QUdpSocket(this);
			QHostAddress* sendAddress = new QHostAddress("127.0.0.1");

			sendSocket->connectToHost(*sendAddress, m_nPort);
			sendSocket->write(DTUfilename.toUtf8());
		}

		exportProgress->update(10);
		// DB 2021-09-02: messagebox "Export Complete"
		if (m_nNonInteractiveMode == 0)
		{
			QMessageBox::information(0, "DazToUnreal Bridge",
				tr("Export phase from Daz Studio complete. Please switch to Unreal to continue with Import phase."), QMessageBox::Ok);
		}

		exportProgress->finish();
	}

	m_nExecuteActionResult = DZ_NO_ERROR;

}

void DzUnrealAction::writeDTUHeader(DzJsonWriter& writer)
{
	QString sAssetId = "";
	QString sContentType = QString("Unknown");
	QString sImportName = "";

	if (m_pSelectedNode)
	{
		sAssetId = m_pSelectedNode->getAssetId();
		sImportName = m_pSelectedNode->getName();
		DzPresentation* presentation = m_pSelectedNode->getPresentation();
		if (presentation)
		{
			sContentType = presentation->getType();
		}
	}

	writer.addMember("DTU Version", 4);
	writer.addMember("Asset Name", m_sAssetName);
	writer.addMember("Import Name", sImportName); // Blender Compatibility

	if (m_bConvertRigEnabled &&
		(m_sExportRigMode == "unreal" || m_sExportRigMode == "metahuman") &&
		m_sAssetType == "SkeletalMesh")
	{
		writer.addMember("Asset Type", QString("SkeletalMesh_v2"));
	}
	else
	{
		writer.addMember("Asset Type", m_sAssetType);
	}

	writer.addMember("Use Experimental Animation Transfer", m_bAnimationUseExperimentalTransfer);
	writer.addMember("Asset Id", sAssetId); // Unity Compatibility
	writer.addMember("Content Type", sContentType);
	writer.addMember("FBX File", m_sDestinationFBX);
	QString CharacterBaseFBX = m_sDestinationFBX;
	CharacterBaseFBX.replace(".fbx", "_base.fbx");
	writer.addMember("Base FBX File", CharacterBaseFBX);
	QString CharacterHDFBX = m_sDestinationFBX;
	CharacterHDFBX.replace(".fbx", "_HD.fbx");
	writer.addMember("HD FBX File", CharacterHDFBX);
	writer.addMember("Import Folder", m_sDestinationPath);
	// DB Dec-21-2021: additional metadata
	writer.addMember("Product Name", m_sProductName);
	writer.addMember("Product Component Name", m_sProductComponentName);

}

void DzUnrealAction::writeConfiguration()
{
	if (m_pSelectedNode == nullptr)
		return;

	undoHideAllStrandBasedHair();

	DzUnrealDialog* DazToUnrealDialog = qobject_cast<DzUnrealDialog*>(m_bridgeDialog);

	QTextStream* pCSVStream = nullptr;
	QFile *pCSVfile = nullptr;

	 QString DTUfilename = m_sDestinationPath + m_sExportFilename + ".dtu";
	 QFile DTUfile(DTUfilename);
	 DTUfile.open(QIODevice::WriteOnly);
	 DzJsonWriter writer(&DTUfile);
	 writer.startObject(true);

	 writeDTUHeader(writer);

	 if (m_sAssetType == "SkeletalMesh")
	 {
		 writer.addMember("CreateUniqueSkeleton", DazToUnrealDialog->getUniqueSkeletonPerCharacter());
		 writer.addMember("ConvertToEpicSkeleton", DazToUnrealDialog->getConvertToEpicSkeleton());
		 writer.addMember("FixTwistBones", DazToUnrealDialog->getFixTwistBones());
		 writer.addMember("FaceCharacterRight", DazToUnrealDialog->getFaceCharacterRight());
		 writer.addMember("MaterialCombineMethod", DazToUnrealDialog->getMaterialCombineMethod());
	 }

	 writeStrandHairInfo(writer, m_oStrandHairExportData);

	 if (m_sAssetType == "Animation")
	 {
		 writer.addMember("FixTwistBones", DazToUnrealDialog->getFixTwistBones());
		 writer.addMember("FaceCharacterRight", DazToUnrealDialog->getFaceCharacterRight());
	 }

	 if (m_sAssetType.toLower().contains("mesh") || m_sAssetType == "Animation")
	 {
		 if (m_bExportMaterialPropertiesCSV)
		 {
			 QString filename = m_sDestinationPath + m_sExportFilename + "_Maps.csv";
			 pCSVfile = new QFile(filename);
			 if (pCSVfile->open(QIODevice::WriteOnly))
			 {
				 pCSVStream = new QTextStream(pCSVfile);
				 *pCSVStream << "Version, Object, Material, Type, Color, Opacity, File" << endl;
			 }
		 }
		 writeAllMaterials(m_pSelectedNode, writer, pCSVStream);
		 writeAllMorphs(writer);

		 // DB, 2022-July-5: Daz To Unified Bridge Format support
		 writeMorphLinks(writer);
		 writeMorphNames(writer);
		 DzBoneList aBoneList = getAllBones(m_pSelectedNode);
		 writeSkeletonData(m_pSelectedNode, writer);
		 writeHeadTailData(m_pSelectedNode, writer);
		 writeJointOrientation(aBoneList, writer);
		 writeLimitData(aBoneList, writer);
		 writePoseData(m_pSelectedNode, writer, true);

		 writeAllSubdivisions(writer);
		 writeAllDforceInfo(m_pSelectedNode, writer);

		 writeAllLodSettings(writer);
	 }

	 if (m_sAssetType == "Pose")
	 {
		 writer.addMember("FixTwistBones", DazToUnrealDialog->getFixTwistBones());
		 writer.addMember("FaceCharacterRight", DazToUnrealDialog->getFaceCharacterRight());
		writeAllPoses(writer);
	 }

	 if (m_sAssetType == "Environment")
	 {
		 writeEnvironment(writer);
	 }

	 if (m_sAssetType == "MLDeformer")
	 {
		 writer.addMember("FixTwistBones", DazToUnrealDialog->getFixTwistBones());
		 writer.addMember("FaceCharacterRight", DazToUnrealDialog->getFaceCharacterRight());
		 writeMLDeformerData(writer);
	 }

	 m_oStrandHairExportData.clear();

	 writer.finishObject();
	 DTUfile.close();

	 if (pCSVStream) delete(pCSVStream);
	 if (pCSVfile) 
	 {
		pCSVfile->close();
		delete(pCSVfile);
	 }

	 // DB, 2022-June-4: Hotfix for Corrupted Imports due to UDP Packet before UpgradeToHD
	 if (m_EnableSubdivisions == false)
	 {
		 // Send a message to Unreal telling it to start an import
		 QUdpSocket* sendSocket = new QUdpSocket(this);
		 QHostAddress* sendAddress = new QHostAddress("127.0.0.1");

		 sendSocket->connectToHost(*sendAddress, m_nPort);
		 sendSocket->write(DTUfilename.toUtf8());
	 }
}

// Setup custom FBX export options
void DzUnrealAction::setExportOptions(DzFileIOSettings& ExportOptions)
{

}

// Overrides baseclass implementation with Unreal specific resets
// Resets Default Values but Ignores any saved settings
void DzUnrealAction::resetToDefaults()
{
	DzBridgeAction::resetToDefaults();

	// Must Instantiate m_bridgeDialog so that we can override any saved states
	if (m_bridgeDialog == nullptr)
	{
		DzMainWindow* mw = dzApp->getInterface();
		m_bridgeDialog = new DzUnrealDialog(mw);
		m_bridgeDialog->setBridgeActionObject(this);
	}
	m_bridgeDialog->resetToDefaults();

	if (m_subdivisionDialog != nullptr)
	{
		foreach(QObject * obj, m_subdivisionDialog->getSubdivisionCombos())
		{
			QComboBox* combo = qobject_cast<QComboBox*>(obj);
			if (combo)
				combo->setCurrentIndex(0);
		}
	}
	// reset morph selection
	//DzBridgeMorphSelectionDialog::Get(nullptr)->PrepareDialog();

}

bool DzUnrealAction::setBridgeDialog(DzBasicDialog* arg_dlg)
{
	m_bridgeDialog = qobject_cast<DzUnrealDialog*>(arg_dlg);

	if (m_bridgeDialog == nullptr && arg_dlg != nullptr)
	{
		return false;
	}

	return true;
}

QString DzUnrealAction::readGuiRootFolder()
{
	QString rootFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazToUnreal";

	if (m_bridgeDialog)
	{
		QLineEdit* intermediateFolderEdit = nullptr;
		DzUnrealDialog* unrealDialog = qobject_cast<DzUnrealDialog*>(m_bridgeDialog);

		if (unrealDialog)
			intermediateFolderEdit = unrealDialog->getIntermediateFolderEdit();

		if (intermediateFolderEdit)
			rootFolder = intermediateFolderEdit->text().replace("\\", "/");
	}
	return rootFolder;
}

bool CompareProperties(DzColorProperty* A, DzColorProperty* B)
{
	if (A && B) {
		if (A->getColorValue() != B->getColorValue()) return false;
		DzTexture* pMapA = A->getMapValue();
		DzTexture* pMapB = B->getMapValue();
		if (pMapA && pMapB) {
			if (pMapA->getFilename() != pMapB->getFilename()) return false;
		}
		else if (pMapA || pMapB) return false;
	}
	else if (A || B) return false;

	return true;
}

bool CompareProperties(DzFloatProperty* A, DzFloatProperty* B)
{
	if (A && B) {
		if (A->getValue() != B->getValue()) return false;
	}
	else if (A || B) return false;

	return true;
}

bool CompareProperties(DzImageProperty* A, DzImageProperty* B)
{
	if (A && B) {
		DzTexture* pImageA = A->getValue();
		DzTexture* pImageB = B->getValue();
		if (pImageA && pImageB) {
			if (pImageA->getFilename() != pImageB->getFilename()) return false;
		}
		else if (pImageA || pImageB) return false;
	}
	else if (A || B) return false;

	return true;
}

bool CompareProperties(DzProperty* A, DzProperty* B)
{
	if (A && B) {
		if (A->getName() != B->getName()) return false;
		if (A->getLabel() != B->getLabel()) return false;
		if (A->className() != B->className()) return false;
		if (A->inherits("DzColorProperty")) {
			DzColorProperty* pColorA = qobject_cast<DzColorProperty*>(A);
			DzColorProperty* pColorB = qobject_cast<DzColorProperty*>(B);
			return CompareProperties(pColorA, pColorB);
		}
		else if (A->inherits("DzImageProperty")) {
			DzImageProperty* pImageA = qobject_cast<DzImageProperty*>(A);
			DzImageProperty* pImageB = qobject_cast<DzImageProperty*>(B);
			return CompareProperties(pImageA, pImageB);
		}
		else if (A->inherits("DzFloatProperty")) {
			DzFloatProperty* pFloatA = qobject_cast<DzFloatProperty*>(A);
			DzFloatProperty* pFloatB = qobject_cast<DzFloatProperty*>(B);
			return CompareProperties(pFloatA, pFloatB);
		}
		else {
//			dzApp->log("DzUnrealAction.cpp: CompareProperties(): unhandled property type: " + A->className());
		}
	}
	else if (A || B) return false;

	return true;
}

bool CompareMaterials(DzMaterial* A, DzMaterial* B)
{
	// *** REQUIRES EXTERNAL C++ RUNTIME MEMORY CALLS *** 
	//// compare maps
	//QList<DzTexturePtr> aMapListA;
	//QList<DzTexturePtr> aMapListB;
	//A->getAllMaps(aMapListA);
	//B->getAllMaps(aMapListB);
	//if (aMapListA.count() != aMapListB.count()) return false;
	//for (int i=0; i < aMapListA.count(); i++) {
	//	bool bMatchFound = false;
	//	for (int j=0; j < aMapListB.count(); j++) {
	//		DzTexturePtr pMapA = aMapListA[i];
	//		DzTexturePtr pMapB = aMapListB[j];
	//		if (pMapA->getFilename() == pMapB->getFilename()) {
	//			bMatchFound = true;
	//			break;
	//		}
	//	}
	//	if (bMatchFound == false) return false;
	//}

	if (A && B)
	{
		if (A->getDiffuseColor() != B->getDiffuseColor()) return false;

		int numPropertiesA = A->getNumProperties();
		int numPropertiesB = B->getNumProperties();
		if (numPropertiesA != numPropertiesB) return false;

		// nonsequential comparison, in case A and B properties are out of order
		for (int i = 0; i < numPropertiesA; i++)
		{
			bool bMatchFound = false;
			DzProperty* pPropertyA = A->getProperty(i);
			for (int j = 0; j < numPropertiesB; j++)
			{
				DzProperty* pPropertyB = B->getProperty(j);
				if (CompareProperties(pPropertyA, pPropertyB) == true)
				{
					bMatchFound = true;
					break;
				}
			}
			if (bMatchFound == false) return false;
		}

	}
	else if (A || B) return false;

	return true;
}

// Returns a map of material to the material it's a duplicate of.
QMap<DzMaterial*, DzMaterial*> FindDuplicateMaterials(QList<DzMaterial*> &MaterialList)
{
	QStringList aPreferedNameList;
	aPreferedNameList << "head" << "upper";
	
	QMap<DzMaterial*, DzMaterial*> Duplicates;
	for (int i = 0; i < MaterialList.count(); i++)
	{
		DzMaterial* Material = MaterialList[i];
		QString MaterialName = Material->getName();

		for (int j = i + 1; j < MaterialList.count(); j++)
		{
			DzMaterial* CompareMaterial = MaterialList[j];
			QString CompareMaterialName = CompareMaterial->getName();

			if ( CompareMaterials(Material, CompareMaterial) && !Duplicates.contains(MaterialList[j]) )
			{
				bool bPreferenceOverride = false;
				foreach (QString sPriorityName, aPreferedNameList) {
					if (CompareMaterialName.toLower().contains(sPriorityName)) {
						bPreferenceOverride = true;
						break;
					}
				}
				if (bPreferenceOverride) {
					printf("DEBUG: FindDuplicateMaterials() duplicate: %s, original: %s (preferred name override)\n", MaterialName.toLocal8Bit().constData(), CompareMaterialName.toLocal8Bit().constData());
					Duplicates.insert(Material, CompareMaterial);
				} else {
					printf("DEBUG: FindDuplicateMaterials() duplicate: %s, original: %s\n", CompareMaterialName.toLocal8Bit().constData(), MaterialName.toLocal8Bit().constData());
					Duplicates.insert(CompareMaterial, Material);					
				}
			}
		}
	}
	return Duplicates;
}

bool GetAllMaterialsByMeshName(DzNode* pNode, QMap<QString, QList<DzMaterial*>>& oMeshNameLookupTable)
{

	DzNodeList aMeshNodes;
	if (pNode->getObject()) aMeshNodes << pNode;
	foreach(QObject * pObj, pNode->getNodeChildren(true)) {
		DzNode* pChildNode = qobject_cast<DzNode*>(pObj);
		if (pChildNode && pChildNode->getObject()) {
			aMeshNodes << pChildNode;
		}
	}

	foreach(DzNode * pNode, aMeshNodes) {
		QString sMeshName = pNode->getName();
		QList<DzMaterial*> aMaterialsList;

		if (pNode->getObject()) {
			if (DzShape* pShape = pNode->getObject()->getCurrentShape()) {
				for (int i = 0; i < pShape->getNumMaterials(); i++) {
					QString sMeshName = pNode->getName();
					aMaterialsList << pShape->getMaterial(i);
				}
			}
		}

		oMeshNameLookupTable.insert(sMeshName, aMaterialsList);
	}

	return true;

}

QList<DzMaterial*> GetAllMaterials(DzNode* pNode)
{
	QList<DzMaterial*> aMaterialsList;

	DzNodeList aMeshNodes;
	if (pNode->getObject()) aMeshNodes << pNode;
	foreach(QObject * pObj, pNode->getNodeChildren(true)) {
		DzNode* pChildNode = qobject_cast<DzNode*>(pObj);
		if (pChildNode && pChildNode->getObject()) {
			aMeshNodes << pChildNode;
		}
	}

	foreach(DzNode * pNode, aMeshNodes) {
		if (pNode->getObject()) {
			if (DzShape* pShape = pNode->getObject()->getCurrentShape()) {
				for (int i = 0; i < pShape->getNumMaterials(); i++) {
					aMaterialsList << pShape->getMaterial(i);
				}
			}
		}
	}

	return aMaterialsList;
}

bool DzUnrealAction::postProcessFbx(QString fbxFilePath)
{
	bool bResult = false;

	QString sGeneration = m_pSelectedNode->getName();
	bool bIsG9 = (sGeneration == "Genesis9");
	bool bIsG8or81 = (sGeneration.contains("Genesis8"));
	bool bIsG3 = (sGeneration.contains("Genesis3"));
	bool bIsG2 = (sGeneration.contains("Genesis2"));
	bool bIsG1 = (sGeneration == "Genesis");
	bool bIsSupportedFigure = (m_pSelectedNode->inherits("DzFigure") && (bIsG9 || bIsG8or81 || bIsG3 || bIsG2 || bIsG1) );
	
	if ( m_sAssetType == "SkeletalMesh" &&
		m_bConvertRigEnabled &&
//		!m_bEnableMorphs && !m_EnableSubdivisions &&
//		(m_sExportRigMode == "unreal" || m_sExportRigMode == "metahuman") &&
		bIsSupportedFigure)
	{
		// bake bind pose
		m_bBakeMeshesToSingleBindPose = true;

		QString sTrueRigMode = "";
		sTrueRigMode = m_sExportRigMode;
		m_bConvertFbxJointsEnabled = false;
		m_sExportRigMode = "";
		m_bDetachGeometry = false;
		bResult = DzBridgeAction::postProcessFbx(fbxFilePath);
		m_sExportRigMode = sTrueRigMode;
		m_bConvertFbxJointsEnabled = true;
		if (!bResult)
		{
			return false;
		}

		QString sArchiveFilename = "/g9_unreal_apose_fixed_4.fbx";
		if (bIsG1) sArchiveFilename = "/g1_unreal_apose_fixed.fbx";
		QString sEmbeddedArchivePath = m_sEmbeddedFolderPath + sArchiveFilename;
		QFile srcFile(sEmbeddedArchivePath);
		QString tempPathArchive = dzApp->getTempPath() + sArchiveFilename;
		bool replace = true;
		DzBridgeNameSpace::DzBridgeAction::copyFile(&srcFile, &tempPathArchive, replace);
		srcFile.close();

		bResult = postProcessRigConversion(m_sExportRigMode, fbxFilePath);
		if (!bResult)
		{
			return false;
		}
	}
	else
	{
		//FbxTools::PostProcessRigForUnreal(fbxFilePath, m_bFixTwistBones);
		bResult = DzBridgeAction::postProcessFbx(fbxFilePath);
		if (!bResult)
		{
			return false;
		}

	}

	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("Base Mesh Scene");
	if (exLoadFbxScene(pScene, fbxFilePath) == false) {
		pScene->Destroy();
		return false;
	}	
	// Rename Morphs to Morph Labels
	FbxTools::RenameMorphs(pScene, m_AvailableMorphsTable, true);	
	if (openFBX->SaveScene(pScene, fbxFilePath, -1, m_bEmbedTexturesInOutputFile) == false)
	{
		QString sFbxErrorMessage = QObject::tr("ERROR: DazToUnreal: openFBX->SaveScene():\n\n")
			+ QString("File: \"%1\"\n\n").arg(fbxFilePath)
			+ QString("FbxStatusCode: %1\n").arg(openFBX->GetErrorCode())
			+ QString("Error Message: %1\n\n").arg(openFBX->GetErrorString());
		dzApp->log(sFbxErrorMessage);
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, QObject::tr("Error"),
			QObject::tr("An error occurred while processing the Fbx file:\n\n") + sFbxErrorMessage, QMessageBox::Ok);
		return false;
	}
	pScene->Destroy();
	
	// Insert Unreal specific Fbx Post-processing here
	QList<DzMaterial*> aMaterialsList = GetAllMaterials(m_pSelectedNode);
	QMap<DzMaterial *, DzMaterial *> DuplicateMaterials = FindDuplicateMaterials(aMaterialsList);
	QList<QString> MaterialSlotNames;

	DzUnrealDialog* DazToUnrealDialog = qobject_cast<DzUnrealDialog*>(m_bridgeDialog);
	int nCombineMethod = DazToUnrealDialog->getMaterialCombineMethodAsInt();
	FbxTools::PostProcessMaterials(fbxFilePath, DuplicateMaterials, MaterialSlotNames, nCombineMethod);

	return bResult;
}

// DB 2024-04-17: Moved here from DzBridgeAction.cpp
void DzUnrealAction::writeMLDeformerData(DzJsonWriter& writer)
{
	writer.addMember("AlembicFile", m_sDestinationPath + m_sExportFilename + ".abc");
}

// DB 2023-05-18: Added support for MLDeformer
// Overrides baseclass implementation with Unreal specific export
bool DzUnrealAction::exportNode(DzNode* Node)
{
	if (Node == nullptr)
		return false;

	DzUnrealDialog* DazToUnrealDialog = qobject_cast<DzUnrealDialog*>(m_bridgeDialog);
	m_bFixTwistBones = DazToUnrealDialog->getFixTwistBones();
	m_bMLDeformerExportFace = DazToUnrealDialog->getMLDeformerIncludeFace();

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(Node);

	if (m_sAssetType == "MLDeformer")
	{
		QDir dir;
		dir.mkpath(m_sDestinationPath);
		DzUnrealDialog* unrealBridgeDialog = qobject_cast<DzUnrealDialog*>(m_bridgeDialog);
		MLDeformer::GeneratePoses(Node, 
			unrealBridgeDialog->getMLDeformerPoseCountEdit()->text().toInt(),
			unrealBridgeDialog->getMLDeformerIncludeFingers(),
			unrealBridgeDialog->getMLDeformerIncludeToes());
		if (m_bEnableMorphs)
		{
			MLDeformer::GenerateMorphs(Node, m_MorphNamesToExport);
			m_bAnimationExportActiveCurves = true;
		}
		exportAnimation();
		MLDeformer::ExportTrainingData(Node, m_sDestinationPath + m_sExportFilename + ".abc");
		writeConfiguration();
		return true;
	}

	return DzBridgeAction::exportNode(Node);
}

void DzUnrealAction::exportAnimation()
{
	DzBridgeAction::exportAnimation();
	if (m_sAssetName == "MLDeformer")
	{
		// Insert MLDeformer specific code here
	}
}

void DzUnrealAction::exportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale)
{
	DzBridgeAction::exportNodeAnimation(Bone, BoneMap, AnimBaseLayer, FigureScale);
	if (m_sAssetName == "MLDeformer")
	{
		// Insert MLDeformer specific code here
	}
}


bool DzUnrealAction::readGui(DZ_BRIDGE_NAMESPACE::DzBridgeDialog* pBridgeDialog)
{
	bool bResult = DzBridgeAction::readGui(pBridgeDialog);
	if (!bResult)
	{
		return false;
	}

	// Read in Custom GUI values
	DzUnrealDialog* unrealDialog = qobject_cast<DzUnrealDialog*>(pBridgeDialog);
	if (unrealDialog)
	{
		m_nPort = unrealDialog->getPortEdit()->text().toInt();
		m_bExportMaterialPropertiesCSV = unrealDialog->getExportMaterialPropertyCSVCheckBox()->isChecked();
	}

	return true;
}

bool DzUnrealAction::preProcessScene(DzNode* parentNode)
{
	DzProgress* pProgress = new DzProgress(tr("PreProcessing Scene"), 100, false, true);

	if (m_sAssetType == "SkeletalMesh" || m_sAssetType == "StaticMesh")
	{
		QList<DzNode*> aHairNodesList = findAllStrandBasedHair();
		if (aHairNodesList.count() > 0) {
			QDir().mkpath(m_sDestinationPath);
			if (m_bCombineStrandHairParts)
			{
				QString sHairPostfix = QString("_%1.abc").arg("hair");
				QString sAbcFilename = QString(m_sDestinationFBX).replace(".fbx", sHairPostfix, Qt::CaseInsensitive);
				writeHair(sAbcFilename, aHairNodesList, "unreal");
			}
			else
			{
				foreach(DzNode * pHairNode, aHairNodesList) {
					if (isStrandBasedHair(pHairNode) == false) continue;
					QString sHairPostfix = QString("_%1.abc").arg(cleanString(pHairNode->getLabel()));
					QString sAbcFilename = QString(m_sDestinationFBX).replace(".fbx", sHairPostfix, Qt::CaseInsensitive);
					writeHair(sAbcFilename, QList<DzNode*>() << pHairNode, "unreal");
				}
			}
		}
	}

	QString sGeneration = parentNode->getName();
	bool bIsG9 = (sGeneration == "Genesis9");
	bool bIsG8or81 = (sGeneration.contains("Genesis8"));
	bool bIsG3 = (sGeneration.contains("Genesis3"));
	bool bIsG2 = (sGeneration.contains("Genesis2"));
	bool bIsG1 = (sGeneration == "Genesis");
	bool bIsSupportedFigure = (m_pSelectedNode->inherits("DzFigure") && (bIsG9 || bIsG8or81 || bIsG3 || bIsG2 || bIsG1) );	
	if (parentNode &&
		parentNode->inherits("DzFigure") &&
		m_sAssetType == "SkeletalMesh" &&
//		!m_bEnableMorphs && !m_EnableSubdivisions &&
		bIsSupportedFigure &&
		!m_sExportRigMode.isEmpty() &&
		m_sExportRigMode != "" &&
		m_sExportRigMode != "--")
	{
		m_bConvertRigEnabled = true;
	}
	else
	{
		m_bConvertRigEnabled = false;		
	}
	
	hideAllStrandBasedHair();

	if (m_bConvertRigEnabled && bIsSupportedFigure) {
		bool bGenerateProxyMeshResult = false;
		m_sMvcProxyMeshFilePath = getTempBasefilename() + "_mvc_proxy_mesh.fbx";
		bGenerateProxyMeshResult = generateProxyMesh(parentNode, m_sMvcProxyMeshFilePath, false);
		if (bGenerateProxyMeshResult == false || validateProxyMeshVerts(m_sMvcProxyMeshFilePath, sGeneration) == false) {
			pProgress->cancel();
			pProgress->finish();
			return false;
		}
	}

	bool bConvertRigBackup = m_bConvertRigEnabled;
	m_bConvertRigEnabled = false;
	DzBridgeAction::preProcessScene(parentNode);
	m_bConvertRigEnabled = bConvertRigBackup;

	if (m_bConvertRigEnabled && m_bEnableMorphs) 
	{		
		// ARKit_facs_ctrl_ARKitEnable
		exSetArkitCorrectives(1.0, parentNode);

		m_sMorphProxyFilePath = getTempBasefilename() + "_morph_proxy.fbx";
		bool bGenerateProxyMeshResult = generateProxyMesh(parentNode, m_sMorphProxyFilePath, true);
		if (bGenerateProxyMeshResult == false) {
			pProgress->cancel();
			pProgress->finish();
			return false;
		}
		
		// Retarget Blendshape Mesh to Base Figure Skeleton
		QString sFileBasePath = getTempBasefilename() + "_morph_rig";
		generateMorphProxyRigs(parentNode, sFileBasePath, m_MorphNamesToExport, m_aMorphProxyRigList);
		retargetBlendshapesToBaseRig(m_aMorphProxyRigList, sFileBasePath, sGeneration + ".Shape");
		
		// load and fix mouth close
		QString sSourceFilename = m_sMorphProxyFilePath; // store original filename as source
		QString sOutputFilename = QString(m_sMorphProxyFilePath).replace(".fbx", "-fixed.fbx", Qt::CaseInsensitive); // rename to "-fixed.fbx" for output filename
		if (fixMouthCloseBlendshape(parentNode, sSourceFilename, sOutputFilename) == false) {
			// no op
		} else {
			m_sMorphProxyFilePath = sOutputFilename;
		}

		QString sPoseFilename = "/g9_unreal_apose_fixed_4.fbx";
		QString sEmbeddedFilePath = m_sEmbeddedFolderPath + sPoseFilename;
		QFile srcFile(sEmbeddedFilePath);
		QString sTempPoseFilePath = dzApp->getTempPath() + sPoseFilename;
		bool replace = true;
		DzBridgeNameSpace::DzBridgeAction::copyFile(&srcFile, &sTempPoseFilePath, replace);
		srcFile.close();

		QString sUnrealMannyRigFile = "g9_to_unreal_manny.json";
		QString sG8UnrealRigFile = "g8_to_unreal.json";
		QString sMetahumanRigFile = "g9_to_metahuman.json";
		QString sG8MetahumanRigFile = "g8_to_metahuman.json";
		QString sUnityRigFile = "g9_to_unity.json";
		QString sG8UnityRigFile = "g8_to_unity.json";
		QString sMixamoRigFile = "g9_to_mixamo.json";
		QString sG8MixamoRigFile = "g8_to_mixamo.json";
		// Legacy support
		QString sG2UnrealMannyRigFile = "g2_to_unreal.json";

		QStringList aScriptFilelist = (QStringList() <<
			sUnrealMannyRigFile << sG8UnrealRigFile <<
			sMetahumanRigFile << sG8MetahumanRigFile <<
			sUnityRigFile <<
			sMixamoRigFile << sG8MixamoRigFile <<
			sG2UnrealMannyRigFile
			);
		// copy 
		foreach(auto sScriptFilename, aScriptFilelist)
		{
			bool replace = true;
			QString sEmbeddedFilepath = m_sEmbeddedFolderPath + "/" + sScriptFilename;
			QFile srcFile(sEmbeddedFilepath);
			QString tempFilepath = dzApp->getTempPath() + "/" + sScriptFilename;
			DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFile, &tempFilepath, replace);
			srcFile.close();
		}

		// Compile arguments
		QString sConfigFile;
		if (m_sExportRigMode == "metahuman") {
			if (bIsG9) {
				sConfigFile = dzApp->getTempPath() + "/" + sMetahumanRigFile;
			} else {
				sConfigFile = dzApp->getTempPath() + "/" + sG8MetahumanRigFile;
			}
		}
		else if (m_sExportRigMode == "unreal") {
			if (bIsG9) {
				sConfigFile = dzApp->getTempPath() + "/" + sUnrealMannyRigFile;
			}
			else if (bIsG2 || bIsG1) {
				sConfigFile = dzApp->getTempPath() + "/" + sG2UnrealMannyRigFile;
			}
			else {
				sConfigFile = dzApp->getTempPath() + "/" + sG8UnrealRigFile;
			}
		}
		else if (m_sExportRigMode == "unity") {
			if (bIsG9) {
				sConfigFile = dzApp->getTempPath() + "/" + sUnityRigFile;
			}
			else {
				sConfigFile = dzApp->getTempPath() + "/" + sG8UnityRigFile;
			}
		}
		else if (m_sExportRigMode == "mixamo") {
			if (bIsG9) {
				sConfigFile = dzApp->getTempPath() + "/" + sMixamoRigFile;
			}
			else {
				sConfigFile = dzApp->getTempPath() + "/" + sG8MixamoRigFile;
			}
		}

		// post-process proxy rigs
		QString sBaseFigurePose = sFileBasePath + "_base.fbx";
		foreach (QString sMorphRigFile, m_aMorphProxyRigList)
		{
			QString sMorphName = QString(sMorphRigFile).replace(sFileBasePath + "_", "").replace(".fbx", "");
			QString sMorphLabel = sMorphName;
			if (sMorphName != "base") sMorphLabel = m_AvailableMorphsTable[sMorphName].Label;
			FbxTools::ProxyMeshBoneRenamer(sMorphRigFile, sConfigFile);
			FbxTools::UnrealJointFixCallback2 oUnrealJointFixer;
			postProcessRigConversion(sMorphRigFile, "", "", "", &oUnrealJointFixer, sTempPoseFilePath, "", "", "", "", false);
			OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
			FbxScene* pScene = openFBX->CreateScene("Base Mesh Scene");
			if (exLoadFbxScene(pScene, sMorphRigFile) == false) {
				pScene->Destroy();
				continue;
			}
			FbxNode* RootNode = pScene->GetRootNode();
			QList<FbxNode*> aMeshList;
			FbxTools::GetAllMeshes(RootNode, aMeshList);
			FbxTools::SetSceneTimeMode(pScene, FbxTime::eFrames30);
			FbxAnimStack* pAnimStack = FbxAnimStack::Create(pScene, "Take 001");
			FbxAnimLayer* pAnimBaseLayer = FbxAnimLayer::Create(pScene, "BaseLayer");
			pAnimStack->AddMember(pAnimBaseLayer);
			pScene->SetCurrentAnimationStack(pAnimStack);
			for (int i = 0, n = pScene->GetSrcObjectCount<FbxAnimStack>(); i < n; ++i) {
				FbxAnimStack* pStack = pScene->GetSrcObject<FbxAnimStack>(i);
				if (pStack && pStack != pAnimStack) pStack->Destroy();
			}
			FbxPose* pBasePose = FbxPose::Create(openFBX->GetManager(), "Base Pose");
			FbxTools::LoadAndPose(sBaseFigurePose, pScene, 0, false, false, QList<QString>(), pBasePose);
			QList<FbxNode*> aBoneList;  FbxTools::GetBoneList(RootNode, aBoneList);
			foreach(FbxNode* pNode, aBoneList) {
				FbxTools::AddKeyCurrentNode(pNode, pAnimBaseLayer, FbxTools::MakeFrame(0));
			}
			FbxPose* pMorphPose = FbxPose::Create(openFBX->GetManager(), "Morph Pose");
			FbxTools::LoadAndPose(sMorphRigFile, pScene, 0, false, false, QList<QString>(), pMorphPose);
			foreach(FbxNode* pNode, aBoneList) {
				FbxTools::AddKeyCurrentNode(pNode, pAnimBaseLayer, FbxTools::MakeFrame(10));
			}
			FbxNode* pRootBone = FbxTools::GetRootBone(pScene);
			if (pRootBone) {
				FbxTools::AddMorphCurveByName(pRootBone, pAnimBaseLayer, FbxTools::MakeFrame(0), sMorphLabel, 0.0f, true);
				FbxTools::AddMorphCurveByName(pRootBone, pAnimBaseLayer, FbxTools::MakeFrame(10), sMorphLabel, 1.0f, true);
			}
			pScene->SetCurrentAnimationStack(pAnimStack);
			FbxTools::ApplyBindPose(pScene, pBasePose);
			foreach(FbxNode* pNode, aMeshList) {
				// remove clusters
				FbxMesh* pMesh = pNode->GetMesh();
				if (pMesh) {
					for (int i = 0; i < pMesh->GetDeformerCount(); i++) {
						FbxDeformer* pDeformer = pMesh->GetDeformer(0);
						pMesh->RemoveDeformer(0);
						pDeformer->Destroy();
					}
				}
				pScene->RemoveNode(pNode);
				pNode->Destroy();
			}
			for (int i = 0; i < pScene->GetMaterialCount(); i++) {
				FbxSurfaceMaterial* pMaterial = pScene->GetMaterial(0);
				if (pMaterial == nullptr) continue;
				pScene->RemoveMaterial(pMaterial);
			}
			QString sMorphPoseFile = m_sDestinationPath + "/" + sMorphLabel + "_pose.fbx";
			if (openFBX->SaveScene(pScene, sMorphPoseFile, -1, false) == false)
			{
			}
			pScene->Destroy();
		}
		
		// disable normal morph export pathway
		m_bEnableMorphs = false;
	}

	DzBridgeAction::preProcessRigConversion(parentNode);
	
	pProgress->finish();


	return true;
}

bool DzUnrealAction::undoPreProcessScene()
{
	if (DzBridgeAction::undoPreProcessScene() == false) return false;

//	undoHideAllStrandBasedHair();

	return true;
}




#include "moc_DzUnrealAction.cpp"
