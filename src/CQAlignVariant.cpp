#include <CQAlignVariant.h>
#include <CQUtil.h>

CQUTIL_DEF_META_TYPE(CQAlignVariant, toString, fromString)

int CQAlignVariant::s_metaTypeId;

int
CQAlignVariant::
getMetaType()
{
  if (! s_metaTypeId)
    s_metaTypeId = CQUTIL_REGISTER_META(CQAlignVariant);

  return s_metaTypeId;
}

QString
CQAlignVariant::
alignToString(Qt::Alignment align)
{
  return CQUtil::alignToString(align);
}

Qt::Alignment
CQAlignVariant::
alignFromString(const QString &str)
{
  Qt::Alignment align;

  if (! CQUtil::stringToAlign(str, align))
    return Qt::Alignment();

  return align;
}
