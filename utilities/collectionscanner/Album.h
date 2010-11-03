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

#include <QList>
#include <QString>
#include <QStringList>

#include "Track.h"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace CollectionScanner
{

/**
 * @class Album
 * @short Represents a scanned album and it's contents
 */

class Album
{
public:
    Album();
    Album( const QString &name );
    Album( QXmlStreamReader *reader );

    void append( const Track &track );

    /** Copies all tracks and covers from the otherAlbum to this one.
     *  This can be done when one has to merge different albums into one compilation.
     */
    void merge( const Album &otherAlbum );

    void addCovers( const QStringList &covers );

    QString name() const;

    /** Returns the artist of this album.
     */
    QString artist() const;

    /** Returns true if this album is a compilation (an album with different artists) */
    bool isCompilation() const;

    /** Returns true if this album is not a compilation. */
    bool isNoCompilation() const;

    /** Returns the picture best suited as cover for this album */
    QString cover() const;

    QList<Track> tracks() const;

#ifdef UTILITIES_BUILD
    /** Writes the contents of this object to an xml stream.
     *  Only the content is writen and no enclosing directory tags.
     *  This is done to make it mirror the constructor which does not read those
     *  tags either.
     */
    void toXml( QXmlStreamWriter *writer ) const;
#endif // UTILITIES_BUILD

private:
    QString m_name;
    QStringList m_covers;
    QList<Track> m_tracks;
};

}

#endif // COLLECTIONSCANNER_ALBUM_H

