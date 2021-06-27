#include <CQModelUtil.h>
#include <CQModelVisitor.h>
#include <QSortFilterProxyModel>

namespace CQModelUtil {

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
  return modelHeaderValue(model, c, orientation, (int) role, ok);
}

QVariant
modelHeaderValue(const QAbstractItemModel *model, int c, int role, bool &ok)
{
  return modelHeaderValue(model, c, Qt::Horizontal, role, ok);
}

QVariant
modelHeaderValue(const QAbstractItemModel *model, int c, CQBaseModelRole role, bool &ok)
{
  return modelHeaderValue(model, c, (int) role, ok);
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

  if (ok)
    i = var.toInt(&ok);

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

  auto var = model->headerData(c, Qt::Horizontal, (int) CQBaseModelRole::Type);
  if (! var.isValid()) return false;

  bool ok;
  type = (CQBaseModelType) var.toInt(&ok);
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
  else if (name == "type"              ) return (int) CQBaseModelRole::Type;
  else if (name == "base_type"         ) return (int) CQBaseModelRole::BaseType;
  else if (name == "type_values"       ) return (int) CQBaseModelRole::TypeValues;
  else if (name == "min"               ) return (int) CQBaseModelRole::Min;
  else if (name == "max"               ) return (int) CQBaseModelRole::Max;
  else if (name == "sorted"            ) return (int) CQBaseModelRole::Sorted;
  else if (name == "sort_order"        ) return (int) CQBaseModelRole::SortOrder;
  else if (name == "title"             ) return (int) CQBaseModelRole::Title;
  else if (name == "tip"               ) return (int) CQBaseModelRole::Tip;
  else if (name == "key"               ) return (int) CQBaseModelRole::Key;
  else if (name == "raw_value"         ) return (int) CQBaseModelRole::RawValue;
  else if (name == "intermediate_value") return (int) CQBaseModelRole::IntermediateValue;
  else if (name == "cached_value"      ) return (int) CQBaseModelRole::CachedValue;
  else if (name == "output_value"      ) return (int) CQBaseModelRole::OutputValue;
  else if (name == "group"             ) return (int) CQBaseModelRole::Group;
  else if (name == "format"            ) return (int) CQBaseModelRole::Format;
  else if (name == "iformat"           ) return (int) CQBaseModelRole::IFormat;
  else if (name == "oformat"           ) return (int) CQBaseModelRole::OFormat;
  else if (name == "data_min"          ) return (int) CQBaseModelRole::DataMin;
  else if (name == "data_max"          ) return (int) CQBaseModelRole::DataMax;
  else if (name == "header_type"       ) return (int) CQBaseModelRole::HeaderType;
  else if (name == "header_type_values") return (int) CQBaseModelRole::HeaderTypeValues;
  else if (name == "fill_color"        ) return (int) CQBaseModelRole::FillColor;
  else if (name == "symbol_type"       ) return (int) CQBaseModelRole::SymbolType;
  else if (name == "symbol_size"       ) return (int) CQBaseModelRole::SymbolSize;
  else if (name == "font_size"         ) return (int) CQBaseModelRole::FontSize;
  else if (name == "style"             ) return (int) CQBaseModelRole::Style;
  else if (name == "export"            ) return (int) CQBaseModelRole::Export;

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

}
