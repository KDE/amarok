/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                  *
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

#include "Album.h"

#include "Track.h"

#include <QDebug>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// constructor is needed to put albums in a hash
CollectionScanner::Album::Album()
{}

CollectionScanner::Album::Album( const QString &name, const QString &artist )
    : m_name( name )
    , m_artist( artist )
{}

void
CollectionScanner::Album::addTrack( Track *track )
{
    m_tracks.append( track );
}

QString
CollectionScanner::Album::name() const
{
    return m_name;
}

QString
CollectionScanner::Album::artist() const
{
    return m_artist;
}

void
CollectionScanner::Album::setArtist( const QString &artist )
{
    m_artist = artist;
}


QString
CollectionScanner::Album::cover() const
{
    // we prefer covers included in tracks.
    // At least we know exactly that they really belong to the album
    for( const auto &track : m_tracks )
    {
        // IMPROVEMENT: skip covers that have a strange aspect ratio or are
        // unrealistically small, or do not resolve to a valid image
        if( track->hasCover() )
            return QStringLiteral("amarok-sqltrackuid://") + track->uniqueid();
    }

    // ok. Now we have to figure out which of the cover images is
    // the best.
    QString bestCover;
    int     bestRating = -1;
    qint64  bestSize = 0;

    for( const auto &cover : m_covers )
    {
        int rating = 0;

        if( cover.contains( QLatin1String("front"), Qt::CaseInsensitive ) ||
            cover.contains( QObject::tr( "front", "Front cover of an album" ), Qt::CaseInsensitive ) )
            rating += 2;

        if( cover.contains( QLatin1String("cover"), Qt::CaseInsensitive ) ||
            cover.contains( QObject::tr( "cover", "(Front) Cover of an album" ), Qt::CaseInsensitive ) )
            rating += 2;

        //next: try "folder" (some applications apparently use this)
        //using compare and not contains to not hit "Folder-Back" or something.
        if( cover.compare( QLatin1String("folder"), Qt::CaseInsensitive ) == 0)
            rating += 1;

        QFileInfo info( cover );
        if( (rating == bestRating && info.size() > bestSize) ||
            (rating > bestRating) )
        {
            bestCover = cover;
            bestRating = rating;
            bestSize = info.size();
        }
    }

    return bestCover;
}

QStringList
CollectionScanner::Album::covers() const
{
    return m_covers;
}

void
CollectionScanner::Album::setCovers( const QStringList &covers )
{
    m_covers = covers;
}



QList<CollectionScanner::Track*>
CollectionScanner::Album::tracks() const
{
    return m_tracks;
}

bool
CollectionScanner::Album::isNoCompilation() const
{
    for( const auto &track : m_tracks )
    {
        if( track->isNoCompilation() )
            return true;
    }

    return false;
}
