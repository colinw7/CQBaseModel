#include <CQSelDelegate.h>

CQSelDelegate::
CQSelDelegate(CQSelView *view) :
 view_(view)
{
}

void
CQSelDelegate::
paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if (index.column() == 0) {
    bool checked = index.model()->data(index, Qt::DisplayRole).toBool();

    auto checkState = (checked ? Qt::Checked : Qt::Unchecked);

    QItemDelegate::drawCheck(painter, option, option.rect, checkState);
  }
  else
    QItemDelegate::paint(painter, option, index);
}
