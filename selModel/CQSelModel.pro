LIBNAME = CQSelModel

include($$(MAKE_DIR)/qt_lib.mk)

SOURCES += \
CQSelModel.cpp \
CQSelView.cpp \
CQSelDelegate.cpp \

HEADERS += \
CQSelModel.h \
CQSelView.h \
CQSelDelegate.h \

INCLUDEPATH += \
