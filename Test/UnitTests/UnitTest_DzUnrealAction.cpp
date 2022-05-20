#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzUnrealAction.h"
#include "DzUnrealAction.h"


UnitTest_DzUnrealAction::UnitTest_DzUnrealAction()
{
	m_testObject = (QObject*) new DzUnrealAction();
}

bool UnitTest_DzUnrealAction::runUnitTests()
{
	RUNTEST(_DzUnrealAction);
	RUNTEST(setBridgeDialog);
	RUNTEST(resetToDefaults);
	RUNTEST(readGuiRootFolder);
	RUNTEST(executeAction);
	RUNTEST(writeConfiguration);
	RUNTEST(setExportOptions);

	return true;
}

bool UnitTest_DzUnrealAction::_DzUnrealAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzUnrealAction());
	return bResult;
}

bool UnitTest_DzUnrealAction::setBridgeDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzUnrealAction*>(m_testObject)->setBridgeDialog(nullptr));
	return bResult;
}

bool UnitTest_DzUnrealAction::resetToDefaults(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealAction*>(m_testObject)->resetToDefaults());
	return bResult;
}

bool UnitTest_DzUnrealAction::readGuiRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealAction*>(m_testObject)->readGuiRootFolder());
	return bResult;
}

bool UnitTest_DzUnrealAction::executeAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealAction*>(m_testObject)->executeAction());
	return bResult;
}

bool UnitTest_DzUnrealAction::writeConfiguration(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzUnrealAction*>(m_testObject)->writeConfiguration());
	return bResult;
}

bool UnitTest_DzUnrealAction::setExportOptions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzFileIOSettings arg;
	TRY_METHODCALL(qobject_cast<DzUnrealAction*>(m_testObject)->setExportOptions(arg));
	return bResult;
}


#include "moc_UnitTest_DzUnrealAction.cpp"

#endif