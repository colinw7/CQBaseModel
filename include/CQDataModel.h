#ifndef CQDataModel_H
#define CQDataModel_H

#include <CQBaseModel.h>
#include <QRegExp>
#include <vector>

class CQModelDetails;

/*!
 * \brief model derived from base model which supports a 2d array of variant values
 *
 * Can be made writable to update values.
 */
class CQDataModel : public CQBaseModel {
  Q_OBJECT

  Q_PROPERTY(bool    readOnly READ isReadOnly WRITE setReadOnly)
  Q_PROPERTY(QString filter   READ filter     WRITE setFilter  )
  Q_PROPERTY(QString filename READ filename   WRITE setFilename)

 public:
  using Cells = std::vector<QVariant>;

 public:
  CQDataModel(QObject *parent=nullptr);
  CQDataModel(QObject *parent, int numCols, int numRows);
  CQDataModel(int numCols, int numRows);

  virtual ~CQDataModel();

  //---

  void copyModel(CQDataModel *model);

  //---

  //! resize
  virtual void resizeModel(int numCols, int numRows);

  //! add new rows at end
  virtual void addRow(int n=1);

  //! add new columns to right
  virtual void addColumn(int n=1);

  //---

  //! get/set read only
  bool isReadOnly() const { return readOnly_; }
  void setReadOnly(bool b) { readOnly_ = b; }

  //---

  //! get/set filter
  const QString &filter() const { return filter_; }
  void setFilter(const QString &filter) { filter_ = filter; setFilterInited(false); }

  virtual bool hasFilter() const { return filter_.length(); }

  //--

  //! get/set filename
  const QString &filename() const { return filename_; }
  void setFilename(const QString &v) { filename_ = v; }

  //--

  // model interface
  int columnCount(const QModelIndex &parent=QModelIndex()) const override;

  int rowCount(const QModelIndex &parent=QModelIndex()) const override;

  QVariant headerData(int section, Qt::Orientation orientation=Qt::Horizontal,
                      int role=Qt::DisplayRole) const override;

  bool setHeaderData(int section, Qt::Orientation orientation,
                     const QVariant &value, int role=Qt::DisplayRole) override;

  QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const override;

  bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::DisplayRole) override;

  QModelIndex index(int row, int column, const QModelIndex &parent=QModelIndex()) const override;

  QModelIndex parent(const QModelIndex &index) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;

  //---

  bool setModelData(int r, int c, const QVariant &value,
                    int role=Qt::DisplayRole, bool *changed=nullptr);

  //---

  const RoleDatas &roleDatas() const override;

  //---

  virtual bool acceptsRow(const Cells &cells) const;

  //---

  int findColumnValue(int column, const QVariant &var) const;

  void getColumnValues(int column, QVariantList &vars) const;

 protected slots:
  void resetColumnCache(int column);

 protected:
  using Data  = std::vector<Cells>;

 protected:
  void init(int numCols=0, int numRows=0);
  void init1(size_t numCols, size_t numRows);

  //---

  virtual void initFilter();

  virtual bool isFilterInited() const { return filterInited_; }
  virtual void setFilterInited(bool b) { filterInited_ = b; }

  //---

  CQModelDetails *getDetails() const;

  //---

  void applyFilterColumns(const QStringList &columns);

  //---

  void clearCachedColumn();

  void updateColumnValues(int column) const;

 protected:
  struct FilterData {
    int     column { -1 };
    QRegExp regexp;
    bool    valid  { false };
  };

  using FilterDatas = std::vector<FilterData>;

  bool readOnly_ { false }; //!< is read only

  QString filename_; //!< input filename

  Cells hheader_; //!< horizontal header values
  Cells vheader_; //!< vertical header values
  Data  data_;    //!< row values

  QString     filter_;                 //!< filter text
  bool        filterInited_ { false }; //!< filter initialized
  FilterDatas filterDatas_;            //!< filter datas

  CQModelDetails* details_ { nullptr }; //!< model details

  mutable int          cachedColumn_ { -1 };
  mutable QVariantList cachedColumnVars_;
};

#endif
