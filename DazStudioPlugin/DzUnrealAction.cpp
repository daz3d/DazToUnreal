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

#include "DzUnrealAction.h"
#include "DzUnrealDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#include "MLDeformer.h"
#include "FbxTools.h"

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

}

void DzUnrealAction::executeAction()
{
	m_nExecuteActionResult = DZ_OPERATION_FAILED_ERROR;
	m_eSelectedNodeAssetType = DZ_BRIDGE_NAMESPACE::EAssetType::None;
	
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

void DzUnrealAction::writeConfiguration()
{
	if (m_pSelectedNode == nullptr)
		return;

	DzUnrealDialog* DazToUnrealDialog = qobject_cast<DzUnrealDialog*>(m_bridgeDialog);

	QTextStream* pCSVStream = nullptr;
	QFile *pCSVfile = nullptr;

	 QString DTUfilename = m_sDestinationPath + m_sExportFilename + ".dtu";
	 QFile DTUfile(DTUfilename);
	 DTUfile.open(QIODevice::WriteOnly);
	 DzJsonWriter writer(&DTUfile);
	 writer.startObject(true);

	 if (m_sAssetType == "SkeletalMesh") m_sAssetType = "SkeletalMesh_v2";
	 writeDTUHeader(writer);
	if (m_sAssetType == "SkeletalMesh_v2") m_sAssetType = "SkeletalMesh";

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

bool CompareMaterials(DzMaterial* A, DzMaterial* B)
{
	// compare maps
	QList<DzTexturePtr> aMapListA;
	QList<DzTexturePtr> aMapListB;
	A->getAllMaps(aMapListA);
	B->getAllMaps(aMapListB);

	if (aMapListA.count() != aMapListB.count()) return false;
	for (int i=0; i < aMapListA.count(); i++) {
		bool bMatchFound = false;
		for (int j=0; j < aMapListB.count(); j++) {
			DzTexturePtr pMapA = aMapListA[i];
			DzTexturePtr pMapB = aMapListB[j];
			if (pMapA->getFilename() == pMapB->getFilename()) {
				bMatchFound = true;
				break;
			}
		}
		if (bMatchFound == false) return false;
	}
	
	if (A->getDiffuseColor() != B->getDiffuseColor()) return false;

/*
	DzColorProperty* pPropertyA = qobject_cast<DzColorProperty*>( A->findProperty("Diffuse Color") );
	DzColorProperty* pPropertyB = qobject_cast<DzColorProperty*>( B->findProperty("Diffuse Color") );
	if (pPropertyA && pPropertyB) {
		if (pPropertyA->getColorValue() != pPropertyB->getColorValue()) return false;
		DzTexture* pMapA = pPropertyA->getMapValue();
		DzTexture* pMapB = pPropertyB->getMapValue();
		if (pMapA && pMapB) {
			if (pMapA->getFilename() != pMapB->getFilename()) return false;			
		}
		else if (pMapA || pMapB) return false;
	}
	else if (pPropertyA || pPropertyB) return false;
*/

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
				printf("DEBUG: FindDuplicateMaterials() duplicate: %s, original: %s\n", CompareMaterialName.toLocal8Bit().constData(), MaterialName.toLocal8Bit().constData());
				if (bPreferenceOverride) {
					Duplicates.insert(Material, CompareMaterial);
				} else {
					Duplicates.insert(CompareMaterial, Material);					
				}
			}
		}
	}
	return Duplicates;
}

bool DzUnrealAction::postProcessFbx(QString fbxFilePath)
{
	m_bConvertFbxJointsEnabled = true;
	bool bResult = DzBridgeAction::postProcessFbx(fbxFilePath);

	if (!bResult)
	{
		return false;
	}

	// Insert Unreal specific Fbx Post-processing here
	QList<DzMaterial*> aMaterialsList;
	DzNode* pNode = m_pSelectedNode;
	DzNodeList aMeshNodes;
	if (pNode->getObject()) aMeshNodes << pNode;
	foreach (QObject* pObj, pNode->getNodeChildren(true)) {
		DzNode* pChildNode = qobject_cast<DzNode*>(pObj);
		if (pChildNode && pChildNode->getObject()) {
			aMeshNodes << pChildNode;
		}
	}
	foreach (DzNode* pNode, aMeshNodes) {
		if (pNode->getObject()) {
			if (DzShape* pShape = pNode->getObject()->getCurrentShape()) {
				for (int i=0; i < pShape->getNumMaterials(); i++) {
					aMaterialsList << pShape->getMaterial(i);
				}				
			}
		}		
	}

	QMap<DzMaterial *, DzMaterial *> DuplicateMaterials = FindDuplicateMaterials(aMaterialsList);
	QList<QString> MaterialSlotNames;
	FbxTools::PreProcessFbxFile(fbxFilePath, m_sAssetName, DuplicateMaterials, MaterialSlotNames);
		
	return true;
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

	hideAllStrandBasedHair();

	m_bConvertRigEnabled = true;
	m_sExportRigMode = "unreal";
	DzBridgeAction::preProcessScene(parentNode);

	pProgress->finish();


	return true;
}

bool DzUnrealAction::undoPreProcessScene()
{
	if (DzBridgeAction::undoPreProcessScene() == false) return false;

	undoHideAllStrandBasedHair();

	return true;
}




#include "moc_DzUnrealAction.cpp"
