/*
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

#ifndef AMAROK_META_FILE_P_H
#define AMAROK_META_FILE_P_H

#include "meta.h"

#include <QObject>
#include <QSet>
#include <QString>

#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>

namespace MetaFile
{

    static const QString ARTIST = "audio.artist";
    static const QString ALBUM = "audio.album";
    static const QString TITLE = "audio.title";
    static const QString FILESIZE = "system.size";

//d-pointer implementation

class Track::Private : public QObject
{
public:
    Private( Track *t )
        : QObject()
        , metaInfo()
        , observers()
        , url()
        , batchUpdate( false )
        , album()
        , artist()
        , track( t )
    {}

    void notify() const
    {
        foreach( Meta::TrackObserver *observer, observers )
            observer->metadataChanged( track );
    }

public:
    KFileMetaInfo metaInfo;
    QSet<Meta::TrackObserver*> observers;
    KUrl url;
    bool batchUpdate;
    Meta::AlbumPtr album;
    Meta::ArtistPtr artist;

private:
    Track *track;
};

// internal helper classes

class FileArtist : public Meta::Artist
{
public:
    FileArtist( MetaFile::Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d )
        {
            KFileMetaInfoItem item = d->metaInfo.item( MetaFile::ARTIST );
            if( item.isValid() )
                return item.value().toString();
            else
                return QString();
        }
        else
            return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    MetaFile::Track::Private * const d;
};

class FileAlbum : public Meta::Album
{
public:
    FileAlbum( MetaFile::Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool isCompilation() const
    {
        return false;
    }

    bool hasAlbumArtist() const
    {
        return false;
    }

    Meta::ArtistPtr albumArtist() const
    {
        return Meta::ArtistPtr();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d )
        {
            KFileMetaInfoItem item = d->metaInfo.item( MetaFile::ALBUM );
            if( item.isValid() )
                return item.value().toString();
            else
                return QString();
        }
        else
            return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    QPixmap image( int size, bool withShadow ) const
    {
        return Meta::Album::image( size, withShadow );
    }

    MetaFile::Track::Private * const d;
};

}

#endif
