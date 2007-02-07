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

#else // not windows

#define AMAROK_EXPORT KDE_EXPORT
#endif /* not windows */

#endif /* _AMAROK_EXPORT_H */
