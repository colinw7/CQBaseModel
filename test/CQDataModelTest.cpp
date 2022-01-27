#include <CQDataModelTest.h>
#include <CQDataModel.h>
#include <CQTableView.h>
#include <CQHeaderView.h>
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
#include <QCheckBox>
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

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      std::string arg = &argv[i][1];

      std::cerr << "Invalid option '" << arg << "'\n";
    }
    else {
      std::cerr << "Invalid arg '" << argv[i] << "'\n";
    }
  }

  //---

  CQDataModelTest test;

  test.show();

  app.exec();

  return 0;
}

//-----

static bool ascendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2)
{
  return s1.first < s2.first;
}

static bool decendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2)
{
  return s1.first > s2.first;
}

class CQProxyModel : public QAbstractProxyModel {
 public:
  CQProxyModel(CQDataModel *model) {
    setSourceModel(model);
  }

  // get column count
  int columnCount(const QModelIndex &parent=QModelIndex()) const override {
    QAbstractItemModel *model = this->sourceModel();

    return model->columnCount(mapToSource(parent));
  }

  // get child row count of index
  int rowCount(const QModelIndex &parent=QModelIndex()) const override {
    QAbstractItemModel *model = this->sourceModel();

    return model->rowCount(mapToSource(parent));
  }

  // get child of parent at row/column
  QModelIndex index(int row, int column, const QModelIndex &parent=QModelIndex()) const override {
    if (parent.isValid()) return QModelIndex();

    return createIndex(row, column);
  }

  // get parent of child
  QModelIndex parent(const QModelIndex &child) const override {
    if (! child.isValid()) return QModelIndex();

    return QModelIndex();
  }

  // does parent have children
  bool hasChildren(const QModelIndex &parent=QModelIndex()) const override {
    if (! parent.isValid()) return true;

    return false;
  }

  // get role data for index
  QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const override {
    QAbstractItemModel *model = this->sourceModel();

    return model->data(mapToSource(index), role);
  }

  // get header data for column/section
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role=Qt::DisplayRole) const override {
    QAbstractItemModel *model = this->sourceModel();

    return model->headerData(section, orientation, role);
  }

  // get flags for index
  Qt::ItemFlags flags(const QModelIndex &index) const override {
    QAbstractItemModel *model = this->sourceModel();

    return model->flags(mapToSource(index));
  }

  //---

  void sort(int column, Qt::SortOrder order=Qt::AscendingOrder) override {
    emit layoutAboutToBeChanged();

    typedef QPair<QString, int> ValueRow;
    typedef QVector<ValueRow>   ValueRowList;

    ValueRowList list;

    int nr = rowCount();

    list.reserve(nr);

    for (int r = 0; r < nr; ++r) {
      QString str = data(index(r, column)).toString();

      list.append(ValueRow(str, r));
    }

    if (order == Qt::AscendingOrder)
      std::sort(list.begin(), list.end(), ascendingLessThan);
    else
      std::sort(list.begin(), list.end(), decendingLessThan);

    QVector<int> forwarding(nr);

    for (int r = 0; r < nr; ++r) {
      forwarding[list.at(r).second] = r;
    }

    QModelIndexList oldList = persistentIndexList();
    QModelIndexList newList;

    int numOld = oldList.count();

    newList.reserve(numOld);

    for (int i = 0; i < numOld; ++i) {
      int oldRow = oldList.at(i).row();
      int newRow = forwarding.at(oldRow);

      newList.append(index(newRow, 0));
    }

    changePersistentIndexList(oldList, newList);

    emit layoutChanged();
  }

  //---

  // # Abstarct Proxy Model APIS

  // map source index to proxy index
  QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override {
    if (! sourceIndex.isValid())
      return QModelIndex();

    int r = sourceIndex.row   ();
    int c = sourceIndex.column();

    return this->index(r, c);
  }

  // map proxy index to source index
  QModelIndex mapToSource(const QModelIndex &proxyIndex) const override {
    if (! proxyIndex.isValid())
      return QModelIndex();

    int r = proxyIndex.row   ();
    int c = proxyIndex.column();

    QAbstractItemModel *model = this->sourceModel();

    return model->index(r, c);
  }
};

//-----

CQDataModelTest::
CQDataModelTest()
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  table_ = CQUtil::makeWidget<CQTableView>("table");

  table_->setSortingEnabled(true);

  QHeaderView *header = table_->horizontalHeader();

  header->setSectionsClickable(true);
  header->setHighlightSections(true);
  header->setSortIndicatorShown(true);

  layout->addWidget(table_);

  //---

  int nr = 5;
  int nc = 5;

  model_ = new CQDataModel(5, 5);

  for (int r = 0; r < nr; ++r) {
    for (int c = 0; c < nc; ++c) {
      QString data = QString("%1:%2").arg(r).arg(c);

      QModelIndex ind = model_->index(r, c);

      model_->setData(ind, data);
    }
  }

  //---

  proxy_ = new CQProxyModel(model_);

  table_->setModel(proxy_);
}

CQDataModelTest::
~CQDataModelTest()
{
  delete model_;
}
