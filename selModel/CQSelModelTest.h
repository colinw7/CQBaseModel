#ifndef CQSelModelTest_H
#define CQSelModelTest_H

#include <QFrame>

class CQSelView;
class CQCsvModel;

class CQSelModelTest : public QFrame {
  Q_OBJECT

 public:
  CQSelModelTest();
 ~CQSelModelTest();

  bool isCommentHeader() const { return commentHeader_; }
  void setCommentHeader(bool b) { commentHeader_ = b; }

  bool isFirstLineHeader() const { return firstLineHeader_; }
  void setFirstLineHeader(bool b) { firstLineHeader_ = b; }

  void load(const QString &filename);

  void init();

  QSize sizeHint() const override { return QSize(800, 600); }

 private:
  bool        commentHeader_   { false };
  bool        firstLineHeader_ { false };
  CQSelView*  view_            { nullptr };
  CQCsvModel* model_           { nullptr };
};

#endif
