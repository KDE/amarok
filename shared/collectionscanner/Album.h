/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COLLECTIONSCANNER_ALBUM_H
#define COLLECTIONSCANNER_ALBUM_H

#include "amarokshared_export.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace CollectionScanner
{
class Track;

/** This album class is used by the ScanResultProcessor to sort tracks into albums.
    @class Album
    @short Represents a scanned album and it's contents
    @author Ralf Engels <ralf-engels@gmx.de>
*/
class AMAROKSHARED_EXPORT Album
{
public:
    Album();
    Album( const QString &name, const QString &artist );

    /** Adds a track to this album.
        The track must still be freed by the caller.
    */
    void addTrack( Track *track );

    QString name() const;

    /** Returns the artist of this album.  */
    QString artist() const;
    void setArtist( const QString &artist );

    /** Returns the picture best suited as cover for this album */
    QString cover() const;

    /** Returns all covers added via addCovers() */
    QStringList covers() const;
    void setCovers( const QStringList &covers );

    QList<Track*> tracks() const;

    bool isNoCompilation() const;

private:
    QString m_name;
    QString m_artist;
    QStringList m_covers;
    QList<Track*> m_tracks;
};

}

#endif // COLLECTIONSCANNER_ALBUM_H
