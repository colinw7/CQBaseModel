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

    int w = option.rect.width();
    int h = option.rect.height();

    int s = std::min(w, h);

    QRect rect1(option.rect.center() - QPoint(s/2, s/2), QSize(s, s));

    QItemDelegate::drawCheck(painter, option, rect1, checkState);
  }
  else
    QItemDelegate::paint(painter, option, index);
}
