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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef SIMPLESERVICETYPES_H
#define SIMPLESERVICETYPES_H


#include <QString>
#include <QList>


class SimpleServiceArtist 
{

protected:
    int m_id;
    QString m_name;
    QString m_description;

public:
    SimpleServiceArtist();

    void setId( int id );
    int getId() const;

    void setName( const QString &name );
    QString getName() const;

    void setDescription( const QString &description );
    QString getDescription() const;

};


class SimpleServiceAlbum 
{
protected:
    int m_id;
    QString m_name;
    QString m_description;
    int m_artistId;

public:
    SimpleServiceAlbum();

    void setId( int id );
    int getId() const;

    void setName( const QString &name );
    QString getName() const;

    void setArtistId( int artistId );
    int getArtistId() const;

    void setDescription( const QString description );
    QString getDescription();


};

class SimpleServiceTrack 
{
protected:
    int m_id;
    QString m_name;
    int m_trackNumber;
    int m_duration;
    QString m_url;
    int m_albumId;

public:
    SimpleServiceTrack();

    void setId( int id );
    int getId() const;
 
    void setName( const QString &name );
    QString getName() const;

    void setTrackNumber( int trackNumber );
    int getTrackNumber() const;

    void setDuration( int duration );
    int getDuration() const;

    void setURL( const QString &hifiURL );
    QString getURL() const;

    void setAlbumId( int albumId );
    int getAlbumId() const;

};

typedef QList<SimpleServiceArtist> SimpleServiceArtistList;
typedef QList<SimpleServiceAlbum> SimpleServiceAlbumList;
typedef QList<SimpleServiceTrack> SimpleServiceTrackList;

#endif
