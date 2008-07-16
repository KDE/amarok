set (CMAKE_REQUIRED_INCLUDES ${QT_QTCORE_INCLUDE_DIR})
CHECK_CXX_SOURCE_COMPILES("
#include <QtGlobal>
#ifdef QT_NO_GLIB
#error \"Qt was compiled with Glib disabled\"
#endif", QT4_GLIB_SUPPORT)
