#include <CQModelUtil.h>
#include <CQModelVisitor.h>
#include <CQBaseModel.h>
#include <CQAlignVariant.h>

#include <CMathUtil.h>

#include <QSortFilterProxyModel>
#include <QColor>

namespace CQModelUtil {

int
roleCast(const CQBaseModelRole &role)
{
  return static_cast<int>(role);
}

int
typeCast(const CQBaseModelType &type)
{
  return static_cast<int>(type);
}

//------

bool
isHierarchical(const QAbstractItemModel *model)
{
  if (! model)
    return false;

  QModelIndex parent;

  int nr = model->rowCount(parent);

  nr = std::min(nr, 100); // limit number of rows checked

  for (int row = 0; row < nr; ++row) {
    auto index1 = model->index(row, 0, parent);

    if (model->hasChildren(index1))
      return true;
  }

  return false;
}

//------

QVariant
modelHeaderValue(const QAbstractItemModel *model, int c, Qt::Orientation orientation,
                 int role, bool &ok)
{
  QVariant var;

  if (role >= 0)
    var = model->headerData(c, orientation, role);
  else
    var = model->headerData(c, orientation, Qt::DisplayRole);

  ok = var.isValid();

  return var;
}

QVariant
modelHeaderValue(const QAbstractItemModel *model, int c, Qt::Orientation orientation,
                 CQBaseModelRole role, bool &ok)
{
  return modelHeaderValue(model, c, orientation, roleCast(role), ok);
}

QVariant
modelHeaderValue(const QAbstractItemModel *model, int c, int role, bool &ok)
{
  return modelHeaderValue(model, c, Qt::Horizontal, role, ok);
}

QVariant
modelHeaderValue(const QAbstractItemModel *model, int c, CQBaseModelRole role, bool &ok)
{
  return modelHeaderValue(model, c, roleCast(role), ok);
}

QString
modelHeaderString(const QAbstractItemModel *model, int c, bool &ok)
{
  auto var = modelHeaderValue(model, c, Qt::DisplayRole, ok);

  return var.toString();
}

//------

QVariant
modelValue(const QAbstractItemModel *model, int r, int c, const QModelIndex &parent, bool &ok)
{
  auto ind = model->index(r, c, parent);

  return modelValue(model, ind, ok);
}

QVariant
modelValue(const QAbstractItemModel *model, const QModelIndex &ind, int role, bool &ok)
{
  QVariant var;

  ok = ind.isValid();

  if (ok) {
    if (role >= 0)
      var = model->data(ind, role);
    else {
      var = model->data(ind, Qt::EditRole);

      if (! var.isValid())
        var = model->data(ind, Qt::DisplayRole);
    }

    ok = var.isValid();
  }

  return var;
}

QVariant
modelValue(const QAbstractItemModel *model, const QModelIndex &ind, bool &ok)
{
  auto var = modelValue(model, ind, Qt::EditRole, ok);

  if (! ok)
    var = modelValue(model, ind, Qt::DisplayRole, ok);

  return var;
}

double
modelReal(const QAbstractItemModel *model, const QModelIndex &ind, bool &ok)
{
  auto var = modelValue(model, ind, ok);

  double r = 0.0;

  if (ok)
    r = var.toDouble(&ok);

  return r;
}

long
modelConvInteger(const QAbstractItemModel *model, const QModelIndex &ind, bool &ok)
{
  auto var = modelValue(model, ind, ok);

  long i = 0;

  if (ok) {
    if      (var.type() == QVariant::LongLong)
      i = var.value<qlonglong>();
    else if (var.type() == QVariant::Double) {
      double r = var.toDouble(&ok);

      i = var.toInt(&ok);

      if (r - double(i) > 1E-6)
        ok = false;
    }
    else
      i = var.toInt(&ok);
  }

  return i;
}

long
modelInteger(const QAbstractItemModel *model, const QModelIndex &ind, bool &ok)
{
  auto var = modelValue(model, ind, ok);

  long i = 0;

  if (ok) {
    if (var.type() == QVariant::LongLong)
      i = var.value<qlonglong>();
    else
      i = var.toInt(&ok);
  }

  return i;
}

QString
modelString(const QAbstractItemModel *model, const QModelIndex &ind, bool &ok)
{
  auto var = modelValue(model, ind, ok);

  QString s;

  if (ok)
    s = var.toString();

  return s;
}

//------

bool
columnValueType(const QAbstractItemModel *model, int c, CQBaseModelType &type)
{
  type = CQBaseModelType::STRING;

  auto var = model->headerData(c, Qt::Horizontal, roleCast(CQBaseModelRole::Type));
  if (! var.isValid()) return false;

  bool ok;
  type = static_cast<CQBaseModelType>(var.toInt(&ok));
  if (! ok) return false;

  return true;
}

CQBaseModelType
calcColumnType(const QAbstractItemModel *model, int icolumn, int maxRows)
{
  //CQPerfTrace trace("CQUtil::calcColumnType");

  // determine column type from values

  // process model data
  class ColumnTypeVisitor : public CQModelVisitor {
   public:
    ColumnTypeVisitor(int column) :
     column_(column) {
    }

    void initVisit() override {
      nr_ = model_->rowCount(QModelIndex());
    }

    State visit(const QAbstractItemModel *model, const VisitData &data) override {
      auto ind = model->index(data.row, column_, data.parent);

      // if column can be integral, check if value is valid integer
      if (isInt_) {
        bool ok;

        (void) modelConvInteger(model, ind, ok);

        if (ok)
          return State::SKIP;

        auto str = modelString(model, ind, ok);

        if (! str.length()) { // empty
          ++numEmpty_;
          return State::SKIP;
        }

        isInt_ = false; // non-integral string so can't be intergal
      }

      // if column can be real, check if value is valid real
      if (isReal_) {
        bool ok;

        (void) modelReal(model, ind, ok);

        if (ok)
          return State::SKIP;

        auto str = modelString(model, ind, ok);

        if (! str.length()) { // empty
          ++numEmpty_;
          return State::SKIP;
        }

        isReal_ = false; // non-real string so can't be real
      }

      // not real or integer so assume string and we are done
      return State::TERMINATE;
    }

    CQBaseModelType columnType() {
      if (numEmpty_ == nr_ || (nr_ > maxRows_ && numEmpty_ == maxRows_))
        return CQBaseModelType::STRING;

      if      (isInt_ ) return CQBaseModelType::INTEGER;
      else if (isReal_) return CQBaseModelType::REAL;
      else              return CQBaseModelType::STRING;
    }

   private:
    int  column_   { -1 };   //!< column to check
    bool isInt_    { true }; //!< could be integeral
    bool isReal_   { true }; //!< could be real
    int  nr_       { 0 };    //!< number of rows
    int  numEmpty_ { 0 };    //!< number of empty values
  };

  // determine column value type by looking at model values
  ColumnTypeVisitor columnTypeVisitor(icolumn);

  if (maxRows > 0)
    columnTypeVisitor.setNumRows(maxRows);

  CQModelVisit::exec(model, columnTypeVisitor);

  return columnTypeVisitor.columnType();
}

//------

QAbstractItemModel *
getBaseModel(QAbstractItemModel *model)
{
  auto *sourceModel = model;

  auto *proxyModel = qobject_cast<QAbstractProxyModel *>(sourceModel);

  while (proxyModel) {
    sourceModel = proxyModel->sourceModel();

    proxyModel = qobject_cast<QAbstractProxyModel *>(sourceModel);
  }

  return sourceModel;
}

//------

using RoleData        = CQBaseModel::RoleData;
using RoleDatas       = std::vector<RoleData>;
using NameRoleMap     = std::map<QString, int>;
using RoleNameMap     = std::map<int, QString>;
using NameStandardMap = std::map<QString, bool>;
using NameWritableMap = std::map<QString, bool>;
using NameTypeMap     = std::map<QString, QVariant::Type>;

static RoleDatas       s_roleNames;
static RoleNameMap     s_roleNameMap;
static NameRoleMap     s_nameRoleMap;
static NameStandardMap s_nameStandardMap;
static NameWritableMap s_nameWritableMap;
static NameTypeMap     s_nameTypeMap;

static std::map<Qt::Orientation, RoleDatas>       s_headerRoleDatas;
static std::map<Qt::Orientation, RoleNameMap>     s_headerRoleNameMap;
static std::map<Qt::Orientation, NameRoleMap>     s_headerNameRoleMap;
static std::map<Qt::Orientation, NameTypeMap>     s_headerNameTypeMap;
static std::map<Qt::Orientation, NameWritableMap> s_headerNameWritableMap;

bool
initRoles(const QAbstractItemModel *model)
{
  static QAbstractItemModel *s_model;

  if (s_model != model)
    s_roleNames.clear();

  if (! s_roleNames.empty())
    return false;

  //---

  s_model = const_cast<QAbstractItemModel *>(model);

  s_nameRoleMap    .clear();
  s_nameStandardMap.clear();
  s_nameTypeMap    .clear();

//using Standard = CQBaseModel::Standard;
  using Writable = CQBaseModel::Writable;

  auto addRole = [&](const RoleData &data, bool alias=false) {
    if (! alias) {
      s_roleNames.push_back(data);

      s_roleNameMap[data.role] = data.name;
    }

    s_nameRoleMap    [data.name] = data.role;
    s_nameStandardMap[data.name] = data.standard;
    s_nameWritableMap[data.name] = data.writable;
    s_nameTypeMap    [data.name] = data.type;
  };

  auto addStandardRole = [&](const QString &name, int role,
                             const QVariant::Type &type=QVariant::Invalid,
                             const Writable &writable=Writable(), bool alias=false) {
    auto roleData = RoleData(name, role, type);

    roleData.standard = true;
    roleData.writable = writable;

    addRole(roleData, alias);
  };

  //---

  auto alignMeta = static_cast<QVariant::Type>(CQAlignVariant::getMetaType());

  // standard
  addStandardRole("display"       , Qt::DisplayRole      , QVariant::Invalid); // QVariant
  addStandardRole("edit"          , Qt::EditRole         , QVariant::Invalid);
  addStandardRole("user"          , Qt::UserRole         , QVariant::Invalid);
  addStandardRole("font"          , Qt::FontRole         ,
                  QVariant::Font, Writable(true)); // QFont
  addStandardRole("size_hint"     , Qt::SizeHintRole     , QVariant::Invalid);
  addStandardRole("tool_tip"      , Qt::ToolTipRole      , QVariant::String);
  addStandardRole("background"    , Qt::BackgroundRole   ,
                  QVariant::Color, Writable(true)); // QBrush
  addStandardRole("foreground"    , Qt::ForegroundRole   ,
                  QVariant::Color, Writable(true)); // QBrush
  addStandardRole("text_alignment", Qt::TextAlignmentRole, alignMeta); // Qt::Alignment
  addStandardRole("text_color"    , Qt::ForegroundRole   , QVariant::Color, /*alias*/true);
  addStandardRole("decoration"    , Qt::DecorationRole   , QVariant::Invalid); // QIcon, QPixmap,
                                                                               // QColor

  const auto *baseModel = dynamic_cast<const CQBaseModel *>(model);

  if (baseModel) {
    for (const auto &roleData : baseModel->roleDatas()) {
      addRole(roleData);
    }
  }

  //---

  auto addHeaderRoleData = [&](Qt::Orientation orient, const RoleData &data) {
    s_headerRoleDatas[orient].push_back(data);

    s_headerRoleNameMap[orient][data.role] = data.name;

    s_headerNameRoleMap    [orient][data.name] = data.role;
    s_headerNameWritableMap[orient][data.name] = data.writable;
    s_headerNameTypeMap    [orient][data.name] = data.type;
  };

  auto addHHeaderRole = [&](const QString &name, int role,
                            const QVariant::Type &type=QVariant::Invalid) {
    addHeaderRoleData(Qt::Horizontal,
                      RoleData(name, role, type, CQBaseModel::Standard(), CQBaseModel::Writable()));
  };

  auto addVHeaderRole = [&](const QString &name, int role,
                            const QVariant::Type &type=QVariant::Invalid) {
    addHeaderRoleData(Qt::Vertical,
                      RoleData(name, role, type, CQBaseModel::Standard(), CQBaseModel::Writable()));
  };

  // TODO: set writable ?
  addHHeaderRole("display", Qt::DisplayRole);
  addHHeaderRole("edit"   , Qt::EditRole);
  addHHeaderRole("tooltip", Qt::ToolTipRole);

  if (baseModel) {
    for (const auto &roleData : baseModel->headerRoleDatas(Qt::Horizontal)) {
      addHeaderRoleData(Qt::Horizontal, roleData);
    }
  }

#if 0
  addHHeaderRole("format" , roleCast(CQBaseModelRole::Format));
  addHHeaderRole("iformat", roleCast(CQBaseModelRole::IFormat));
  addHHeaderRole("oformat", roleCast(CQBaseModelRole::OFormat));

  addHHeaderRole("fill_color" , roleCast(CQBaseModelRole::FillColor));
  addHHeaderRole("symbol_type", roleCast(CQBaseModelRole::SymbolType));
  addHHeaderRole("symbol_size", roleCast(CQBaseModelRole::SymbolSize));
  addHHeaderRole("font_size"  , roleCast(CQBaseModelRole::FontSize));
#endif

  addVHeaderRole("display", Qt::DisplayRole);

  return true;
}

const QStringList &
roleNames(QAbstractItemModel *model)
{
  static QStringList s_names;

  if (initRoles(model))
    s_names.clear();

  if (s_names.empty()) {
    for (const auto &roleName : s_roleNames)
      s_names << roleName.name;
  }

  return s_names;
};

int
nameToRole(QAbstractItemModel *model, const QString &name)
{
  (void) initRoles(model);

  auto p = s_nameRoleMap.find(name);

  if (p != s_nameRoleMap.end())
    return (*p).second;

  bool ok;

  int role = name.toInt(&ok);
  if (! ok) return -1;

  return role;
}

QString
roleToName(QAbstractItemModel *model, int role)
{
  (void) initRoles(model);

  auto p = s_roleNameMap.find(role);

  if (p != s_roleNameMap.end())
    return (*p).second;

  return QString::number(role);
}

bool
nameIsStandard(QAbstractItemModel *model, const QString &name)
{
  (void) initRoles(model);

  auto p = s_nameStandardMap.find(name);

  if (p != s_nameStandardMap.end())
    return (*p).second;

  return false;
}

bool
nameIsWritable(QAbstractItemModel *model, const QString &name)
{
  (void) initRoles(model);

  auto p = s_nameWritableMap.find(name);

  if (p != s_nameWritableMap.end())
    return (*p).second;

  return false;
}

QVariant::Type
nameType(QAbstractItemModel *model, const QString &name)
{
  (void) initRoles(model);

  auto p = s_nameTypeMap.find(name);

  if (p != s_nameTypeMap.end())
    return (*p).second;

  return QVariant::Invalid;
}

//---

const QStringList &
headerRoleNames(QAbstractItemModel *model, Qt::Orientation orient)
{
  static QStringList s_names;

  if (initRoles(model))
    s_names.clear();

  if (s_names.empty()) {
    const auto &headerRoleDatas = s_headerRoleDatas[orient];

    for (const auto &roleName : headerRoleDatas)
      s_names << roleName.name;
  }

  return s_names;
};

int
headerNameToRole(QAbstractItemModel *model, Qt::Orientation orient, const QString &name)
{
  (void) initRoles(model);

  const auto &headerNameRoleMap = s_headerNameRoleMap[orient];

  auto p = headerNameRoleMap.find(name);

  if (p != headerNameRoleMap.end())
    return (*p).second;

  bool ok;

  int role = name.toInt(&ok);
  if (! ok) return -1;

  return role;
}

QString
headerRoleToName(QAbstractItemModel *model, Qt::Orientation orient, int role)
{
  (void) initRoles(model);

  const auto &headerRoleNameMap = s_headerRoleNameMap[orient];

  auto p = headerRoleNameMap.find(role);

  if (p != headerRoleNameMap.end())
    return (*p).second;

  return QString::number(role);
}

QVariant::Type
headerNameType(QAbstractItemModel *model, Qt::Orientation orient, const QString &name)
{
  (void) initRoles(model);

  const auto &headerNameTypeMap = s_headerNameTypeMap[orient];

  auto p = headerNameTypeMap.find(name);

  if (p != headerNameTypeMap.end())
    return (*p).second;

  return QVariant::Invalid;
}

bool
headerNameIsWritable(QAbstractItemModel *model, Qt::Orientation orient, const QString &name)
{
  (void) initRoles(model);

  const auto &headerNameWritableMap = s_headerNameWritableMap[orient];

  auto p = headerNameWritableMap.find(name);

  if (p != headerNameWritableMap.end())
    return (*p).second;

  return false;
}

//---

bool
stringToRowCol(const QString &str, int &row, int &col)
{
  // get cell index
  row = -1;
  col = -1;

  auto strs = str.split(":");

  if (strs.length() != 2)
    return false;

  bool ok;
  row = strs[0].toInt(&ok); if (! ok) row = -1;
  col = strs[1].toInt(&ok); if (! ok) col = -1;

  if (row < 0 || col < 0)
    return false;

  return true;
}

//---

QVariant
intVariant(long l)
{
  return QVariant(static_cast<qlonglong>(l));
}

QVariant
realVariant(double r)
{
  return QVariant(r);
}

QVariant
stringVariant(const QString &s)
{
  return QVariant(s);
}

QVariant
boolVariant(bool b)
{
  return QVariant(b);
}

QVariant
colorVariant(const QColor &c)
{
  return QVariant(c);
}

#if 0
QVariant
nullVariant()
{
  return QVariant();
}
#endif

QVariant
nanVariant()
{
  return realVariant(CMathUtil::getNaN());
}

//---

bool
isInteger(double r)
{
  return CMathUtil::isLong(r);
}

}
