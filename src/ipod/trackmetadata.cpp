/***************************************************************************
 *   Copyright (C) 2004 by Michael Schulze                                 *
 *   mike.s@genion.de                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "trackmetadata.h"
#include "tracklist.h"

#include <qfileinfo.h>
#include <qstringlist.h>


TrackMetadata::TrackMetadata()
    : itunesdb::Track()
{
}


TrackMetadata::TrackMetadata(Q_UINT32 trackid)
    : itunesdb::Track()
{
}


TrackMetadata::TrackMetadata(const Track& track)
    : itunesdb::Track(track)
{
}

TrackMetadata::~TrackMetadata()
{
}

/*!
    \fn TrackMetadata::getFileExtension()
 */
const QString& TrackMetadata::getFileExtension() const
{
    return file_extension;
}

/*!
    \fn TrackMetadata::setFileExtension( QString& extension)
 */
void TrackMetadata::setFileExtension(const QString& extension)
{
    file_extension= extension;
}


/*!
    \fn TrackMetadata::readFromFile(const QString& filename)
 */
bool TrackMetadata::readFromFile(const QString& filename)
{
}


/*!
    \fn TrackMetadata::toLogEntry()
 */
QStringList& TrackMetadata::toLogEntry(QStringList& valuebuffer) const
{
}


/*!
    \fn TrackMetadata::readFromLogEntry()
 */
bool TrackMetadata::readFromLogEntry(const QStringList& values)
{
}

