// Copyright:  See COPYING file that comes with this distribution

#ifndef _AMAROK_EXPORT_H_
#define _AMAROK_EXPORT_H_

#include <kdemacros.h>

#ifdef Q_WS_WIN

#ifndef AMAROK_EXPORT
# ifdef MAKE_AMAROK_LIB
#  define AMAROK_EXPORT KDE_EXPORT
# else
#  define AMAROK_EXPORT KDE_IMPORT
# endif
#endif

#ifndef AMAROK_TAGLIB_EXPORT
# ifdef MAKE_AMAROK_TAGLIB_LIB
#  define AMAROK_TAGLIB_EXPORT KDE_EXPORT
# else
#  define AMAROK_TAGLIB_EXPORT KDE_IMPORT
# endif
#endif

#else // not windows

#define AMAROK_EXPORT KDE_EXPORT
#define AMAROK_TAGLIB_EXPORT KDE_IMPORT
#endif /* not windows */

#endif /* _AMAROK_EXPORT_H */
