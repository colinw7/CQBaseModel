#include <CQBaseModel.h>
#include <CQModelUtil.h>
#include <CQModelNameValues.h>
#include <CMathUtil.h>

#include <QApplication>
#include <QThread>

#include <cmath>
#include <cassert>

namespace {

using TypeName  = std::map<CQBaseModelType, QString>;
using NameType  = std::map<QString, CQBaseModelType>;
using AliasName = std::map<QString, QString>;

static TypeName  s_typeName;
static NameType  s_nameType;
static AliasName s_aliasName;

void addType(CQBaseModelType type, const QString &name) {
  s_typeName[type] = name;
  s_nameType[name] = type;
}

void addAlias(const QString &alias, const QString &name) {
  s_aliasName[alias] = name;
}

void initTypes() {
  // Note: all names in lower case
  if (s_typeName.empty()) {
    addType(CQBaseModelType::BOOLEAN        , "boolean"        );
    addType(CQBaseModelType::INTEGER        , "integer"        );
    addType(CQBaseModelType::REAL           , "real"           );
    addType(CQBaseModelType::STRING         , "string"         );
    addType(CQBaseModelType::STRINGS        , "string_list"    );
    addType(CQBaseModelType::POINT          , "point"          );
    addType(CQBaseModelType::LINE           , "line"           );
    addType(CQBaseModelType::RECT           , "rectangle"      );
    addType(CQBaseModelType::SIZE           , "size"           );
    addType(CQBaseModelType::POLYGON        , "polygon"        );
    addType(CQBaseModelType::POLYGON_LIST   , "polygon_list"   );
    addType(CQBaseModelType::COLOR          , "color"          );
    addType(CQBaseModelType::FONT           , "font"           );
    addType(CQBaseModelType::PEN            , "pen"            );
    addType(CQBaseModelType::BRUSH          , "brush"          );
    addType(CQBaseModelType::IMAGE          , "image"          );
    addType(CQBaseModelType::TIME           , "time"           );
    addType(CQBaseModelType::SYMBOL         , "symbol_type"    );
    addType(CQBaseModelType::SYMBOL_SIZE    , "symbol_size"    );
    addType(CQBaseModelType::FONT_SIZE      , "font_size"      );
    addType(CQBaseModelType::PATH           , "path"           );
    addType(CQBaseModelType::STYLE          , "style"          );
    addType(CQBaseModelType::CONNECTION_LIST, "connection_list");
    addType(CQBaseModelType::NAME_PAIR      , "name_pair"      );
    addType(CQBaseModelType::NAME_VALUES    , "name_values"    );
    addType(CQBaseModelType::COLUMN         , "column"         );
    addType(CQBaseModelType::COLUMN_LIST    , "column_list"    );
    addType(CQBaseModelType::ENUM           , "enum"           );
    addType(CQBaseModelType::LENGTH         , "length"         );

    addAlias("bool"       , "boolean"        );
    addAlias("int"        , "integer"        );
    addAlias("double"     , "real"           );
    addAlias("rect"       , "rectangle"      );
    addAlias("symbol"     , "symbol_type"    );
    addAlias("connections", "connection_list");
    addAlias("columns"    , "column_list"    );
  }
}

};

//---

CQBaseModel::
CQBaseModel(QObject *parent) :
 QAbstractItemModel(parent)
{
  setObjectName("baseModel");

  initTypes();
}

//---

void
CQBaseModel::
copyModel(CQBaseModel *model)
{
  beginResetModel();

  // data
  columnDatas_ = model->columnDatas_;
  rowDatas_    = model->rowDatas_;
  dataType_    = model->dataType_;

  metaNameValues_ = model->metaNameValues_;

  endResetModel();
}

//---

void
CQBaseModel::
genColumnTypes()
{
  resetColumnTypes();

  // auto determine type for each column. Do column by column to allow early out
  auto nc = columnCount();

  for (decltype(nc) column = 0; column < nc; ++column)
    genColumnType(column);
}

void
CQBaseModel::
genColumnType(int column)
{
  auto &columnData = getColumnData(column);

  genColumnType(columnData);

  Q_EMIT columnTypeChanged(columnData.column);
}

void
CQBaseModel::
genColumnType(const ColumnData &columnData) const
{
  if (columnData.type == CQBaseModelType::NONE) {
    std::unique_lock<std::mutex> lock(typeMutex_);

    if (columnData.type == CQBaseModelType::NONE) {
      auto *th = const_cast<CQBaseModel *>(this);

      th->genColumnTypeI(const_cast<ColumnData &>(columnData));
    }
  }
}

void
CQBaseModel::
genColumnTypeI(ColumnData &columnData)
{
  auto maxRows = maxTypeRows();

  if (maxRows <= 0)
    maxRows = 1000;

  //---

  columnData.type     = CQBaseModelType::STRING;
  columnData.baseType = CQBaseModelType::STRING;

  //---

  ColumnTypeData columnTypeData;

  columnTypeData.type = CQModelUtil::calcColumnType(this, columnData.column, maxRows);

  // if inderminate (no values or all reals or integers) then use real if any reals,
  // integer if any integers and string if no values.
  if (columnTypeData.type == CQBaseModelType::NONE) {
    if      (columnTypeData.numReal)
      columnTypeData.type = CQBaseModelType::REAL;
    else if (columnTypeData.numInt)
      columnTypeData.type = CQBaseModelType::INTEGER;
    else
      columnTypeData.type = CQBaseModelType::STRING;
  }

  columnTypeData.baseType = columnTypeData.type;

  if (columnTypeData.type != columnData.type) {
    columnData.type     = columnTypeData.type;
    columnData.baseType = columnTypeData.baseType;
  }
}

void
CQBaseModel::
setCurrentIndex(const QModelIndex &ind)
{
  currentIndex_ = ind;

  Q_EMIT currentIndexChanged(currentIndex_);
}

CQBaseModelType
CQBaseModel::
columnType(int column) const
{
  if (column < 0 || column >= columnCount())
    return CQBaseModelType::NONE;

  const auto &columnData = getColumnData(column);

  if (columnData.type == CQBaseModelType::NONE)
    genColumnType(columnData);

  return columnData.type;
}

bool
CQBaseModel::
setColumnType(int column, CQBaseModelType type)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  if (columnData.baseType == CQBaseModelType::NONE)
    genColumnType(columnData);

  if (type != columnData.type) {
    columnData.type = type;

    Q_EMIT columnTypeChanged(column);
  }

  return true;
}

CQBaseModelType
CQBaseModel::
columnBaseType(int column) const
{
  if (column < 0 || column >= columnCount())
    return CQBaseModelType::NONE;

  const auto &columnData = getColumnData(column);

  if (columnData.type == CQBaseModelType::NONE)
    genColumnType(columnData);

  return columnData.baseType;
}

bool
CQBaseModel::
setColumnBaseType(int column, CQBaseModelType type)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  if (type != columnData.baseType) {
    columnData.baseType = type;

    Q_EMIT columnBaseTypeChanged(column);
  }

  return true;
}

QString
CQBaseModel::
columnTypeValues(int column) const
{
  if (column < 0 || column >= columnCount())
    return QString();

  const auto &columnData = getColumnData(column);

  return columnData.typeValues;
}

bool
CQBaseModel::
setColumnTypeValues(int column, const QString &str)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.typeValues = str;

  Q_EMIT columnTypeChanged(column);

  return true;
}

QVariant
CQBaseModel::
columnMin(int column) const
{
  if (column < 0 || column >= columnCount())
    return QVariant();

  const auto &columnData = getColumnData(column);

  return columnData.min;
}

bool
CQBaseModel::
setColumnMin(int column, const QVariant &v)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.min = v;

  Q_EMIT columnRangeChanged(column);

  return true;
}

QVariant
CQBaseModel::
columnMax(int column) const
{
  if (column < 0 || column >= columnCount())
    return QVariant();

  const auto &columnData = getColumnData(column);

  return columnData.max;
}

bool
CQBaseModel::
setColumnMax(int column, const QVariant &v)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.max = v;

  Q_EMIT columnRangeChanged(column);

  return true;
}

QVariant
CQBaseModel::
columnSum(int column) const
{
  if (column < 0 || column >= columnCount())
    return QVariant();

  const auto &columnData = getColumnData(column);

  return columnData.sum;
}

bool
CQBaseModel::
setColumnSum(int column, const QVariant &v)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.sum = v;

  Q_EMIT columnRangeChanged(column);

  return true;
}

bool
CQBaseModel::
isColumnKey(int column) const
{
  if (column < 0 || column >= columnCount())
    return false;

  const auto &columnData = getColumnData(column);

  return columnData.key;
}

bool
CQBaseModel::
setColumnKey(int column, bool b)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.key = b;

  Q_EMIT columnKeyChanged(column);

  return true;
}

bool
CQBaseModel::
isColumnSorted(int column) const
{
  if (column < 0 || column >= columnCount())
    return false;

  const auto &columnData = getColumnData(column);

  return columnData.sorted;
}

bool
CQBaseModel::
setColumnSorted(int column, bool b)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.sorted = b;

  Q_EMIT columnSortedChanged(column);

  return true;
}

Qt::SortOrder
CQBaseModel::
columnSortOrder(int column) const
{
  if (column < 0 || column >= columnCount())
    return Qt::AscendingOrder;

  const auto &columnData = getColumnData(column);

  return columnData.sortOrder;
}

bool
CQBaseModel::
setColumnSortOrder(int column, Qt::SortOrder order)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.sortOrder = order;

  Q_EMIT columnSortOrderChanged(column);

  return true;
}

QString
CQBaseModel::
columnTitle(int column) const
{
  if (column < 0 || column >= columnCount())
    return "";

  const auto &columnData = getColumnData(column);

  return columnData.title;
}

bool
CQBaseModel::
setColumnTitle(int column, const QString &s)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.title = s;

  Q_EMIT columnTitleChanged(column);

  return true;
}

QString
CQBaseModel::
columnTip(int column) const
{
  if (column < 0 || column >= columnCount())
    return "";

  const auto &columnData = getColumnData(column);

  return columnData.tip;
}

bool
CQBaseModel::
setColumnTip(int column, const QString &s)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.tip = s;

  Q_EMIT columnTipChanged(column);

  return true;
}

CQBaseModelType
CQBaseModel::
columnHeaderType(int column) const
{
  if (column < 0 || column >= columnCount())
    return CQBaseModelType::NONE;

  const auto &columnData = getColumnData(column);

  return columnData.headerType;
}

bool
CQBaseModel::
setColumnHeaderType(int column, CQBaseModelType type)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  if (columnData.headerType != type) {
    columnData.headerType = type;

    Q_EMIT columnHeaderTypeChanged(column);
  }

  return true;
}

QString
CQBaseModel::
headerTypeValues(int column) const
{
  if (column < 0 || column >= columnCount())
    return QString();

  const auto &columnData = getColumnData(column);

  return columnData.headerTypeValues;
}

bool
CQBaseModel::
setHeaderTypeValues(int column, const QString &str)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto &columnData = getColumnData(column);

  columnData.headerTypeValues = str;

  Q_EMIT columnTypeChanged(column);

  return true;
}

QVariant
CQBaseModel::
columnNameValue(int column, const QString &name) const
{
  if (column < 0 || column >= columnCount())
    return "";

  auto values = columnTypeValues(column);

  CQModelNameValues nameValues(values);

  QVariant value;

  if (! nameValues.nameValue(name, value))
    return QVariant();

  return value;
}

bool
CQBaseModel::
setColumnNameValue(int column, const QString &name, const QVariant &value)
{
  if (column < 0 || column >= columnCount())
    return false;

  auto values = columnTypeValues(column);

  CQModelNameValues nameValues(values);

  nameValues.setNameValue(name, value);

  setColumnTypeValues(column, nameValues.toString());

  return true;
}

const CQBaseModel::ColumnData &
CQBaseModel::
getColumnData(int column) const
{
  return const_cast<CQBaseModel *>(this)->getColumnData(column);
}

CQBaseModel::ColumnData &
CQBaseModel::
getColumnData(int column)
{
  assert(column >= 0 || column < columnCount());

  auto p = columnDatas_.find(column);

  if (p != columnDatas_.end()) {
    auto &columnData = (*p).second;

    assert(columnData.column == column);

    return columnData;
  }

  //---

  std::unique_lock<std::mutex> lock(mutex_);

  auto *th = const_cast<CQBaseModel *>(this);

  auto p1 = th->columnDatas_.find(column);

  if (p1 == th->columnDatas_.end())
    p1 = th->columnDatas_.insert(p1, ColumnDatas::value_type(column, ColumnData(column)));

  return (*p1).second;
}

void
CQBaseModel::
resetColumnType(int column)
{
  auto p = columnDatas_.find(column);

  if (p != columnDatas_.end())
    columnDatas_.erase(p);
}

void
CQBaseModel::
resetColumnTypes()
{
  for (auto &p : columnDatas_) {
    auto &columnData = p.second;

    columnData.type = CQBaseModelType::NONE;
  }
}

//------

QVariant
CQBaseModel::
rowGroup(int row) const
{
  if (row < 0 || row >= rowCount())
    return QVariant();

  const RowData &rowData = getRowData(row);

  return rowData.group;
}

bool
CQBaseModel::
setRowGroup(int row, const QVariant &v)
{
  if (row < 0 || row >= rowCount())
    return false;

  RowData &rowData = getRowData(row);

  rowData.group = v;

  return true;
}

const CQBaseModel::RowData &
CQBaseModel::
getRowData(int row) const
{
  return const_cast<CQBaseModel *>(this)->getRowData(row);
}

CQBaseModel::RowData &
CQBaseModel::
getRowData(int row)
{
  assert(row >= 0 || row < rowCount());

  auto p = rowDatas_.find(row);

  if (p != rowDatas_.end()) {
    RowData &rowData = (*p).second;

    assert(rowData.row == row);

    return rowData;
  }

  //---

  std::unique_lock<std::mutex> lock(mutex_);

  auto *th = const_cast<CQBaseModel *>(this);

  auto p1 = th->rowDatas_.find(row);

  if (p1 == th->rowDatas_.end())
    p1 = th->rowDatas_.insert(p1, RowDatas::value_type(row, RowData(row)));

  return (*p1).second;
}

//------

void
CQBaseModel::
beginResetModel()
{
  if (resetDepth_ == 0)
    QAbstractItemModel::beginResetModel();

  ++resetDepth_;
}

void
CQBaseModel::
endResetModel()
{
  --resetDepth_;

  if (resetDepth_ == 0)
    QAbstractItemModel::endResetModel();
}

//------

QVariant
CQBaseModel::
headerData(int section, Qt::Orientation orientation, int role) const
{
  auto typeVariant = [](const CQBaseModelType &type) {
    if (type == CQBaseModelType::NONE)
      return QVariant();

    return QVariant(CQModelUtil::typeCast(type));
  };

  using CQModelUtil::roleCast;

  //---

  // generic column data
  if      (orientation == Qt::Horizontal) {
    if      (role == roleCast(CQBaseModelRole::Type)) {
      auto type = columnType(section);

      return typeVariant(type);
    }
    else if (role == roleCast(CQBaseModelRole::BaseType)) {
      auto type = columnBaseType(section);

      return typeVariant(type);
    }
    else if (role == roleCast(CQBaseModelRole::TypeValues)) {
      return QVariant(columnTypeValues(section));
    }
    else if (role == roleCast(CQBaseModelRole::Min)) {
      return columnMin(section);
    }
    else if (role == roleCast(CQBaseModelRole::Max)) {
      return columnMax(section);
    }
    else if (role == roleCast(CQBaseModelRole::Sum)) {
      return columnSum(section);
    }
    else if (role == roleCast(CQBaseModelRole::Key)) {
      return isColumnKey(section);
    }
    else if (role == roleCast(CQBaseModelRole::Sorted)) {
      return isColumnSorted(section);
    }
    else if (role == roleCast(CQBaseModelRole::SortOrder)) {
      return columnSortOrder(section);
    }
    else if (role == roleCast(CQBaseModelRole::Title)) {
      return columnTitle(section);
    }
    else if (role == roleCast(CQBaseModelRole::Tip)) {
      return columnTip(section);
    }
    else if (role == roleCast(CQBaseModelRole::DataMin)) {
      return columnMin(section);
    }
    else if (role == roleCast(CQBaseModelRole::DataMax)) {
      return columnMax(section);
    }
    else if (role == roleCast(CQBaseModelRole::HeaderType)) {
      auto type = columnHeaderType(section);

      return typeVariant(type);
    }
    else if (role == roleCast(CQBaseModelRole::HeaderTypeValues)) {
      return QVariant(headerTypeValues(section));
    }
    else {
      return QAbstractItemModel::headerData(section, orientation, role);
    }
  }
  // generic row data
  else if (orientation == Qt::Vertical) {
    if (role == roleCast(CQBaseModelRole::Group)) {
      return rowGroup(section);
    }
    else {
      return QAbstractItemModel::headerData(section, orientation, role);
    }
  }
  else {
    assert(false);
  }

  return QVariant();
}

bool
CQBaseModel::
setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
  using CQModelUtil::roleCast;

  //---

  // generic column data
  bool rc = false;

  if      (orientation == Qt::Horizontal) {
    if      (role == roleCast(CQBaseModelRole::Type)) {
      bool ok { false };

      auto type = variantToType(value, &ok);
      if (! ok) return false;

      rc = setColumnType(section, type);
    }
    else if (role == roleCast(CQBaseModelRole::BaseType)) {
      bool ok { false };

      auto type = variantToType(value, &ok);
      if (! ok) return false;

      rc = setColumnBaseType(section, type);
    }
    else if (role == roleCast(CQBaseModelRole::TypeValues)) {
      auto str = value.toString();

      rc = setColumnTypeValues(section, str);
    }
    else if (role == roleCast(CQBaseModelRole::Min)) {
      rc = setColumnMin(section, value);
    }
    else if (role == roleCast(CQBaseModelRole::Max)) {
      rc = setColumnMax(section, value);
    }
    else if (role == roleCast(CQBaseModelRole::Sum)) {
      rc = setColumnSum(section, value);
    }
    else if (role == roleCast(CQBaseModelRole::Key)) {
      rc = setColumnKey(section, value.toBool());
    }
    else if (role == roleCast(CQBaseModelRole::Sorted)) {
      rc = setColumnSorted(section, value.toBool());
    }
    else if (role == roleCast(CQBaseModelRole::SortOrder)) {
      rc = setColumnSortOrder(section, static_cast<Qt::SortOrder>(value.toInt()));
    }
    else if (role == roleCast(CQBaseModelRole::Title)) {
      rc = setColumnTitle(section, value.toString());
    }
    else if (role == roleCast(CQBaseModelRole::Tip)) {
      rc = setColumnTip(section, value.toString());
    }
    else if (role == roleCast(CQBaseModelRole::DataMin)) {
      assert(false);
    }
    else if (role == roleCast(CQBaseModelRole::DataMax)) {
      assert(false);
    }
    else if (role == roleCast(CQBaseModelRole::HeaderType)) {
      bool ok { false };

      auto type = variantToType(value, &ok);
      if (! ok) return false;

      return setColumnHeaderType(section, type);
    }
    else if (role == roleCast(CQBaseModelRole::HeaderTypeValues)) {
      auto str = value.toString();

      rc = setHeaderTypeValues(section, str);
    }
    else {
      return QAbstractItemModel::setHeaderData(section, orientation, role);
    }
  }
  // generic row data
  else if (orientation == Qt::Vertical) {
    if (role == roleCast(CQBaseModelRole::Group)) {
      rc = setRowGroup(section, value);
    }
    else {
      return QAbstractItemModel::setHeaderData(section, orientation, role);
    }
  }
  else {
    assert(false);
  }

  if (rc) {
    auto *t1 = this->thread();
    auto *t2 = QThread::currentThread();

    if (t1 == t2)
      Q_EMIT headerDataChanged(orientation, section, section);
  }

  return rc;
}

QVariant
CQBaseModel::
data(const QModelIndex &index, int role) const
{
  if (role == Qt::TextAlignmentRole) {
    auto type = columnType(index.column());

    if (type == CQBaseModelType::INTEGER || type == CQBaseModelType::REAL)
      return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    else
      return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
  }

  //return QAbstractItemModel::data(index, role);
  return QVariant();
}

//------

const CQBaseModel::RoleDatas &
CQBaseModel::
headerRoleDatas(Qt::Orientation orient) const
{
  static std::map<Qt::Orientation, CQBaseModel::RoleDatas> s_headerRoleDatas;

  //---

  using Standard = CQBaseModel::Standard;
  using Writable = CQBaseModel::Writable;

  auto addHeaderRole = [&](Qt::Orientation orient, const QString &name, int role,
                           const QVariant::Type &type, const Writable &writable) {
    s_headerRoleDatas[orient].emplace_back(name, role, type, Standard(), writable);
  };

  auto addHHeaderRole = [&](const QString &name, int role, const QVariant::Type &type,
                            const Writable &writable=Writable()) {
    addHeaderRole(Qt::Horizontal, name, role, type, writable);
  };

  auto addVHeaderRole = [&](const QString &name, int role, const QVariant::Type &type,
                            const Writable &writable=Writable()) {
    addHeaderRole(Qt::Vertical, name, role, type, writable);
  };

  using CQModelUtil::roleCast;

  //---

  if      (orient == Qt::Horizontal) {
    addHHeaderRole("type"              , roleCast(CQBaseModelRole::Type),
                   QVariant::UserType, Writable(true));
    addHHeaderRole("base_type"         , roleCast(CQBaseModelRole::BaseType),
                   QVariant::UserType, Writable(true));
    addHHeaderRole("type_values"       , roleCast(CQBaseModelRole::TypeValues),
                   QVariant::String  , Writable(true));
    addHHeaderRole("min"               , roleCast(CQBaseModelRole::Min),
                   QVariant::Invalid , Writable(true));
    addHHeaderRole("max"               , roleCast(CQBaseModelRole::Max),
                   QVariant::Invalid , Writable(true));
    addHHeaderRole("sum"               , roleCast(CQBaseModelRole::Sum),
                   QVariant::Invalid , Writable(true));
    addHHeaderRole("key"               , roleCast(CQBaseModelRole::Key),
                   QVariant::Bool    , Writable(true));
    addHHeaderRole("sorted"            , roleCast(CQBaseModelRole::Sorted),
                   QVariant::Bool    , Writable(true));
    addHHeaderRole("sort_order"        , roleCast(CQBaseModelRole::SortOrder),
                   QVariant::Int     , Writable(true));
    addHHeaderRole("title"             , roleCast(CQBaseModelRole::Title),
                   QVariant::String  , Writable(true));
    addHHeaderRole("tip"               , roleCast(CQBaseModelRole::Tip),
                   QVariant::String  , Writable(true));
    addHHeaderRole("data_min"          , roleCast(CQBaseModelRole::DataMin),
                   QVariant::Invalid , Writable(false));
    addHHeaderRole("data_max"          , roleCast(CQBaseModelRole::DataMax),
                   QVariant::Invalid , Writable(false));
    addHHeaderRole("header_type"       , roleCast(CQBaseModelRole::HeaderType),
                   QVariant::UserType, Writable(true));
    addHHeaderRole("header_type_values", roleCast(CQBaseModelRole::HeaderTypeValues),
                   QVariant::Invalid , Writable(true));
  }
  else if (orient == Qt::Vertical) {
    addVHeaderRole("group", roleCast(CQBaseModelRole::Group),
                   QVariant::Invalid, Writable(true));
  }

  return s_headerRoleDatas[orient];
}

const CQBaseModel::RoleDatas &
CQBaseModel::
roleDatas() const
{
  static CQBaseModel::RoleDatas s_roleDatas;

  return s_roleDatas;
}

//------

QVariant
CQBaseModel::
metaNameValue(const QString &name, const QString &key) const
{
  auto pn = metaNameValues_.find(name);
  if (pn == metaNameValues_.end()) return QVariant();

  auto pk = (*pn).second.find(key);
  if (pk == (*pn).second.end()) return QVariant();

  return (*pk).second;
}

void
CQBaseModel::
setMetaNameValue(const QString &name, const QString &key, const QVariant &value)
{
  metaNameValues_[name][key] = value;
}

QStringList
CQBaseModel::
metaNames() const
{
  QStringList names;

  for (const auto &pn : metaNameValues_)
    names << pn.first;

  return names;
}

QStringList
CQBaseModel::
metaNameKeys(const QString &name) const
{
  QStringList keys;

  auto pn = metaNameValues_.find(name);

  if (pn != metaNameValues_.end()) {
    for (const auto &pk : (*pn).second)
      keys << pk.first;
  }

  return keys;
}

//------

int
CQBaseModel::
modelColumnNameToInd(const QString &name) const
{
  return modelColumnNameToInd(this, name);
}

int
CQBaseModel::
modelColumnNameToInd(const QAbstractItemModel *model, const QString &name)
{
  int role = Qt::DisplayRole;

  auto nc = model->columnCount();

  for (decltype(nc) icolumn = 0; icolumn < nc; ++icolumn) {
    auto var = model->headerData(icolumn, Qt::Horizontal, role);

    if (! var.isValid())
      continue;

    auto name1 = var.toString();

    if (name == name1)
      return icolumn;
  }

  //---

  bool ok { false };

  int column = name.toInt(&ok);

  if (ok)
    return column;

  return -1;
}

//------

CQBaseModelType
CQBaseModel::
variantToType(const QVariant &var, bool *ok)
{
  auto type = CQBaseModelType::NONE;

  if      (var.type() == QVariant::Int)
    type = static_cast<CQBaseModelType>(var.toInt(ok));
  else if (var.type() == QVariant::LongLong)
    type = static_cast<CQBaseModelType>(var.toLongLong(ok));
  else {
    auto str = var.toString();

    type = nameType(str);
  }

  if (! isType(CQModelUtil::typeCast(type))) {
    if (ok)
      *ok = false;
  }

  return type;
}

QVariant
CQBaseModel::
typeToVariant(CQBaseModelType type)
{
  return QVariant(CQModelUtil::typeCast(type));
}

bool
CQBaseModel::
isSameType(const QVariant &var, CQBaseModelType type)
{
  if (type == CQBaseModelType::REAL && var.type() == QVariant::Double)
    return true;

  if (type == CQBaseModelType::INTEGER &&
      (var.type() == QVariant::Int || var.type() == QVariant::LongLong))
    return true;

#if 0
  if (type == CQBaseModelType::TIME && var.type() == QVariant::Double)
    return true;
#endif

  return false;
}

QVariant
CQBaseModel::
typeStringToVariant(const QString &str, CQBaseModelType type)
{
  if      (type == CQBaseModelType::REAL) {
    bool ok { false };

    double real = toReal(str, ok);

    if (ok)
      return QVariant(real);

    if (! str.length())
      return QVariant();
  }
  else if (type == CQBaseModelType::INTEGER) {
    bool ok { false };

    long integer = toInt(str, ok);

    if (ok)
      return CQModelUtil::intVariant(integer);

    if (! str.length())
      return QVariant();
  }
#if 0
  else if (type == CQBaseModelType::TIME) {
    bool ok { false };

    double real = toReal(str, ok);

    if (ok)
      return QVariant(real);
  }
#endif

  return QVariant(str);
}

bool
CQBaseModel::
isType(int type)
{
  initTypes();

  return (s_typeName.find(static_cast<CQBaseModelType>(type)) != s_typeName.end());
}

QString
CQBaseModel::
typeName(CQBaseModelType type)
{
  initTypes();

  auto p = s_typeName.find(type);

  if (p == s_typeName.end())
    return "none";

  return (*p).second;
}

CQBaseModelType
CQBaseModel::
nameType(const QString &name)
{
  initTypes();

  auto lname = name.toLower();

  auto p = s_nameType.find(lname);

  if (p != s_nameType.end())
    return (*p).second;

  auto pa = s_aliasName.find(lname);

  if (pa != s_aliasName.end())
    return nameType((*pa).second);

  return CQBaseModelType::NONE;
}

double
CQBaseModel::
toReal(const QString &str, bool &ok)
{
  return str.toDouble(&ok);
}

long
CQBaseModel::
toInt(const QString &str, bool &ok)
{
  return str.toInt(&ok);
}

//---

void
CQBaseModel::
copyHeaderRoles(QAbstractItemModel *toModel) const
{
  auto nc = columnCount();

  for (decltype(nc) ic = 0; ic < nc; ++ic)
    copyColumnHeaderRoles(toModel, ic, ic);
}

void
CQBaseModel::
copyColumnHeaderRoles(QAbstractItemModel *toModel, int c1, int c2) const
{
  using CQModelUtil::roleCast;

  //---

  static std::vector<int> hroles = {{
    Qt::DisplayRole,
    roleCast(CQBaseModelRole::Type),
    roleCast(CQBaseModelRole::BaseType),
    roleCast(CQBaseModelRole::TypeValues),
    roleCast(CQBaseModelRole::Min),
    roleCast(CQBaseModelRole::Max),
    roleCast(CQBaseModelRole::Sum),
    roleCast(CQBaseModelRole::Key),
    roleCast(CQBaseModelRole::Sorted),
    roleCast(CQBaseModelRole::SortOrder),
    roleCast(CQBaseModelRole::Title),
    roleCast(CQBaseModelRole::Tip),
    roleCast(CQBaseModelRole::DataMin),
    roleCast(CQBaseModelRole::DataMax),
    roleCast(CQBaseModelRole::HeaderType),
    roleCast(CQBaseModelRole::HeaderTypeValues)
  }};

  // copy horizontal header data
  for (const auto &role : hroles) {
    auto var = headerData(c1, Qt::Horizontal, role);

    if (var.isValid())
      toModel->setHeaderData(c2, Qt::Horizontal, var, role);
  }
}
