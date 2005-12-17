// Copyright:  See COPYING file that comes with this distribution

#ifndef _AMAROK_EXPORT_H_
#define _AMAROK_EXPORT_H_

#include <config.h>

#ifdef __KDE_HAVE_GCC_VISIBILITY
#define LIBAMAROK_NO_EXPORT __attribute__ ((visibility("hidden")))
#define LIBAMAROK_EXPORT __attribute__ ((visibility("default")))
#else
#define LIBAMAROK_NO_EXPORT
#define LIBAMAROK_EXPORT
#endif
 
#endif

