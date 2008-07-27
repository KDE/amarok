/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFMMOOSECOMMON_H
#define LASTFMMOOSECOMMON_H

// try to convert last.fm's debug output as best we can
#define DEBUG_PREFIX "lastfm"
#include <qdebug.h>
#include "logger.h"
#include "Debug.h"
#ifdef qDebug
#undef qDebug
#endif
#define qDebug() debug()

#include "Amarok.h"

namespace MooseUtils
{
    inline QString savePath( const QString &file ) { return Amarok::saveLocation() + file; }
}

#endif // LASTFMMOOSECOMMON_H
