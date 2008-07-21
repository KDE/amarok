find_package(Qt4)
include(CheckSymbolExists)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
CHECK_SYMBOL_EXISTS(QT_KDE_QT_COPY qglobal.h QT_KDE_QT_COPY)
if(QT_KDE_QT_COPY)
    add_subdirectory( generator )
endif(QT_KDE_QT_COPY)

