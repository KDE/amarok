CHECK_CXX_SOURCE_COMPILES("
#include <QtCore/QtGlobal>
#ifdef QT_NO_GLIB
#error \"Qt was compiled with Glib disabled\"
#endif", QT4_GLIB_SUPPORT)
