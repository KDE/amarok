//Copyright: see the COPYING file that comes with this distribution

#ifndef _AMAROK_LIBS_EXPORT_H
#define _AMAROK_LIBS_EXPORT_H

#include <kdemacros.h>

#ifdef Q_WS_WIN

#ifndef AMAROK_TAGLIB_EXPORT
# ifdef MAKE_AMAROK_TAGLIB_LIB
#  define AMAROK_TAGLIB_EXPORT KDE_EXPORT
# else
#  define AMAROK_TAGLIB_EXPORT KDE_IMPORT
# endif
#endif


#else //not windows

#define AMAROK_TAGLIB_EXPORT KDE_EXPORT

#endif //not windows

#endif // _AMAROK_LIBS_EXPORT_H
