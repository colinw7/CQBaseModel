TEMPLATE = app

TARGET = CQDataModelTest

QT += widgets

DEPENDPATH += .

QMAKE_CXXFLAGS += \
-std=c++14 \

MOC_DIR = .moc

SOURCES += \
CQDataModelTest.cpp \

HEADERS += \
CQDataModelTest.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj

INCLUDEPATH += \
. \
../include \
../../CQUtil/include \
../../CUtil/include \
../../CFont/include \
../../CMath/include \
../../COS/include \

PRE_TARGETDEPS = \
../lib/libCQBaseModel.a \

unix:LIBS += \
-L../lib \
-L../../CQUtil/lib \
-L../../CFont/lib \
-L../../CImageLib/lib \
-L../../CConfig/lib \
-L../../CUtil/lib \
-L../../CFileUtil/lib \
-L../../CFile/lib \
-L../../CMath/lib \
-L../../CStrUtil/lib \
-L../../CRegExp/lib \
-L../../COS/lib \
-lCQBaseModel -lCQUtil \
-lCFont -lCImageLib -lCConfig -lCUtil \
-lCFileUtil -lCFile -lCMath -lCStrUtil -lCRegExp -lCOS \
-lpng -ljpeg -ltre
