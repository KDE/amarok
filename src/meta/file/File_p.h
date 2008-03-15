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

#include "Meta.h"
#include "MetaUtility.h"

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>

#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>
#include <KLocale>

namespace MetaFile
{

//d-pointer implementation

class Track::Private : public QObject
{
public:
    Private( Track *t )
        : QObject()
        , metaInfo()
        , url()
        , batchUpdate( false )
        , album()
        , artist()
        , track( t )
    {}

public:
    KFileMetaInfo metaInfo;
    KUrl url;
    bool batchUpdate;
    Meta::AlbumPtr album;
    Meta::ArtistPtr artist;
    Meta::GenrePtr genre;
    Meta::ComposerPtr composer;
    Meta::YearPtr year;

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

    Meta::AlbumList albums()
    {
        return Meta::AlbumList();
    }

    QString name() const
    {
        if( d )
        {
            KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::ARTIST ) );
            if( item.isValid() && !item.value().toString().isEmpty()  )
                return item.value().toString();
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
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
            KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::ALBUM ) );
            if( item.isValid() && !item.value().toString().isEmpty()  )
                return item.value().toString();
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPixmap image( int size, bool withShadow )
    {
        return Meta::Album::image( size, withShadow );
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileGenre : public Meta::Genre
{
public:
    FileGenre( MetaFile::Track::Private *dptr )
        : Meta::Genre()
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
            KFileMetaInfoItem item = d->metaInfo.item(Meta::Field::xesamPrettyToFullFieldName(  Meta::Field::GENRE ) );
            if( item.isValid() && !item.value().toString().isEmpty()  )
                return item.value().toString();
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileComposer : public Meta::Composer
{
public:
    FileComposer( MetaFile::Track::Private *dptr )
        : Meta::Composer()
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
            KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::COMPOSER ) );
            if( item.isValid() && !item.value().toString().isEmpty()  )
                return item.value().toString();
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileYear : public Meta::Year
{
public:
    FileYear( MetaFile::Track::Private *dptr )
        : Meta::Year()
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
            KFileMetaInfoItem item = d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::YEAR ) );
            if( item.isValid() && !item.value().toString().isEmpty()  )
                return item.value().toString();
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};


}

#endif
