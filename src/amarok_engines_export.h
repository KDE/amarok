/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef _AMAROK_ENGINES_EXPORT_H_
#define _AMAROK_ENGINES_EXPORT_H_

#include <kdemacros.h>

#ifdef Q_WS_WIN

#ifndef AMAROK_PHONON_ENGINE_EXPORT
# ifdef MAKE_AMAROK_PHONON-ENGINE_PART
#  define AMAROK_PHONON_ENGINE_EXPORT KDE_EXPORT
# else
#  define AMAROK_PHONON_ENGINE_EXPORT KDE_IMPORT
# endif
#endif

#else //not windows

#define AMAROK_PHONON_ENGINE_EXPORT KDE_EXPORT

#endif //not windows

#endif //AMAROK_ENGINES_EXPORT_H
