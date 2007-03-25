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
#include "../querybuilder.h"
#include "sqlmeta.h"
#include "sqlregistry.h"

#include <QFile>
#include <QListIterator>
#include <QMutexLocker>

#include <klocale.h>

SqlTrack::SqlTrack( const QStringList &result ) : Track()
{
    SqlRegistry *registry = SqlRegistry::instance();
    m_url = KUrl( result[0] );
    m_title = result[1];
    m_comment = result[2];
    m_trackNumber = result[3].toInt();
    m_discNumber = result[4].toInt();
    m_score = result[5].toDouble();
    m_rating = result[6].toInt();
    m_bitrate = result[7].toInt();
    m_length = result[8].toInt();
    m_filesize = result[9].toInt();
    m_sampleRate = result[10].toInt();
    //create date
    m_lastPlayed = result[12].toUInt();
    m_playCount = result[13].toInt();
    //file type
    //BPM
    m_artist = registry->getArtist( result[16] );
    m_album = registry->getAlbum( result[17] );
    //isCompilation
    m_genre = registry->getGenre( result[19] );
    m_composer = registry->getComposer( result[20] );
    m_year = registry->getYear( result[21] );
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

void
SqlTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
    notifyObservers();
}


void
SqlTrack::addToQueryFilter( QueryBuilder &qb ) const {
    //FIXME: implement me!
}

void
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

void
SqlArtist::addToQueryFilter( QueryBuilder &qb ) const {
    qb.addMatch( QueryBuilder::tabArtist, m_name, false, true );
}


void
SqlArtist::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName, true );
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

void
SqlAlbum::addToQueryFilter( QueryBuilder &qb ) const {
    qb.addMatch( QueryBuilder::tabAlbum, m_name, false, true );
}

void
SqlAlbum::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName, true );
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

void
SqlComposer::addToQueryFilter( QueryBuilder &qb ) const {
    qb.addMatch( QueryBuilder::tabComposer, m_name, false, true );
}

void
SqlComposer::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabComposer, QueryBuilder::valName, true );
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

void
SqlGenre::addToQueryFilter( QueryBuilder &qb ) const {
    qb.addMatch( QueryBuilder::tabGenre, m_name, false, true );
}

void
SqlGenre::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName, true );
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

void
SqlYear::addToQueryFilter( QueryBuilder &qb ) const {
    qb.addMatch( QueryBuilder::tabYear, m_name, false, true );
}

void
SqlYear::addToQueryResult( QueryBuilder &qb ) {
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabYear, QueryBuilder::valName, true );
}
