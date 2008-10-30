/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFM_DLL_EXPORT_MACRO_H
#define LASTFM_DLL_EXPORT_MACRO_H

#include <kdemacros.h>

#ifndef LASTFM_EXPORT
# if defined(MAKE_AMAROK_SERVICE_LIBLASTFM_LIB)
   /* We are building this library */ 
#  define LASTFM_EXPORT KDE_EXPORT
# else
   /* We are using this library */ 
#  define LASTFM_EXPORT KDE_IMPORT
# endif
#endif

#define LASTFM_RADIO_DLLEXPORT LASTFM_EXPORT
#define LASTFM_FINGERPRINT_DLLEXPORT LASTFM_EXPORT
#define LASTFM_WS_DLLEXPORT LASTFM_EXPORT
#define LASTFM_CORE_DLLEXPORT LASTFM_EXPORT
#define LASTFM_TYPES_DLLEXPORT LASTFM_EXPORT
#define LASTFM_SCROBBLE_DLLEXPORT LASTFM_EXPORT

#endif
