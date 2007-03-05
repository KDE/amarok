/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "collectiondb.h"
#include "sqlmeta.h"

#include <QFile>
#include <QListIterator>
#include <QMutexLocker>

#include <klocale.h>

SqlTrack::SqlTrack( const QStringList &result ) : Track()
{
    //TODO
}

bool
SqlTrack::isPlayable() const
{
    return QFile::exists( m_url.path() );
}

bool
SqlTrack::isEditable() const
{
    return QFile::exists( m_url.path() ) && QFile::permissions( m_url.path() ) & QFile::WriteUser;
}

QString
SqlTrack::type() const
{
    return m_url.isLocalFile()
           ? m_url.fileName().mid( m_url.fileName().lastIndexOf( '.' ) + 1 )
           : i18n( "Stream" );
}

QString
SqlTrack::prettyTitle() const
{
    QString s = m_artist->name();

    //NOTE this gets regressed often, please be careful!
    //     whatever you do, handle the stream case, streams have no artist but have an excellent title

    //FIXME doesn't work for resume playback

    if( s.isEmpty() )
        s = name();
    else
        s = i18n("%1 - %2", m_artist->name(), name() );

    //TODO 
    if( s.isEmpty() ) s = prettyTitle( m_url.fileName() );

    return s;
}

QString
SqlTrack::prettyTitle( const QString &filename ) //static
{
    QString s = filename; //just so the code is more readable

    //remove .part extension if it exists
    if (s.endsWith( ".part" ))
        s = s.left( s.length() - 5 );

    //remove file extension, s/_/ /g and decode %2f-like sequences
    s = s.left( s.lastIndexOf( '.' ) ).replace( '_', ' ' );
    s = QUrl::fromPercentEncoding( s.toAscii() );

    return s;
}

void
SqlTrack::setArtist( const QString &newArtist )
{
    //TODO get new artist from registry and set it
    m_artist->invalidateCache();
}

void
SqlTrack::subscribe( TrackObserver *observer )
{
    if( !m_observers.contains( observer ) )
        m_observers.append( observer );
}

void
SqlTrack::unsubscribe( TrackObserver *observer )
{
    m_observers.removeAll( observer );
}

void
SqlTrack::notifyObservers()
{
    for( QListIterator<TrackObserver*> iter( m_observers ) ; iter.hasNext(); )
        iter.next()->metadataChanged( this );
}

//---------------------- class Artist --------------------------

SqlArtist::SqlArtist( const QString &name ) : Artist()
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_tracks( )
    ,m_mutex( )
{
    //nothing to do (yet)
}

void
SqlArtist::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks = TrackList();
    m_mutex.unlock();
}

TrackList
SqlArtist::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryBuilder qb;
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlAlbum---------------------------------

SqlAlbum::SqlAlbum( const QString &name ) : Album()
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
{
    //nothing to do
}

void
SqlAlbum::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks = TrackList();
    m_mutex.unlock();
}

TrackList
SqlAlbum::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryBuilder qb;
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlComposer---------------------------------

SqlComposer::SqlComposer( const QString &name ) : Composer()
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
{
    //nothing to do
}

void
SqlComposer::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks = TrackList();
    m_mutex.unlock();
}

TrackList
SqlComposer::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryBuilder qb;
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlGenre---------------------------------

SqlGenre::SqlGenre( const QString &name ) : Genre()
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
{
    //nothing to do
}

void
SqlGenre::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks = TrackList();
    m_mutex.unlock();
}

TrackList
SqlGenre::tracks()
{
    QMutexLocker locker( &m_mutex );
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else
    {
        QueryBuilder qb;
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}
