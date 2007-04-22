/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#ifndef MAGNATUNETYPES_H
#define MAGNATUNETYPES_H

#include "../simpleservicetypes.h"


#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>


class MagnatuneArtist : public SimpleServiceArtist
{

private:

    QString m_photoURL;
    QString m_homeURL;

public:
    MagnatuneArtist();

    void setPhotoURL( const QString &photoURL );
    QString getPhotoURL() const;
  
    void setHomeURL( const QString &homeURL );
    QString getHomeURL() const;
};


class MagnatuneAlbum : public SimpleServiceAlbum
{
private:

    QString m_coverURL;
    QDate m_launchDate;
    QString m_albumCode;
    QString m_mp3Genre;
    QString m_magnatuneGenres;


public:
    MagnatuneAlbum();

    void setCoverURL( const QString &coverURL );
    QString getCoverURL() const;

    void setLaunchDate( const QDate &launchDate );
    QDate getLaunchDate() const;

    void setAlbumCode( const QString &albumCode );
    QString getAlbumCode() const;

    void setMp3Genre( const QString &mp3Genre );
    QString getMp3Genre() const;

    void setMagnatuneGenres( const QString &magnatuneGenres );
    QString getMagnatuneGenres() const;

};

class MagnatuneTrack : public SimpleServiceTrack
{
private:

    QString m_lofiURL;
    int m_artistId;
    QStringList m_moods;

public:
    MagnatuneTrack();

    void setLofiURL( const QString &lofiURL );
    QString getLofiURL() const;

    void setArtistId( int artistId );
    int getArtistId() const;

    void setMoods( const QStringList &moods );
    QStringList getMoods() const;
};

typedef QList<MagnatuneArtist> MagnatuneArtistList;
typedef QList<MagnatuneAlbum> MagnatuneAlbumList;
typedef QList<MagnatuneTrack> MagnatuneTrackList;

#endif
