#include "Test.h"
#include "dialogs/ExternalToolsDialog.h"

#include "ui/DiffView/HunkWidget.h"
//#include "ui/DiffView/Line.h"
#include <QString>


using namespace QTest;

class TestEditorLineInfo: public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

  void editorLineMarkers1();

private:
  int closeDelay = 0;
  git::Repository mRepo;
  //HunkWidget* mHunk{nullptr};
};

void TestEditorLineInfo::initTestCase()
{
  bool lfs = false;
  bool submodule = false;

  //mHunk = new HunkWidget(nullptr, diff, patch, staged, index, lfs, submodule, nullptr);
}

void TestEditorLineInfo::editorLineMarkers1() {
    QString path = Test::extractRepository("editorLineMarkers1.zip");
    QVERIFY(!path.isEmpty());
    mRepo = git::Repository::open(path);

    //auto hunk = new HunkWidget(nullptr, )
}

void TestEditorLineInfo::cleanupTestCase()
{
//  delete mHunk;
//  mHunk = nullptr;
  qWait(closeDelay);
}

TEST_MAIN(TestEditorLineInfo)
//#include "HunkWidget.moc"
#include "EditorLineInfos.moc"
