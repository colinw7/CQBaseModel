#include <CQSelModel.h>
#include <CQSelView.h>
#include <CQPixmapCache.h>

#include <QFontMetrics>

#include <cassert>

#include <svg/checked_svg.h>
#include <svg/unchecked_svg.h>
#include <svg/part_checked_svg.h>

//------

CQSelModel::
CQSelModel(CQSelView *view) :
 QAbstractItemModel(), view_(view)
{
  setObjectName("subsetModel");

  checkedIcon_     = CQPixmapCacheInst->getIcon("CHECKED");
  uncheckedIcon_   = CQPixmapCacheInst->getIcon("UNCHECKED");
  partCheckedIcon_ = CQPixmapCacheInst->getIcon("PART_CHECKED");
}

CQSelModel::
~CQSelModel()
{
}

QAbstractItemModel *
CQSelModel::
sourceModel() const
{
  return sourceModel_;
}

void
CQSelModel::
setSourceModel(QAbstractItemModel *sourceModel)
{
  sourceModel_ = sourceModel;
}

//------

// get number of columns
int
CQSelModel::
columnCount(const QModelIndex &) const
{
  auto *model = this->sourceModel();
  if (! model) return 0;

  //return model->columnCount(mapToSource(parent)) + 1;
  return model->columnCount(QModelIndex()) + 1;
}

// get number of child rows for parent
int
CQSelModel::
rowCount(const QModelIndex &parent) const
{
  auto *model = this->sourceModel();
  if (! model) return 0;

  if (parent.isValid())
    return 0;

  return model->rowCount(QModelIndex());
}

// get child node for row/column of parent
QModelIndex
CQSelModel::
index(int row, int column, const QModelIndex &parent) const
{
  if (row < 0 || row >= rowCount(parent))
    return QModelIndex();

  return createIndex(row, column, nullptr);
}

// get parent for child
QModelIndex
CQSelModel::
parent(const QModelIndex &) const
{
  // flat - no parent
  return QModelIndex();
}

bool
CQSelModel::
hasChildren(const QModelIndex &parent) const
{
  if (! parent.isValid())
    return true;

  // flat - no children
  return false;
}

QVariant
CQSelModel::
data(const QModelIndex &index, int role) const
{
  auto *model = this->sourceModel();
  if (! model) return QVariant();

  int c = index.column();

  if (c < 0 || c >= columnCount())
    return QVariant();

  if (c == 0) {
    if (role == Qt::DisplayRole || role == Qt::EditRole)
      return isSelected(index);
    else
      return QVariant();
  }

  return model->data(mapToSource(index), role);
}

bool
CQSelModel::
setData(const QModelIndex &index, const QVariant &value, int role)
{
  auto *model = this->sourceModel();
  if (! model) return false;

  int c = index.column();

  if (c < 0 || c >= columnCount())
    return false;

  //---

  if (c == 0) {
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      setSelected(index, value.toBool());
      return true;
    }

    return false;
  }

  return model->setData(mapToSource(index), value, role);
}

QVariant
CQSelModel::
headerData(int section, Qt::Orientation orientation, int role) const
{
  auto *model = this->sourceModel();
  if (! model) return QVariant();

  if (orientation == Qt::Vertical)
    return model->headerData(section, orientation, role);

  if (section < 0 || section >= columnCount())
    return QVariant();

  //---

  if (section == 0) {
    if      (role == Qt::DisplayRole) {
      return "";
    }
    else if (role == Qt::DecorationRole) {
      int numSelected { 0 };

      for (const auto &ps : selected_)
        if (ps.second)
          ++numSelected;

      if      (numSelected == 0)
        return uncheckedIcon_;
      else if (numSelected == rowCount())
        return checkedIcon_;
      else
        return partCheckedIcon_;
    }
    else if (role == Qt::SizeHintRole) {
      QFontMetrics fm(view_->font());

      int s = fm.height() + 4;

      return QSize(s, s);
    }
    else
      return QVariant();
  }
  else {
    return model->headerData(section - 1, orientation, role);
  }
}

bool
CQSelModel::
setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
  auto *model = this->sourceModel();
  if (! model) return false;

  if (orientation == Qt::Vertical)
    return model->setHeaderData(section, orientation, role);

  if (section < 0 || section >= columnCount())
    return false;

  //---

  if (section == 0) {
    return false;
  }
  else {
    return model->setHeaderData(section - 1, orientation, value, role);
  }
}

Qt::ItemFlags
CQSelModel::
flags(const QModelIndex &index) const
{
  auto *model = this->sourceModel();
  if (! model) return Qt::ItemFlags();

  int c = index.column();

  if (c < 0 || c >= columnCount())
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (c == 0)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  return model->flags(mapToSource(index));
}

// map index in source model to this model
QModelIndex
CQSelModel::
mapFromSource(const QModelIndex &sourceIndex) const
{
  if (! sourceIndex.isValid())
    return QModelIndex();

  auto *model = this->sourceModel();
  if (! model) return QModelIndex();

  int r = sourceIndex.row   ();
  int c = sourceIndex.column();

  if (r < 0 || r >= model->rowCount())
    return QModelIndex();

  if (c < 0 || c >= model->columnCount())
    return QModelIndex();

  return createIndex(r, c + 1, nullptr);
}

// map index in this model to source model
QModelIndex
CQSelModel::
mapToSource(const QModelIndex &proxyIndex) const
{
  if (! proxyIndex.isValid())
    return QModelIndex();

  int r = proxyIndex.row   ();
  int c = proxyIndex.column();

  if (r < 0 || r >= rowCount())
    return QModelIndex();

  if (c < 0 || c >= columnCount())
    return QModelIndex();

  //---

  auto *model = this->sourceModel();
  if (! model) return QModelIndex();

  if (c <= 0)
    return QModelIndex();

  return model->index(r, c - 1, QModelIndex());
}

//---

bool
CQSelModel::
isSelected(const QModelIndex &ind) const
{
  auto p = selected_.find(ind);

  if (p != selected_.end())
    return (*p).second;

  return false;
}

void
CQSelModel::
setSelected(const QModelIndex &ind, bool selected)
{
  selected_[ind] = selected;
}

//---

void
CQSelModel::
reset()
{
  beginResetModel();
  endResetModel();
}
