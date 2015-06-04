set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
set(CMAKE_REQUIRED_DEFINITIONS "")
set(CMAKE_REQUIRED_FLAGS "")
check_cxx_source_compiles("
#include <QtCore/QtGlobal>
int main()
{
#if defined(QT_NO_GLIBASDF)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}" 
QT4_GLIB_SUPPORT)

