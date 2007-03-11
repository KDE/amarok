/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "querybuilder.h"
#include "sqlmeta.h"
#include "sqlregistry.h"

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
SqlTrack::prettyName() const
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
    //invalidate cache of the old artist...
    m_artist->invalidateCache();
    m_artist = SqlRegistry::instance()->getArtist( newArtist );
    //and the new one
    m_artist->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setGenre( const QString &newGenre )
{
    m_genre->invalidateCache();
    m_genre = SqlRegistry::instance()->getGenre( newGenre );
    m_genre->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setComposer( const QString &newComposer )
{
    m_composer->invalidateCache();
    m_composer = SqlRegistry::instance()->getComposer( newComposer );
    m_composer->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setYear( const QString &newYear )
{
    m_year->invalidateCache();
    m_year = SqlRegistry::instance()->getYear( newYear );
    m_year->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setAlbum( const QString &newAlbum )
{
    m_album->invalidateCache();
    m_album = SqlRegistry::instance()->getAlbum( newAlbum );
    m_album->invalidateCache();
    notifyObservers();
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

void
SqlTrack::setScore( double newScore )
{
    m_score = newScore;
    notifyObservers();
}

void
SqlTrack::setRating( int newRating )
{
    m_rating = newRating;
    notifyObservers();
}

void
SqlTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber= newTrackNumber;
    notifyObservers();
}

void
SqlTrack::setDiscNumber( int newDiscNumber )
{
    m_discNumber = newDiscNumber;
    notifyObservers();
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
    m_tracks.clear();
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
    m_tracks.clear();
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
    m_tracks.clear();
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
    m_tracks.clear();
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

//---------------SqlYear---------------------------------

SqlYear::SqlYear( const QString &name ) : Year()
    ,m_name( name )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
{
    //nothing to do
}

void
SqlYear::invalidateCache()
{
    m_mutex.lock();
    m_tracksLoaded = false;
    m_tracks.clear();
    m_mutex.unlock();
}

TrackList
SqlYear::tracks()
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
