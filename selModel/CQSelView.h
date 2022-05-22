#ifndef CQSelView_H
#define CQSelView_H

#include <QTableView>

class CQSelModel;
class QSortFilterProxyModel;

class CQSelView : public QTableView {
  Q_OBJECT

 public:
  CQSelView(QWidget *parent=nullptr);

  void setModel(QAbstractItemModel *model) override;

 private slots:
  void updateModelFromSelection();

 private:
  CQSelModel*            selModel_  { nullptr };
  QSortFilterProxyModel* sortModel_ { nullptr };
};

#endif
