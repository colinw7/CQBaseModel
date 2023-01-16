#include <CQDataModel.h>
#include <CQModelDetails.h>
#include <CQModelUtil.h>
#include <iostream>

CQDataModel::
CQDataModel(QObject *parent) :
 CQBaseModel(parent)
{
  init();
}

CQDataModel::
CQDataModel(QObject *parent, int numCols, int numRows) :
 CQBaseModel(parent)
{
  init(numCols, numRows);
}

CQDataModel::
CQDataModel(int numCols, int numRows) :
 CQBaseModel(nullptr)
{
  init(numCols, numRows);
}

CQDataModel::
~CQDataModel()
{
  delete details_;
}

void
CQDataModel::
init(int numCols, int numRows)
{
  setObjectName("dataModel");

  if (numCols > 0 && numRows > 0)
    init1(size_t(numCols), size_t(numRows));

  connect(this, SIGNAL(columnTypeChanged(int)), this, SLOT(resetColumnCache(int)));
}

void
CQDataModel::
init1(size_t numCols, size_t numRows)
{
  hheader_.resize(numCols);
  vheader_.resize(numRows);

  data_.resize(numRows);

  for (size_t i = 0; i < numRows; ++i)
    data_[i].resize(numCols);

  clearCachedColumn();
}

void
CQDataModel::
copyModel(CQDataModel *model)
{
  beginResetModel();

  // copy data
  hheader_ = model->hheader_;
  vheader_ = model->vheader_;
  data_    = model->data_;

  CQBaseModel::copyModel(model);

  clearCachedColumn();

  endResetModel();
}

void
CQDataModel::
resizeModel(int numCols, int numRows)
{
  beginResetModel();

  if (numCols > 0 && numRows > 0)
    init1(size_t(numCols), size_t(numRows));

  endResetModel();
}

void
CQDataModel::
addRow(int n)
{
  beginResetModel();

  if (! vheader_.empty())
    vheader_.push_back("");

  for (int i = 0; i < n; ++i) {
    Cells row;

    row.resize(size_t(columnCount()));

    data_.push_back(row);
  }

  clearCachedColumn();

  endResetModel();
}

void
CQDataModel::
addColumn(int n)
{
  beginResetModel();

  auto nr = rowCount();

  hheader_.push_back("");

  for (size_t ir = 0; ir < size_t(nr); ++ir) {
    auto &row = data_[ir];

    for (int i = 0; i < n; ++i)
      row.push_back("");
  }

  clearCachedColumn();

  endResetModel();
}

//------

void
CQDataModel::
initFilter()
{
  setFilterInited(true);

  //---

  filterDatas_.clear();

  if (! hasFilter())
    return;

  //---

  auto numHeaders = hheader_.size();

  auto patterns = filter_.split(",");

  for (int i = 0; i < patterns.size(); ++i) {
    FilterData filterData;

    auto fields = patterns[i].split(":");

    if (fields.length() == 2) {
      auto name  = fields[0];
      auto value = fields[1];

      filterData.column = -1;

      for (size_t j = 0; j < numHeaders; ++j) {
        if (hheader_[j] == name) {
          filterData.column = int(j);
          break;
        }
      }

      if (filterData.column == -1) {
        bool ok;

        filterData.column = name.toInt(&ok);

        if (! ok)
          filterData.column = -1;
      }

      filterData.regexp = QRegExp(value, Qt::CaseSensitive, QRegExp::Wildcard);
    }
    else {
      filterData.column = 0;
      filterData.regexp = QRegExp(patterns[i], Qt::CaseSensitive, QRegExp::Wildcard);
    }

    filterData.valid = (filterData.column >= 0 && filterData.column < int(numHeaders));

    filterDatas_.push_back(filterData);
  }
}

bool
CQDataModel::
acceptsRow(const Cells &cells) const
{
  if (! hasFilter())
    return true;

  //---

  if (! isFilterInited()) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (! isFilterInited()) {
      auto *th = const_cast<CQDataModel *>(this);

      th->initFilter();
    }
  }

  //---

  for (std::size_t i = 0; i < filterDatas_.size(); ++i) {
    const FilterData &filterData = filterDatas_[i];

    if (! filterData.valid)
      continue;

    auto field = cells[size_t(filterData.column)].toString();

    if (! filterData.regexp.exactMatch(field))
      return false;
  }

  return true;
}

//------

int
CQDataModel::
columnCount(const QModelIndex &) const
{
  return int(hheader_.size());
}

int
CQDataModel::
rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  return int(data_.size());
}

QVariant
CQDataModel::
headerData(int section, Qt::Orientation orientation, int role) const
{
  if (section < 0)
    return QVariant();

  auto section1 = size_t(section);

  if (orientation == Qt::Horizontal) {
    if (hheader_.empty())
      return CQBaseModel::headerData(section, orientation, role);

    int numCols = columnCount();

    if (section < 0 || section >= numCols)
      return QVariant();

    if      (role == Qt::DisplayRole) {
      if (hheader_[section1].toString().length())
        return hheader_[section1];

      return CQBaseModel::headerData(section, orientation, role);
    }
    else if (role == Qt::EditRole) {
      if (hheader_[section1].toString().length())
        return hheader_[section1];

      return CQBaseModel::headerData(section, orientation, role);
    }
    else if (role == Qt::ToolTipRole) {
      auto var = hheader_[section1];

      auto type = columnType(section);

      auto str = var.toString() + ":" + typeName(type);

      return QVariant(str);
    }
    else if (role == static_cast<int>(CQBaseModelRole::DataMin)) {
      auto *details = getDetails();

      return details->columnDetails(section)->minValue();
    }
    else if (role == static_cast<int>(CQBaseModelRole::DataMax)) {
      auto *details = getDetails();

      return details->columnDetails(section)->maxValue();
    }
    else {
      return CQBaseModel::headerData(section, orientation, role);
    }
  }
  else {
    if (vheader_.empty())
      return CQBaseModel::headerData(section, orientation, role);

    int numRows = rowCount();

    if (section < 0 || section >= numRows)
      return QVariant();

    if      (role == Qt::DisplayRole) {
      if (vheader_[section1].toString().length())
        return vheader_[section1];
      else
        return CQBaseModel::headerData(section, orientation, role);
    }
    else if (role == Qt::EditRole) {
      if (vheader_[section1].toString().length())
        return vheader_[section1];
      else
        return CQBaseModel::headerData(section, orientation, role);
    }
    else if (role == Qt::ToolTipRole) {
      return vheader_[section1];
    }
    else {
      return CQBaseModel::headerData(section, orientation, role);
    }
  }
}

bool
CQDataModel::
setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
  if (section < 0)
    return false;

  auto section1 = size_t(section);

  if (orientation == Qt::Horizontal) {
    if (hheader_.empty())
      return CQBaseModel::setHeaderData(section, orientation, value, role);

    int numCols = columnCount();

    if (section < 0 || section >= numCols)
      return false;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      hheader_[section1] = value;

      Q_EMIT headerDataChanged(orientation, section, section);

      return true;
    }
    else {
      return CQBaseModel::setHeaderData(section, orientation, value, role);
    }
  }
  else {
    if (vheader_.empty())
      return CQBaseModel::setHeaderData(section, orientation, value, role);

    int numRows = rowCount();

    if (section < 0 || section >= numRows)
      return false;

    if (role == Qt::DisplayRole) {
      vheader_[section1] = value;

      Q_EMIT headerDataChanged(orientation, section, section);

      return true;
    }
    else {
      return CQBaseModel::setHeaderData(section, orientation, value, role);
    }
  }
}

QVariant
CQDataModel::
data(const QModelIndex &index, int role) const
{
  if (! index.isValid())
    return QVariant();

  int r = index.row();
  int c = index.column();

  auto nr = data_.size();

  if (r < 0 || size_t(r) >= nr)
    return QVariant();

  const auto &cells = data_[size_t(r)];

  auto nc = cells.size();

  if (c < 0 || size_t(c) >= nc)
    return QVariant();

  //---

  const auto &columnData = getColumnData(c);

  //---

  auto getRowRoleValue = [&](int row, int role, QVariant &value) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto pr = columnData.roleRowValues.find(role);
    if (pr == columnData.roleRowValues.end()) return false;

    const RowValues &rowValues = (*pr).second;

    auto pr1 = rowValues.find(row);
    if (pr1 == rowValues.end()) return false;

    value = (*pr1).second;

    return true;
  };

  auto setRowRoleValue = [&](int row, int role, const QVariant &value) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto &columnData1 = const_cast<ColumnData &>(columnData);

    columnData1.roleRowValues[role][row] = value;
  };

  using CQModelUtil::roleCast;

  //---

  if      (role == Qt::DisplayRole) {
    return cells[size_t(c)];
  }
  else if (role == Qt::EditRole) {
    auto type = columnType(c);

    // check in cached values
    QVariant var;

    if (getRowRoleValue(r, roleCast(CQBaseModelRole::CachedValue), var)) {
      if (type == CQBaseModelType::NONE || isSameType(var, type))
        return var;
    }

    // not cached so get raw value
    var = cells[size_t(c)];

    // column has no type or already correct type then just return
    if (type == CQBaseModelType::NONE || isSameType(var, type))
      return var;

    // cache converted value
    if (var.type() == QVariant::String) {
      auto var1 = typeStringToVariant(var.toString(), type);

      if (var1.isValid())
        setRowRoleValue(r, roleCast(CQBaseModelRole::CachedValue), var1);

      return var1;
    }

    return var;
  }
  else if (role == Qt::ToolTipRole) {
    return cells[size_t(c)];
  }
  else if (role == roleCast(CQBaseModelRole::RawValue) ||
           role == roleCast(CQBaseModelRole::IntermediateValue) ||
           role == roleCast(CQBaseModelRole::CachedValue) ||
           role == roleCast(CQBaseModelRole::OutputValue)) {
    QVariant var;

    if (getRowRoleValue(r, role, var))
      return var;

    if (role == roleCast(CQBaseModelRole::RawValue)) {
      return cells[size_t(c)];
    }

    return QVariant();
  }
  else {
    QVariant var;

    if (getRowRoleValue(r, role, var))
      return var;
  }

  return CQBaseModel::data(index, role);
}

bool
CQDataModel::
setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (isReadOnly()) {
    std::cerr << "CQDataModel::setData for read only model\n";
    return false;
  }

  if (! index.isValid())
    return false;

  int r = index.row();
  int c = index.column();

  bool changed;

  if (! setModelData(r, c, value, role, &changed))
    return false;

  if (changed)
    Q_EMIT dataChanged(index, index, QVector<int>(1, role));

  return true;
}

bool
CQDataModel::
setModelData(int r, int c, const QVariant &value, int role, bool *changed)
{
  if (changed)
    *changed = true;

  auto nr = data_.size();

  if (r < 0 || size_t(r) >= nr)
    return false;

  auto &cells = data_[size_t(r)];

//auto nc = cells.size();
  auto nc = columnCount();

  if (c < 0 || c >= nc)
    return false;

  //---

  clearCachedColumn();

  //---

  auto &columnData = getColumnData(c);

  //---

  auto clearRowRoleValue = [&](int row, int role) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto pr = columnData.roleRowValues.find(role);
    if (pr == columnData.roleRowValues.end()) return false;

    RowValues &rowValues = (*pr).second;

    auto pr1 = rowValues.find(row);
    if (pr1 == rowValues.end()) return false;

    rowValues.erase(pr1);

    return true;
  };

  auto setRowRoleValue = [&](int row, int role, const QVariant &value) {
    std::unique_lock<std::mutex> lock(mutex_);

    columnData.roleRowValues[role][row] = value;
  };

  using CQModelUtil::roleCast;

  //---

  if      (role == Qt::DisplayRole) {
    //auto type = columnType(c);

    cells[size_t(c)] = value;
  }
  else if (role == Qt::EditRole) {
    //auto type = columnType(c);

    while (size_t(c) >= cells.size())
      cells.push_back(QVariant());

    cells[size_t(c)] = value;

    clearRowRoleValue(r, roleCast(CQBaseModelRole::RawValue));
    clearRowRoleValue(r, roleCast(CQBaseModelRole::IntermediateValue));
    clearRowRoleValue(r, roleCast(CQBaseModelRole::CachedValue));
    clearRowRoleValue(r, roleCast(CQBaseModelRole::OutputValue));
  }
  else if (role == roleCast(CQBaseModelRole::RawValue) ||
           role == roleCast(CQBaseModelRole::IntermediateValue) ||
           role == roleCast(CQBaseModelRole::CachedValue) ||
           role == roleCast(CQBaseModelRole::OutputValue)) {
    setRowRoleValue(r, role, value);

    if (changed)
      *changed = false;
  }
  else {
    setRowRoleValue(r, role, value);
  }

  return true;
}

//------

const CQBaseModel::RoleDatas &
CQDataModel::
roleDatas() const
{
  static RoleDatas s_roleDatas;

  //---

  using Standard = CQBaseModel::Standard;
  using Writable = CQBaseModel::Writable;

  auto addRole = [&](const QString &name, int role, const QVariant::Type &type,
                     const Writable &writable=Writable()) {
    s_roleDatas.emplace_back(name, role, type, Standard(), writable);
  };

  using CQModelUtil::roleCast;

  //---

  addRole("raw_value"         , roleCast(CQBaseModelRole::RawValue         ),
          QVariant::Invalid, Writable());
  addRole("intermediate_value", roleCast(CQBaseModelRole::IntermediateValue),
          QVariant::Invalid, Writable());
  addRole("cached_value"      , roleCast(CQBaseModelRole::CachedValue      ),
          QVariant::Invalid, Writable());
  addRole("output_value"      , roleCast(CQBaseModelRole::OutputValue      ),
          QVariant::Invalid, Writable());

  return s_roleDatas;
}

//------

QModelIndex
CQDataModel::
index(int row, int column, const QModelIndex &) const
{
  return createIndex(row, column, nullptr);
}

QModelIndex
CQDataModel::
parent(const QModelIndex &index) const
{
  if (! index.isValid())
    return QModelIndex();

  return QModelIndex();
}

Qt::ItemFlags
CQDataModel::
flags(const QModelIndex &index) const
{
  if (! index.isValid())
    return Qt::ItemFlags();

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (! isReadOnly())
    flags |= Qt::ItemIsEditable;

  return flags;
}

void
CQDataModel::
resetColumnCache(int column)
{
  std::unique_lock<std::mutex> lock(mutex_);

  auto &columnData = getColumnData(column);

  columnData.roleRowValues.clear();
}

CQModelDetails *
CQDataModel::
getDetails() const
{
  if (! details_) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (! details_) {
      auto *th = const_cast<CQDataModel *>(this);

      th->details_ = new CQModelDetails(th);
    }
  }

  return details_;
}

//------

void
CQDataModel::
applyFilterColumns(const QStringList &columns)
{
  if (! columns.length())
    return;

  auto data    = data_;
  auto hheader = hheader_;

  std::map<int, int> columnMap;

  int nc1 = int(hheader_.size());
  int nc2 = columns.length();

  for (int c = 0; c < nc1; ++c)
    columnMap[c] = -1;

  for (int c = 0; c < nc2; ++c) {
    const auto &name = columns[c];

    // get index for matching column name
    int ind = -1;

    for (int c1 = 0; c1 < nc1; ++c1) {
      if (hheader[size_t(c1)] == name) {
        ind = c1;
        break;
      }
    }

    // if name not found, try and convert column name to number
    if (ind == -1) {
      bool ok;

      int ind1 = name.toInt(&ok);

      if (ok && ind1 >= 0 && ind1 < nc1)
        ind = ind1;
    }

    if (ind == -1) {
      std::cerr << "Invalid column name '" << name.toStdString() << "'\n";
      continue;
    }

    columnMap[ind] = c;
  }

  // get new number of columns
  nc2 = 0;

  for (int c = 0; c < nc1; ++c) {
    int c1 = columnMap[c];

    if (c1 >= 0 && c1 < nc1)
      ++nc2;
  }

  // remap horizontal header and row data
  hheader_.clear(); hheader_.resize(size_t(nc2));

  auto nr = data.size();

  for (size_t r = 0; r < nr; ++r) {
    auto &cells1 = data [r]; // old data
    auto &cells2 = data_[r]; // new data

    cells2.clear(); cells2.resize(size_t(nc2));

    for (int c = 0; c < nc1; ++c) {
      int c1 = columnMap[c];

      if (c1 < 0 || c1 >= nc1)
        continue;

      hheader_[size_t(c1)] = hheader[size_t(c)];
      cells2  [size_t(c1)] = cells1 [size_t(c)];
    }
  }
}

//---

int
CQDataModel::
findColumnValue(int column, const QVariant &var) const
{
  updateColumnValues(column);

  int nr = cachedColumnVars_.size();

  for (int r = 0; r < nr; ++r) {
    if (cachedColumnVars_[r] == var)
      return r;
  }

  return -1;
}

void
CQDataModel::
getColumnValues(int column, QVariantList &vars) const
{
  updateColumnValues(column);

  vars = cachedColumnVars_;
}

void
CQDataModel::
updateColumnValues(int column) const
{
  if (column != cachedColumn_) {
    cachedColumn_ = column;

    cachedColumnVars_.clear();

    int nr = rowCount();

    for (int r = 0; r < nr; ++r) {
      auto ind = index(r, column, QModelIndex());

      auto var = data(ind, Qt::DisplayRole);

      cachedColumnVars_.push_back(var);
    }
  }
}

void
CQDataModel::
clearCachedColumn()
{
  cachedColumn_ = -1;
  cachedColumnVars_.clear();
}
