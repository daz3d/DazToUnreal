#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzUnrealAction : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzUnrealAction();
	bool runUnitTests();

private:
	bool _DzUnrealAction(UnitTest::TestResult* testResult);
	bool setBridgeDialog(UnitTest::TestResult* testResult);
	bool resetToDefaults(UnitTest::TestResult* testResult);
	bool readGuiRootFolder(UnitTest::TestResult* testResult);
	bool executeAction(UnitTest::TestResult* testResult);
	bool writeConfiguration(UnitTest::TestResult* testResult);
	bool setExportOptions(UnitTest::TestResult* testResult);

};

#endif