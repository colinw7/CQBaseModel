#ifndef CQAlignVariant_H
#define CQAlignVariant_H

#include <CQUtilMeta.h>

class CQAlignVariant {
 private:
  static int s_metaTypeId;

 public:
  static int getMetaType();

  static QString alignToString(Qt::Alignment);
  static Qt::Alignment alignFromString(const QString &s);

  //---

  CQUTIL_DEF_META_CONVERSIONS(CQAlignVariant, s_metaTypeId)

 public:
  CQAlignVariant() = default;

  explicit CQAlignVariant(const QString &str) {
    (void) fromString(str);
  }

  CQAlignVariant(const CQAlignVariant &rhs) = default;

 ~CQAlignVariant() = default;

  CQAlignVariant &operator=(const CQAlignVariant &rhs) = default;

  //---

  QString toString() const { return alignToString(align_); }

  bool fromString(const QString &s) {
    align_ = alignFromString(s);
    return true;
  }

  //---

 private:
  Qt::Alignment align_ { Qt::AlignCenter };
};

//---

CQUTIL_DCL_META_TYPE(CQAlignVariant)

#endif
