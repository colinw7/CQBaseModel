#include <CQBaseModel.h>
#include <CQModelUtil.h>
#include <CQModelNameValues.h>
#include <CMathUtil.h>
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
  nameValues_  = model->nameValues_;

  endResetModel();
}

//---

void
CQBaseModel::
genColumnTypes()
{
  resetColumnTypes();

  // auto determine type for each column. Do column by column to allow early out
  int nc = columnCount();

  for (int column = 0; column < nc; ++column)
    genColumnType(column);
}

void
CQBaseModel::
genColumnType(int column)
{
  ColumnData &columnData = getColumnData(column);

  genColumnType(columnData);

  emit columnTypeChanged(columnData.column);
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
  int maxRows = maxTypeRows();

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

CQBaseModelType
CQBaseModel::
columnType(int column) const
{
  if (column < 0 || column >= columnCount())
    return CQBaseModelType::NONE;

  const ColumnData &columnData = getColumnData(column);

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

  ColumnData &columnData = getColumnData(column);

  if (columnData.baseType == CQBaseModelType::NONE)
    genColumnType(columnData);

  if (type != columnData.type) {
    columnData.type = type;

    emit columnTypeChanged(column);
  }

  return true;
}

CQBaseModelType
CQBaseModel::
columnBaseType(int column) const
{
  if (column < 0 || column >= columnCount())
    return CQBaseModelType::NONE;

  const ColumnData &columnData = getColumnData(column);

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

  ColumnData &columnData = getColumnData(column);

  if (type != columnData.baseType) {
    columnData.baseType = type;

    emit columnBaseTypeChanged(column);
  }

  return true;
}

QString
CQBaseModel::
columnTypeValues(int column) const
{
  if (column < 0 || column >= columnCount())
    return QString();

  const ColumnData &columnData = getColumnData(column);

  return columnData.typeValues;
}

bool
CQBaseModel::
setColumnTypeValues(int column, const QString &str)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.typeValues = str;

  emit columnTypeChanged(column);

  return true;
}

QVariant
CQBaseModel::
columnMin(int column) const
{
  if (column < 0 || column >= columnCount())
    return QVariant();

  const ColumnData &columnData = getColumnData(column);

  return columnData.min;
}

bool
CQBaseModel::
setColumnMin(int column, const QVariant &v)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.min = v;

  emit columnRangeChanged(column);

  return true;
}

QVariant
CQBaseModel::
columnMax(int column) const
{
  if (column < 0 || column >= columnCount())
    return QVariant();

  const ColumnData &columnData = getColumnData(column);

  return columnData.max;
}

bool
CQBaseModel::
setColumnMax(int column, const QVariant &v)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.max = v;

  emit columnRangeChanged(column);

  return true;
}

bool
CQBaseModel::
isColumnKey(int column) const
{
  if (column < 0 || column >= columnCount())
    return false;

  const ColumnData &columnData = getColumnData(column);

  return columnData.key;
}

bool
CQBaseModel::
setColumnKey(int column, bool b)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.key = b;

  emit columnKeyChanged(column);

  return true;
}

bool
CQBaseModel::
isColumnSorted(int column) const
{
  if (column < 0 || column >= columnCount())
    return false;

  const ColumnData &columnData = getColumnData(column);

  return columnData.sorted;
}

bool
CQBaseModel::
setColumnSorted(int column, bool b)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.sorted = b;

  emit columnSortedChanged(column);

  return true;
}

Qt::SortOrder
CQBaseModel::
columnSortOrder(int column) const
{
  if (column < 0 || column >= columnCount())
    return Qt::AscendingOrder;

  const ColumnData &columnData = getColumnData(column);

  return columnData.sortOrder;
}

bool
CQBaseModel::
setColumnSortOrder(int column, Qt::SortOrder order)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.sortOrder = order;

  emit columnSortOrderChanged(column);

  return true;
}

QString
CQBaseModel::
columnTitle(int column) const
{
  if (column < 0 || column >= columnCount())
    return "";

  const ColumnData &columnData = getColumnData(column);

  return columnData.title;
}

bool
CQBaseModel::
setColumnTitle(int column, const QString &s)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.title = s;

  emit columnTitleChanged(column);

  return true;
}

QString
CQBaseModel::
columnTip(int column) const
{
  if (column < 0 || column >= columnCount())
    return "";

  const ColumnData &columnData = getColumnData(column);

  return columnData.tip;
}

bool
CQBaseModel::
setColumnTip(int column, const QString &s)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.tip = s;

  emit columnTipChanged(column);

  return true;
}

CQBaseModelType
CQBaseModel::
columnHeaderType(int column) const
{
  if (column < 0 || column >= columnCount())
    return CQBaseModelType::NONE;

  const ColumnData &columnData = getColumnData(column);

  return columnData.headerType;
}

bool
CQBaseModel::
setColumnHeaderType(int column, CQBaseModelType type)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  if (columnData.headerType != type) {
    columnData.headerType = type;

    emit columnHeaderTypeChanged(column);
  }

  return true;
}

QString
CQBaseModel::
headerTypeValues(int column) const
{
  if (column < 0 || column >= columnCount())
    return QString();

  const ColumnData &columnData = getColumnData(column);

  return columnData.headerTypeValues;
}

bool
CQBaseModel::
setHeaderTypeValues(int column, const QString &str)
{
  if (column < 0 || column >= columnCount())
    return false;

  ColumnData &columnData = getColumnData(column);

  columnData.headerTypeValues = str;

  emit columnTypeChanged(column);

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
    ColumnData &columnData = (*p).second;

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
    ColumnData &columnData = p.second;

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

    return QVariant((int) type);
  };

  // generic column data
  if      (orientation == Qt::Horizontal) {
    if      (role == static_cast<int>(CQBaseModelRole::Type)) {
      auto type = columnType(section);

      return typeVariant(type);
    }
    else if (role == static_cast<int>(CQBaseModelRole::BaseType)) {
      auto type = columnBaseType(section);

      return typeVariant(type);
    }
    else if (role == static_cast<int>(CQBaseModelRole::TypeValues)) {
      return QVariant(columnTypeValues(section));
    }
    else if (role == static_cast<int>(CQBaseModelRole::Min)) {
      return columnMin(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Max)) {
      return columnMax(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Key)) {
      return isColumnKey(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Sorted)) {
      return isColumnSorted(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::SortOrder)) {
      return columnSortOrder(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Title)) {
      return columnTitle(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Tip)) {
      return columnTip(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::DataMin)) {
      return columnMin(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::DataMax)) {
      return columnMax(section);
    }
    else if (role == static_cast<int>(CQBaseModelRole::HeaderType)) {
      auto type = columnHeaderType(section);

      return typeVariant(type);
    }
    else if (role == static_cast<int>(CQBaseModelRole::HeaderTypeValues)) {
      return QVariant(headerTypeValues(section));
    }
    else {
      return QAbstractItemModel::headerData(section, orientation, role);
    }
  }
  // generic row data
  else if (orientation == Qt::Vertical) {
    if (role == static_cast<int>(CQBaseModelRole::Group)) {
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
  // generic column data
  if      (orientation == Qt::Horizontal) {
    if      (role == static_cast<int>(CQBaseModelRole::Type)) {
      bool ok { false };

      auto type = variantToType(value, &ok);
      if (! ok) return false;

      return setColumnType(section, type);
    }
    else if (role == static_cast<int>(CQBaseModelRole::BaseType)) {
      bool ok { false };

      auto type = variantToType(value, &ok);
      if (! ok) return false;

      return setColumnBaseType(section, type);
    }
    else if (role == static_cast<int>(CQBaseModelRole::TypeValues)) {
      auto str = value.toString();

      return setColumnTypeValues(section, str);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Min)) {
      return setColumnMin(section, value);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Max)) {
      return setColumnMax(section, value);
    }
    else if (role == static_cast<int>(CQBaseModelRole::Key)) {
      return setColumnKey(section, value.toBool());
    }
    else if (role == static_cast<int>(CQBaseModelRole::Sorted)) {
      return setColumnSorted(section, value.toBool());
    }
    else if (role == static_cast<int>(CQBaseModelRole::SortOrder)) {
      return setColumnSortOrder(section, (Qt::SortOrder) value.toInt());
    }
    else if (role == static_cast<int>(CQBaseModelRole::Title)) {
      return setColumnTitle(section, value.toString());
    }
    else if (role == static_cast<int>(CQBaseModelRole::Tip)) {
      return setColumnTip(section, value.toString());
    }
    else if (role == static_cast<int>(CQBaseModelRole::DataMin)) {
      assert(false);
    }
    else if (role == static_cast<int>(CQBaseModelRole::DataMax)) {
      assert(false);
    }
    else if (role == static_cast<int>(CQBaseModelRole::HeaderType)) {
      bool ok { false };

      auto type = variantToType(value, &ok);
      if (! ok) return false;

      return setColumnHeaderType(section, type);
    }
    else if (role == static_cast<int>(CQBaseModelRole::HeaderTypeValues)) {
      auto str = value.toString();

      return setHeaderTypeValues(section, str);
    }
    else {
      return QAbstractItemModel::setHeaderData(section, orientation, role);
    }
  }
  // generic row data
  else if (orientation == Qt::Vertical) {
    if (role == static_cast<int>(CQBaseModelRole::Group)) {
      return setRowGroup(section, value);
    }
    else {
      return QAbstractItemModel::setHeaderData(section, orientation, role);
    }
  }
  else {
    assert(false);
  }

  return false;
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

QVariant
CQBaseModel::
nameValue(const QString &name) const
{
  auto p = nameValues_.find(name);
  if (p == nameValues_.end()) return QVariant();

  return (*p).second;
}

void
CQBaseModel::
setNameValue(const QString &name, const QVariant &value)
{
  nameValues_[name] = value;
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

  for (int icolumn = 0; icolumn < model->columnCount(); ++icolumn) {
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

  if (var.type() == QVariant::Int) {
    type = (CQBaseModelType) var.toInt(ok);
  }
  else {
    auto str = var.toString();

    type = nameType(str);
  }

  if (! isType((int) type)) {
    if (ok)
      *ok = false;
  }

  return type;
}

QVariant
CQBaseModel::
typeToVariant(CQBaseModelType type)
{
  return QVariant((int) type);
}

bool
CQBaseModel::
isSameType(const QVariant &var, CQBaseModelType type)
{
  if (type == CQBaseModelType::REAL && var.type() == QVariant::Double)
    return true;

  if (type == CQBaseModelType::INTEGER && var.type() == QVariant::Int)
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
      return QVariant(int(integer));

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

  return (s_typeName.find((CQBaseModelType) type) != s_typeName.end());
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

bool
CQBaseModel::
isInteger(double r)
{
  return CMathUtil::isInteger(r);
}
