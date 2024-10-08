set(CMAKE_REQUIRED_LIBRARIES Qt::Core)
check_cxx_source_compiles("
#include <QtGlobal>
int main()
{
#if defined(QT_NO_GLIB)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}"
QT5_GLIB_SUPPORT)
