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

#include "amarok.h"
#include "sqlmeta.h"
#include "sqlregistry.h"
#include "sqlcollection.h"

#include "mountpointmanager.h"

#include <QFile>
#include <QListIterator>
#include <QMutexLocker>

#include <klocale.h>

SqlTrack::SqlTrack( SqlCollection* collection, const QStringList &result )
    : Track()
    , m_collection( collection )
{
    int deviceid = result[0].toInt();
    QString rpath = result[1];
    m_url = KUrl( MountPointManager::instance()->getAbsolutePath( deviceid, rpath ) );
    m_title = result[2];
    m_comment = result[3];
    m_trackNumber = result[4].toInt();
    m_discNumber = result[5].toInt();
    m_score = result[6].toDouble();
    m_rating = result[7].toInt();
    m_bitrate = result[8].toInt();
    m_length = result[9].toInt();
    m_filesize = result[10].toInt();
    m_sampleRate = result[11].toInt();
    //create date
    m_lastPlayed = result[13].toUInt();
    m_playCount = result[14].toInt();
    //file type
    //BPM

    SqlRegistry* registry = m_collection->registry();
    m_artist = registry->getArtist( result[17], result[18].toInt() );
    m_album = registry->getAlbum( result[19], result[20].toInt() );
    //isCompilation
    m_genre = registry->getGenre( result[21], result[22].toInt() );
    m_composer = registry->getComposer( result[24], result[25].toInt() );
    m_year = registry->getYear( result[26], result[27].toInt() );
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
SqlTrack::fullPrettyName() const
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


QString
SqlTrack::prettyName() const
{
    //FIXME: This should handle cases when name() is empty!
    return name();
}

void
SqlTrack::setArtist( const QString &newArtist )
{
    //invalidate cache of the old artist...
    m_artist->invalidateCache();
    m_artist = m_collection->registry()->getArtist( newArtist );
    //and the new one
    m_artist->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setGenre( const QString &newGenre )
{
    m_genre->invalidateCache();
    m_genre = m_collection->registry()->getGenre( newGenre );
    m_genre->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setComposer( const QString &newComposer )
{
    m_composer->invalidateCache();
    m_composer = m_collection->registry()->getComposer( newComposer );
    m_composer->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setYear( const QString &newYear )
{
    m_year->invalidateCache();
    m_year = m_collection->registry()->getYear( newYear );
    m_year->invalidateCache();
    notifyObservers();
}

void
SqlTrack::setAlbum( const QString &newAlbum )
{
    m_album->invalidateCache();
    m_album = m_collection->registry()->getAlbum( newAlbum );
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

void
SqlTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
    notifyObservers();
}

/*void
SqlTrack::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valComment );

    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valRating );

    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBitrate );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valLength );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFilesize );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valSamplerate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valCreateDate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valAccessDate );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valFileType );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valBPM );

    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valIsCompilation );
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName );
}*/

//---------------------- class Artist --------------------------

SqlArtist::SqlArtist( SqlCollection* collection, int id, const QString &name ) : Artist()
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_tracks( )
    ,m_mutex( )
    ,m_collection( collection )
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
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}


QString
SqlArtist::sortableName() const
{
    if ( m_modifiedName.isEmpty() && !m_name.isEmpty() ) {
        if ( m_name.startsWith( "the ", Qt::CaseInsensitive ) ) {
            QString begin = m_name.left( 3 );
            m_modifiedName = QString( "%1, %2" ).arg( m_name, begin );
            m_modifiedName = m_modifiedName.mid( 4 );
        }
        else {
            m_modifiedName = m_name;
        }
    }
    return m_modifiedName;
}

/*void
SqlArtist::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
}*/

//---------------SqlAlbum---------------------------------

SqlAlbum::SqlAlbum( SqlCollection* collection, int id, const QString &name ) : Album()
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
    ,m_collection( collection )
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
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlComposer---------------------------------

SqlComposer::SqlComposer( SqlCollection* collection, int id, const QString &name ) : Composer()
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
    ,m_collection( collection )
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
        
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlGenre---------------------------------

SqlGenre::SqlGenre( SqlCollection* collection, int id, const QString &name ) : Genre()
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
    ,m_collection( collection )
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
        
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}

//---------------SqlYear---------------------------------

SqlYear::SqlYear( SqlCollection* collection, int id, const QString &name ) : Year()
    ,m_name( name )
    ,m_id( id )
    ,m_tracksLoaded( false )
    ,m_mutex()
    ,m_tracks()
    ,m_collection( collection )
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
        
        //build query, create SqlTrack objects and add to tracklist
        m_tracksLoaded = true;
        return m_tracks;
    }
}


