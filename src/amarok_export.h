// Copyright:  See COPYING file that comes with this distribution

// Do not add new exports to this file if you can help it.
// Use amarok_libs_export.h or amarok_engines_export.h as touching
// this file forces a largeish rebuild.
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
