#include <CQSelModelTest.h>
#include <CQSelView.h>
#include <CQCsvModel.h>
#include <CQUtil.h>
#include <CQMsgHandler.h>

#define CQ_APP_H 1

#ifdef CQ_APP_H
#include <CQApp.h>
#else
#include <QApplication>
#endif

#include <QVBoxLayout>
#include <QSortFilterProxyModel>
#include <iostream>

int
main(int argc, char **argv)
{
  CQMsgHandler::install();

#ifdef CQ_APP_H
  CQApp app(argc, argv);
#else
  QApplication app(argc, argv);
#endif

  std::vector<QString> filenames;

  bool commentHeader   = false;
  bool firstLineHeader = false;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      std::string arg = &argv[i][1];

      if      (arg == "comment_header")
        commentHeader = true;
      else if (arg == "first_line_header")
        firstLineHeader = true;
      else
        std::cerr << "Invalid option '" << arg << "'\n";
    }
    else
      filenames.push_back(argv[i]);
  }

  //---

  CQSelModelTest test;

  test.setCommentHeader  (commentHeader);
  test.setFirstLineHeader(firstLineHeader);

  if (! filenames.empty())
    test.load(filenames[0]);

  test.init();

  test.show();

  app.exec();

  return 0;
}

//-----

CQSelModelTest::
CQSelModelTest()
{
  auto *layout = new QVBoxLayout(this);

  view_ = CQUtil::makeWidget<CQSelView>("selView");

  layout->addWidget(view_);
}

CQSelModelTest::
~CQSelModelTest()
{
  delete model_;
}

void
CQSelModelTest::
load(const QString &filename)
{
  model_ = new CQCsvModel;

  if (isCommentHeader())
    model_->setCommentHeader(true);

  if (isFirstLineHeader())
    model_->setFirstLineHeader(true);

  model_->load(filename);
}

void
CQSelModelTest::
init()
{
  view_->setModel(model_);
}
