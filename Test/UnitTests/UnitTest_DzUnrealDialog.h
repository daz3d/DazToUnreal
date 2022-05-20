#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzUnrealDialog : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzUnrealDialog();
	bool runUnitTests();

private:
	bool _DzUnrealDialog(UnitTest::TestResult* testResult);
	bool getIntermediateFolderEdit(UnitTest::TestResult* testResult);
	bool getPortEdit(UnitTest::TestResult* testResult);
	bool getExportMaterialPropertyCSVCheckBox(UnitTest::TestResult* testResult);
	bool resetToDefaults(UnitTest::TestResult* testResult);
	bool HandleSelectIntermediateFolderButton(UnitTest::TestResult* testResult);
	bool HandlePortChanged(UnitTest::TestResult* testResult);
	bool loadSavedSettings(UnitTest::TestResult* testResult);


};


#endif