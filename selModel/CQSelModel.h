#ifndef CQSelModel_H
#define CQSelModel_H

#include <QAbstractItemModel>
#include <QIcon>

class CQSelView;

/*!
 * \brief add extra selection state column to model
 */
class CQSelModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  CQSelModel(CQSelView *view);
 ~CQSelModel();

  //---

  // get/set source model
  QAbstractItemModel *sourceModel() const;
  void setSourceModel(QAbstractItemModel *sourceModel);

  //---

  // # Abstract Model APIS

  //! get column count
  int columnCount(const QModelIndex &parent=QModelIndex()) const override;

  //! get child row count of index
  int rowCount(const QModelIndex &parent=QModelIndex()) const override;

  //! get child of parent at row/column
  QModelIndex index(int row, int column, const QModelIndex &parent=QModelIndex()) const override;

  //! get parent of child
  QModelIndex parent(const QModelIndex &child) const override;

  //! does parent have children
  bool hasChildren(const QModelIndex &parent=QModelIndex()) const override;

  //! get role data for index
  QVariant data(const QModelIndex &index, int role) const override;

  //! set role data for index
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  //! get header data for column/section
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  //! set header data for column/section
  bool setHeaderData(int section, Qt::Orientation orientation,
                     const QVariant &value, int role) override;

  //! get flags for index
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  //---

  // # Abstract Proxy Model APIS

  //! map source index to proxy index
  QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
  //! map proxy index to source index
  QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

  //---

  bool isSelected (const QModelIndex &ind) const;
  void setSelected(const QModelIndex &ind, bool selected);

  //---

  void reset();

 private:
  using Selected = std::map<QModelIndex, bool>;

  CQSelView*          view_        { nullptr };
  QAbstractItemModel* sourceModel_ { nullptr };
  Selected            selected_;
  QIcon               checkedIcon_;
  QIcon               uncheckedIcon_;
  QIcon               partCheckedIcon_;
};

#endif
