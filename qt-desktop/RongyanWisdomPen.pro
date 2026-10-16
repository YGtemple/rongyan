QT       += core gui widgets network
# network 模块：请求 HiAgent 工作流 API

CONFIG += c++17
CONFIG += console

QMAKE_CXXFLAGS += -Wall

# 修复：禁用中文 TARGET！qmake 对非 ASCII 输出名支持不稳定
TARGET = EnglishWriter
TEMPLATE = app

VERSION = 1.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# ============================================================
# 构建说明
# ============================================================
#   qmake RongyanWisdomPen.pro
#   make            # Windows: nmake / mingw32-make
#
# 依赖：Qt 6.x（Core / Gui / Widgets / Network）
# 运行：需自行申请 HiAgent 工作流，并把 mainwindow.cpp 中的
#       YOUR_HIAGENT_API_KEY 与接口地址替换为实际值。
# ============================================================
