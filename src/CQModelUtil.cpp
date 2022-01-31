#include <CQModelUtil.h>
#include <CQModelVisitor.h>

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

    State visit(const QAbstractItemModel *model, const VisitData &data) override {
      auto ind = model->index(data.row, column_, data.parent);

      // if column can be integral, check if value is valid integer
      if (isInt_) {
        bool ok;

        (void) modelInteger(model, ind, ok);

        if (ok)
          return State::SKIP;

        auto str = modelString(model, ind, ok);

        if (! str.length())
          return State::SKIP;

        isInt_ = false;
      }

      // if column can be real, check if value is valid real
      if (isReal_) {
        bool ok;

        (void) modelReal(model, ind, ok);

        if (ok)
          return State::SKIP;

        auto str = modelString(model, ind, ok);

        if (! str.length())
          return State::SKIP;

        isReal_ = false;
      }

      // not value real or integer so assume string and we are done
      return State::TERMINATE;
    }

    CQBaseModelType columnType() {
      if      (isInt_ ) return CQBaseModelType::INTEGER;
      else if (isReal_) return CQBaseModelType::REAL;
      else              return CQBaseModelType::STRING;
    }

   private:
    int  column_ { -1 };   // column to check
    bool isInt_  { true }; // could be integeral
    bool isReal_ { true }; // could be real
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

const QStringList &
roleNames()
{
  static QStringList names;

  if (names.empty()) {
    // standard
    names << "display" << "edit" << "user" << "font" << "size_hint" <<
             "tool_tip" << "background" << "foreground" << "text_alignment" <<
             "text_color" << "decoration";

    // custom
    names << "type" << "base_type" << "type_values" << "min" << "max" << "sorted" <<
             "sort_order" << "title" << "tip" << "key" << "raw_value" << "intermediate_value" <<
             "cached_value" << "output_value" << "group" << "format" << "iformat" << "oformat" <<
             "data_min" << "data_max" << "header_type" << "header_type_values" <<
             "fill_color" << "symbol_type" << "symbol_size" << "font_size" << "style" <<
             "export";
  }

  return names;
};

int
nameToRole(const QString &name)
{
  // standard
  if      (name == "display"       ) return Qt::DisplayRole;
  else if (name == "edit"          ) return Qt::EditRole;
  else if (name == "user"          ) return Qt::UserRole;
  else if (name == "font"          ) return Qt::FontRole;
  else if (name == "size_hint"     ) return Qt::SizeHintRole;
  else if (name == "tool_tip"      ) return Qt::ToolTipRole;
  else if (name == "background"    ) return Qt::BackgroundRole;
  else if (name == "foreground"    ) return Qt::ForegroundRole;
  else if (name == "text_alignment") return Qt::TextAlignmentRole;
  else if (name == "text_color"    ) return Qt::TextColorRole;
  else if (name == "decoration"    ) return Qt::DecorationRole;

  // custom
  else if (name == "type"              ) return roleCast(CQBaseModelRole::Type);
  else if (name == "base_type"         ) return roleCast(CQBaseModelRole::BaseType);
  else if (name == "type_values"       ) return roleCast(CQBaseModelRole::TypeValues);
  else if (name == "min"               ) return roleCast(CQBaseModelRole::Min);
  else if (name == "max"               ) return roleCast(CQBaseModelRole::Max);
  else if (name == "sorted"            ) return roleCast(CQBaseModelRole::Sorted);
  else if (name == "sort_order"        ) return roleCast(CQBaseModelRole::SortOrder);
  else if (name == "title"             ) return roleCast(CQBaseModelRole::Title);
  else if (name == "tip"               ) return roleCast(CQBaseModelRole::Tip);
  else if (name == "key"               ) return roleCast(CQBaseModelRole::Key);
  else if (name == "raw_value"         ) return roleCast(CQBaseModelRole::RawValue);
  else if (name == "intermediate_value") return roleCast(CQBaseModelRole::IntermediateValue);
  else if (name == "cached_value"      ) return roleCast(CQBaseModelRole::CachedValue);
  else if (name == "output_value"      ) return roleCast(CQBaseModelRole::OutputValue);
  else if (name == "group"             ) return roleCast(CQBaseModelRole::Group);
  else if (name == "format"            ) return roleCast(CQBaseModelRole::Format);
  else if (name == "iformat"           ) return roleCast(CQBaseModelRole::IFormat);
  else if (name == "oformat"           ) return roleCast(CQBaseModelRole::OFormat);
  else if (name == "data_min"          ) return roleCast(CQBaseModelRole::DataMin);
  else if (name == "data_max"          ) return roleCast(CQBaseModelRole::DataMax);
  else if (name == "header_type"       ) return roleCast(CQBaseModelRole::HeaderType);
  else if (name == "header_type_values") return roleCast(CQBaseModelRole::HeaderTypeValues);
  else if (name == "fill_color"        ) return roleCast(CQBaseModelRole::FillColor);
  else if (name == "symbol_type"       ) return roleCast(CQBaseModelRole::SymbolType);
  else if (name == "symbol_size"       ) return roleCast(CQBaseModelRole::SymbolSize);
  else if (name == "font_size"         ) return roleCast(CQBaseModelRole::FontSize);
  else if (name == "style"             ) return roleCast(CQBaseModelRole::Style);
  else if (name == "export"            ) return roleCast(CQBaseModelRole::Export);

  bool ok;

  int role = name.toInt(&ok);

  if (ok)
    return role;

  return -1;
}

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

QVariant
nullVariant()
{
  return QVariant();
}

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
