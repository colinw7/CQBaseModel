#ifndef CQDataModelTest_H
#define CQDataModelTest_H

#include <QFrame>

class CQTableView;
class CQDataModel;
class CQProxyModel;

class CQDataModelTest : public QFrame {
  Q_OBJECT

 public:
  CQDataModelTest();
 ~CQDataModelTest();

  QSize sizeHint() const { return QSize(800, 600); }

 private:
  CQTableView*  table_ { nullptr };
  CQDataModel*  model_ { nullptr };
  CQProxyModel* proxy_ { nullptr };
};

#endif
