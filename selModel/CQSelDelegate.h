#ifndef CQSelDelegate_H
#define CQSelDelegate_H

#include <QItemDelegate>

class CQSelView;

class CQSelDelegate : public QItemDelegate {
 public:
  CQSelDelegate(CQSelView *view);

  CQSelView *view() const { return view_; }

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

 private:
  CQSelView *view_ { nullptr };
};

#endif
