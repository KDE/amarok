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

#include "sqlregistry.h"

#include <QMutexLocker>

SqlRegistry *
SqlRegistry::instance()
{
    static SqlRegistry instance;
    return &instance;
}

SqlRegistry::SqlRegistry() : QObject( 0 )
{
    setObjectName( "SqlRegistry" );
}

TrackPtr
SqlRegistry::getTrack( const QString &url )
{
    QMutexLocker locker( &m_trackMutex );
    if( m_trackMap.contains( url ) )
        return m_trackMap.value( url );
    else
    {
        TrackPtr track( new SqlTrack( QStringList() ) );
        m_trackMap.insert( url, track );
        return track;
    }
}

ArtistPtr
SqlRegistry::getArtist( const QString &name )
{
    QMutexLocker locker( &m_artistMutex );
    if( m_artistMap.contains( name ) )
        return m_artistMap.value( name );
    else
    {
        ArtistPtr artist( new SqlArtist( name ) );
        m_artistMap.insert( name, artist );
        return artist;
    }
}

GenrePtr
SqlRegistry::getGenre( const QString &name )
{
    QMutexLocker locker( &m_genreMutex );
    if( m_genreMap.contains( name ) )
        return m_genreMap.value( name );
    else
    {
        GenrePtr genre( new SqlGenre( name ) );
        m_genreMap.insert( name, genre );
        return genre;
    }
}

ComposerPtr
SqlRegistry::getComposer( const QString &name )
{
    QMutexLocker locker( &m_composerMutex );
    if( m_composerMap.contains( name ) )
        return m_composerMap.value( name );
    else
    {
        ComposerPtr composer( new SqlComposer( name ) );
        m_composerMap.insert( name, composer );
        return composer;
    }
}

YearPtr
SqlRegistry::getYear( const QString &name )
{
    QMutexLocker locker( &m_yearMutex );
    if( m_yearMap.contains( name ) )
        return m_yearMap.value( name );
    else
    {
        YearPtr year( new SqlYear( name ) );
        m_yearMap.insert( name, year );
        return year;
    }
}

AlbumPtr
SqlRegistry::getAlbum( const QString &name )
{
    QMutexLocker locker( &m_albumMutex );
    if( m_albumMap.contains( name ) )
        return m_albumMap.value( name );
    else
    {
        AlbumPtr album( new SqlAlbum( name ) );
        m_albumMap.insert( name, album );
        return album;
    }
}

void
SqlRegistry::emptyCache()
{
    bool hasTrack, hasAlbum, hasArtist, hasYear, hasGenre, hasComposer;
    hasTrack = hasAlbum = hasArtist = hasYear = hasGenre = hasComposer = false;

    //try to avoid possible deadlocks by aborting when we can't get all locks
    if ( ( hasTrack = m_trackMutex.tryLock() )
         && ( hasAlbum = m_albumMutex.tryLock() )
         && ( hasArtist = m_artistMutex.tryLock() )
         && ( hasYear = m_yearMutex.tryLock() )
         && ( hasGenre = m_genreMutex.tryLock() )
         && ( hasComposer = m_composerMutex.tryLock() ) )
    {
        //do stuff
        
    }

    //make sure to unlock all necessary locks
    //important: calling unlock() on an unlocked mutex gives an undefined result
    //unlocking a mutex locked by another thread results in an error, so be careful
    if( hasTrack ) m_trackMutex.unlock();
    if( hasAlbum ) m_albumMutex.unlock();
    if( hasArtist ) m_artistMutex.unlock();
    if( hasYear ) m_yearMutex.unlock();
    if( hasGenre ) m_genreMutex.unlock();
    if( hasComposer ) m_composerMutex.unlock();
}

#include "sqlregistry.moc"
