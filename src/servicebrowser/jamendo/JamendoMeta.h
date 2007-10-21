/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef JAMENDOMETA_H
#define JAMENDOMETA_H


#include "../servicemetabase.h"
#include "../ServiceAlbumCoverDownloader.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>


class JamendoMetaFactory : public ServiceMetaFactory
{

public:
    JamendoMetaFactory( const QString &dbPrefix );
    virtual ~JamendoMetaFactory() {}

    //virtual int getTrackSqlRowCount();
    //virtual QString getTrackSqlRows();
    virtual Meta::TrackPtr createTrack( const QStringList &rows );

    virtual int getAlbumSqlRowCount();
    virtual QString getAlbumSqlRows();
    virtual Meta::AlbumPtr createAlbum( const QStringList &rows );

    virtual int getArtistSqlRowCount();
    virtual QString getArtistSqlRows();
    virtual Meta::ArtistPtr createArtist( const QStringList &rows );

    //virtual int getGenreSqlRowCount();
    //virtual QString getGenreSqlRows();
    virtual Meta::GenrePtr createGenre( const QStringList &rows );

};


namespace Meta
{

class JamendoTrack  : public ServiceTrack
{


public:
    JamendoTrack( const QString &name );
    JamendoTrack( const QStringList &resultRow );

};


class JamendoArtist : public ServiceArtist
{

private:

    QString m_country;
    QString m_photoURL;
    QString m_jamendoURL;
    QString m_homeURL;

public:
    JamendoArtist( const QString &name );
    JamendoArtist( const QStringList &resultRow );

    void setPhotoURL( const QString &photoURL );
    QString photoURL() const;

    void setCountry( const QString &country );
    QString country() const;

    void setHomeURL( const QString &homeURL );
    QString homeURL() const;

    void setJamendoURL( const QString &jamendoURL );
    QString jamendoURL() const;
};


class JamendoAlbum  : public ServiceAlbumWithCover
{
private:
    float m_popularity;
    QString m_coverURL;
    int m_launchYear;
    QString m_genre;
    QString m_mp3TorrentUrl;
    QString m_oggTorrentUrl;


public:
    JamendoAlbum( const QString &name );
    JamendoAlbum( const QStringList &resultRow );

        
    virtual QString downloadPrefix() const { return "jamendo"; }
    
    void setPopularity( float popularity );
    float popularity() const;

    virtual void setCoverUrl( const QString &coverURL );
    virtual QString coverUrl() const;

    void setLaunchYear( int launchYear );
    int launchYear() const;

    void setGenre( const QString &genre );
    QString genre() const;

    void setMp3TorrentUrl( QString url );
    QString mp3TorrentUrl();

    void setOggTorrentUrl( QString url );
    QString oggTorrentUrl();
    


};

class JamendoGenre  : public ServiceGenre
{

public:
    JamendoGenre( const QString &name );
    JamendoGenre( const QStringList &resultRow );

};

}

#endif
