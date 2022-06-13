#include <CQSelView.h>
#include <CQSelModel.h>
#include <CQSelDelegate.h>

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <set>

CQSelView::
CQSelView(QWidget *parent) :
 QTableView(parent)
{
  setObjectName("selView");

  setSortingEnabled(true);

  auto *hheader = this->horizontalHeader();
  auto *vheader = this->verticalHeader();

  hheader->setSectionsClickable(true);
  hheader->setHighlightSections(true);
  hheader->setSortIndicatorShown(true);
  hheader->setMinimumSectionSize(4);

  vheader->setVisible(false);

  setItemDelegate(new CQSelDelegate(this));

  setAlternatingRowColors(true);
}

void
CQSelView::
setModel(QAbstractItemModel *model)
{
  delete selModel_;
  delete sortModel_;

  if (model) {
    selModel_ = new CQSelModel(this);
    selModel_->setSourceModel(model);

    sortModel_ = new QSortFilterProxyModel;
    sortModel_->setSourceModel(selModel_);

    QTableView::setModel(sortModel_);

    auto *sm = selectionModel();
    if (! sm) return;

    auto *hheader = this->horizontalHeader();

    QFontMetrics fm(font());
    hheader->resizeSection(0, fm.height() + 16);
    hheader->setSectionResizeMode(0, QHeaderView::Fixed);

    connect(sm, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            this, SLOT(updateModelFromSelection()));
  }
  else {
    selModel_  = nullptr;
    sortModel_ = nullptr;
  }
}

void
CQSelView::
updateModelFromSelection()
{
  auto *sm = selectionModel();
  if (! sm) return;

  //---

  disconnect(sm, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
             this, SLOT(updateModelFromSelection()));

  //---

  auto inds = sm->selectedIndexes();

  std::set<int> rows;

  for (const auto &ind : inds) {
    auto ind1 = sortModel_->mapToSource(ind);

    rows.insert(ind1.row());
  }

  for (int r = 0; r < selModel_->rowCount(); ++r) {
    bool sel = (rows.find(r) != rows.end());

    auto ind = selModel_->index(r, 0, QModelIndex());

    selModel_->setData(ind, QVariant(sel), Qt::DisplayRole);
  }

  //---

  connect(sm, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(updateModelFromSelection()));

  //---

  update();
  viewport()->update();
}
