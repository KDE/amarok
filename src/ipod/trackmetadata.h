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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TRACKMETADATA_H
#define TRACKMETADATA_H

#include <qstringlist.h>

#include "itunesdb/track.h"

/**
Holds track metadata information: Additional to the information itunesdb::Track delivers this cllas holds information about the file extension, type etc.

@author Michael Schulze
*/


class MetaBundle;
class TrackList;

class TrackMetadata : public itunesdb::Track
{
    public:
        TrackMetadata();
        TrackMetadata(Q_UINT32 trackid);
        TrackMetadata(const Track& track);

        ~TrackMetadata();
        const QString& getFileExtension() const;
        void setFileExtension(const QString& extension);
        bool readFromBundle( const MetaBundle& bundle );

        QStringList& toLogEntry(QStringList& valuebuffer) const;
        bool readFromLogEntry(const QStringList& values);

    private:
        QString file_extension;
};

#endif
