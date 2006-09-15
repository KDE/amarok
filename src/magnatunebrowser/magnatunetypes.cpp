// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution

#include "magnatunetypes.h"


//// MagnatuneArtist ////

MagnatuneArtist::MagnatuneArtist( )
{
}

MagnatuneArtist::~ MagnatuneArtist( )
{
}

MagnatuneArtist::MagnatuneArtist(const MagnatuneArtist& artist)
{
    m_id = artist.getId();
   m_name = artist.getName();
   m_description = artist.getDescription();
   m_photoURL = artist.getPhotoURL();
   m_homeURL = artist.getHomeURL();
}

void MagnatuneArtist::setId( int id )
{
   m_id = id;
}

int MagnatuneArtist::getId( ) const
{
    return m_id;
}

void MagnatuneArtist::setName( QString name )
{
   m_name = name;
}

QString MagnatuneArtist::getName( ) const
{
   return m_name;
}

void MagnatuneArtist::setDescription( QString description )
{
    m_description = description;
}

QString MagnatuneArtist::getDescription( ) const
{ 
   return m_description;
}

void MagnatuneArtist::setPhotoURL( QString photoURL )
{
   m_photoURL = photoURL;
}

QString MagnatuneArtist::getPhotoURL( ) const
{
    return m_photoURL;
}

void MagnatuneArtist::setHomeURL( QString homeURL )
{
    m_homeURL = homeURL;
}

QString MagnatuneArtist::getHomeURL( ) const
{
    return m_homeURL;
}








//// MagnatuneAlbum ////

MagnatuneAlbum::MagnatuneAlbum( )
{
}

MagnatuneAlbum::~ MagnatuneAlbum( )
{
}

MagnatuneAlbum::MagnatuneAlbum(const MagnatuneAlbum& album )
{
    m_id = album.getId();
    m_name = album.getName();
    m_albumCode = album.getAlbumCode();
    m_launchDate = album.getLaunchDate();
    m_coverURL = album.getCoverURL();
    m_mp3Genre = album.getMp3Genre();
    m_magnatuneGenres = album.getMagnatuneGenres();
    m_artistId = album.getArtistId();
}

void MagnatuneAlbum::setId( int id )
{
   m_id = id;
}

int MagnatuneAlbum::getId( ) const
{
    return m_id;
}

void MagnatuneAlbum::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int MagnatuneAlbum::getArtistId( ) const
{
   return m_artistId;
}


void MagnatuneAlbum::setName( QString name )
{
    m_name = name;
}

QString MagnatuneAlbum::getName( ) const
{
   return m_name;
}

void MagnatuneAlbum::setCoverURL( QString coverURL )
{
    m_coverURL = coverURL;
}

QString MagnatuneAlbum::getCoverURL( ) const
{
    return m_coverURL;
}

void MagnatuneAlbum::setLaunchDate( QDate launchDate )
{
    m_launchDate = launchDate;
}

QDate MagnatuneAlbum::getLaunchDate( ) const
{
    return m_launchDate;
}

void MagnatuneAlbum::setAlbumCode( QString albumCode )
{
    m_albumCode = albumCode;
}

QString MagnatuneAlbum::getAlbumCode( ) const
{
    return m_albumCode;
}

void MagnatuneAlbum::setMp3Genre( QString mp3Genre )
{
    m_mp3Genre = mp3Genre;
}

QString MagnatuneAlbum::getMp3Genre( ) const
{
    return m_mp3Genre;
}

void MagnatuneAlbum::setMagnatuneGenres( QString magnatuneGenres )
{
    m_magnatuneGenres = magnatuneGenres;
}

QString MagnatuneAlbum::getMagnatuneGenres( ) const
{
    return m_magnatuneGenres;
}









//// MagnatuneTrack ////

MagnatuneTrack::MagnatuneTrack( )
{
}

MagnatuneTrack::MagnatuneTrack(const MagnatuneTrack& track)
{
    m_id = track.getId();
    m_name = track.getName();
    m_artistId = track.getArtistId();
    m_albumId = track.getAlbumId();
    m_duration = track.getDuration();
    m_trackNumber = track.getTrackNumber();
    m_hifiURL = track.getHifiURL();
    m_lofiURL = track.getLofiURL();

}

MagnatuneTrack::~ MagnatuneTrack( )
{
}

void MagnatuneTrack::setId( int id )
{
   m_id = id;
}

int MagnatuneTrack::getId( ) const
{
    return m_id;
}

void MagnatuneTrack::setArtistId( int artistId )
{
    m_artistId = artistId;
}

int MagnatuneTrack::getArtistId( ) const
{
   return m_artistId;
}

void MagnatuneTrack::setAlbumId( int albumId )
{
    m_albumId = albumId;
}

int MagnatuneTrack::getAlbumId( ) const
{
   return m_albumId;
}



void MagnatuneTrack::setName( QString name )
{
    m_name = name;
}

QString MagnatuneTrack::getName( ) const
{
   return m_name;
}

void MagnatuneTrack::setTrackNumber( int trackNumber )
{
    m_trackNumber = trackNumber;
}

int MagnatuneTrack::getTrackNumber( ) const
{
    return m_trackNumber;
}

void MagnatuneTrack::setDuration( int duration )
{
    m_duration = duration;
}

int MagnatuneTrack::getDuration( ) const
{
    return m_duration;
}

void MagnatuneTrack::setHifiURL( QString hifiURL )
{
   m_hifiURL = hifiURL;
}

QString MagnatuneTrack::getHifiURL( ) const
{
    return m_hifiURL;
}

void MagnatuneTrack::setLofiURL( QString lofiURL )
{
    m_lofiURL = lofiURL;
}
 
QString MagnatuneTrack::getLofiURL( ) const
{
    return m_lofiURL;
}














