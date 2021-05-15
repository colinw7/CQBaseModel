TEMPLATE = lib

TARGET = CQBaseModel

DEPENDPATH += .

QT += widgets

CONFIG += staticlib

QMAKE_CXXFLAGS += -std=c++14

MOC_DIR = .moc

SOURCES += \
CQBaseModel.cpp \
CQDataModel.cpp \
CQModelDetails.cpp \
CQModelNameValues.cpp \
CQModelUtil.cpp \
CQModelVisitor.cpp \
CQSortModel.cpp \
CQValueSet.cpp \

HEADERS += \
../include/CQBaseModel.h \
../include/CQBaseModelTypes.h \
../include/CQDataModel.h \
../include/CQModelDetails.h \
../include/CQModelNameValues.h \
../include/CQModelUtil.h \
../include/CQModelVisitor.h \
../include/CQSortModel.h \
../include/CQStatData.h \
../include/CQValueSet.h \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += \
../include \
../../CQUtil/include \
../../CUtil/include \
../../CFont/include \
../../CMath/include \
../../COS/include \
