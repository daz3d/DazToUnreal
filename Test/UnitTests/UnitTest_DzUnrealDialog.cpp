#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzUnrealDialog.h"
#include "DzUnrealDialog.h"


UnitTest_DzUnrealDialog::UnitTest_DzUnrealDialog()
{
	m_testObject = (QObject*) new DzUnrealDialog();
}

bool UnitTest_DzUnrealDialog::runUnitTests()
{
	RUNTEST(_DzUnrealDialog);
	RUNTEST(getIntermediateFolderEdit);
	RUNTEST(getPortEdit);
	RUNTEST(getExportMaterialPropertyCSVCheckBox);
	RUNTEST(resetToDefaults);
	RUNTEST(HandleSelectIntermediateFolderButton);
	RUNTEST(HandlePortChanged);
	RUNTEST(loadSavedSettings);

	return true;
}

bool UnitTest_DzUnrealDialog::_DzUnrealDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzUnrealDialog());
	return bResult;
}

bool UnitTest_DzUnrealDialog::getIntermediateFolderEdit(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->getIntermediateFolderEdit());
	return bResult;
}

bool UnitTest_DzUnrealDialog::getPortEdit(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->getPortEdit());
	return bResult;
}

bool UnitTest_DzUnrealDialog::getExportMaterialPropertyCSVCheckBox(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->getExportMaterialPropertyCSVCheckBox());
	return bResult;
}

bool UnitTest_DzUnrealDialog::resetToDefaults(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->resetToDefaults());
	return bResult;
}

bool UnitTest_DzUnrealDialog::HandleSelectIntermediateFolderButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->HandleSelectIntermediateFolderButton());
	return bResult;
}

bool UnitTest_DzUnrealDialog::HandlePortChanged(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->HandlePortChanged(""));
	return bResult;
}

bool UnitTest_DzUnrealDialog::loadSavedSettings(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealDialog*>(m_testObject)->loadSavedSettings());
	return bResult;
}

#include "moc_UnitTest_DzUnrealDialog.cpp"
#endif