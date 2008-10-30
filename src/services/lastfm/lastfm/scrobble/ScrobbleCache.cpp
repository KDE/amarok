/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "ScrobbleCache.h"
#include "lastfm/core/CoreDir.h"
#include <QFile>
#include <QDomElement>
#include <QDomDocument>



ScrobbleCache::ScrobbleCache( const QString& username )
{
    Q_ASSERT( username.length() );

    m_path = CoreDir::data().filePath( username + "_subs_cache.xml" );
    m_username = username;
    read();
}


void
ScrobbleCache::read()
{
    m_tracks.clear();

    QFile file( m_path );
    file.open( QFile::Text | QFile::ReadOnly );
    QTextStream stream( &file );
    stream.setCodec( "UTF-8" );

    QDomDocument xml;
    xml.setContent( stream.readAll() );

    for (QDomNode n = xml.documentElement().firstChild(); !n.isNull(); n = n.nextSibling())
        if (n.nodeName() == "item")
            m_tracks += Track( n.toElement() );
}


void
ScrobbleCache::write()
{
    if (m_tracks.isEmpty())
    {
        QFile::remove( m_path );
    }
    else {
        QDomDocument xml;
        QDomElement e = xml.createElement( "submissions" );
        e.setAttribute( "product", "Last.fm Audioscrobbler" );
        e.setAttribute( "version", "2" );

        foreach (Track i, m_tracks)
            e.appendChild( i.toDomElement( xml ) );

        xml.appendChild( e );

        QFile file( m_path );
        file.open( QIODevice::WriteOnly | QIODevice::Text );

        QTextStream stream( &file );
        stream.setCodec( "UTF-8" );
        stream << "<?xml version='1.0' encoding='utf-8'?>\n";
        stream << xml.toString( 2 );
    }
}


void
ScrobbleCache::add( const Scrobble& track )
{
    if (track.isValid())
        add( QList<Track>() << track );
}


void
ScrobbleCache::add( const QList<Track>& tracks )
{
    foreach (const Track& track, tracks)
    {
        if (track.isNull()) 
            qDebug() << "Will not cache an empty track";
        else
            m_tracks += track;
    }
    write();
}


int
ScrobbleCache::remove( const QList<Track>& toremove )
{
    QMutableListIterator<Track> i( m_tracks );
    while (i.hasNext()) {
        Track t = i.next();
        for (int x = 0; x < toremove.count(); ++x)
            if (toremove[x] == t)
                i.remove();
    }

    write();

    // yes we return # remaining, rather # removed, but this is an internal 
    // function and the behaviour is documented so it's alright imo --mxcl
    return m_tracks.count();
}
