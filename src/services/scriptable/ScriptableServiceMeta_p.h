/*
   Copyright (C) 2007 Maximilian Kossick     <maximilian.kossick@googlemail.com>
   Copyright (C) 2008 Peter ZHOU             <peterzhoulei@gmail.com>
   Copyright (C) 2008 Nikolaj Hald Nielsen   <nhnFreespirit@@gmail.com>

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

#ifndef AMAROK_SCRIPTABLE_SERVICE_META_P_H
#define AMAROK_SCRIPTABLE_SERVICE_META_P_H


#include "Debug.h"
#include "Meta.h"

#include <QObject>
#include <QPointer>
#include <QString>

#include <KLocale>




//d-pointer implementation

struct MetaData
{
    MetaData()
    : discNumber( 0 )
    , trackNumber( 0 )
    , length( 0 )
    , fileSize( 0 )
    , sampleRate( 0 )
    , bitRate( 0 )
    , year( 0 )
    { }


    QString artist;
    QString album;
    QString genre;
    QString comment;
    QString composer;
    int discNumber;
    int trackNumber;
    int length;
    int fileSize;
    int sampleRate;
    int bitRate;
    int year;

};

class Meta::ScriptableServiceTrack::Private : public QObject
{
    public:
        Private( ScriptableServiceTrack *t )
        : QObject()
        , track( t )
        {}

    public:

        Meta::AlbumPtr album;
        Meta::ArtistPtr artist;
        Meta::GenrePtr genre;
        Meta::ComposerPtr composer;
        Meta::YearPtr year;

        MetaData m_data;

    private:
        ScriptableServiceTrack *track;
};


// internal helper classes

class ScriptableServiceInternalArtist : public Meta::Artist
{
    public:
        ScriptableServiceInternalArtist( Meta::ScriptableServiceTrack::Private *dptr )
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
                const QString artist = d->m_data.artist;
                if( !artist.isEmpty() )
                    return artist;
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

        QPointer<Meta::ScriptableServiceTrack::Private> const d;
};

class ScriptableServiceInternalAlbum : public Meta::Album
{
    public:
        ScriptableServiceInternalAlbum( Meta::ScriptableServiceTrack::Private *dptr )
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
                const QString albumName = d->m_data.album;
                if( !albumName.isEmpty() )
                    return albumName;
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

        QPixmap image( int size )
        {
            return Meta::Album::image( size );
        }

        QPointer<Meta::ScriptableServiceTrack::Private> const d;
};

class ScriptableServiceInternalGenre : public Meta::Genre
{
    public:
        ScriptableServiceInternalGenre( Meta::ScriptableServiceTrack::Private *dptr )
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
                QString genreName = d->m_data.genre;
                if( !genreName.isEmpty() )
                    return genreName;
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

        QPointer<Meta::ScriptableServiceTrack::Private> const d;
};

class ScriptableServiceInternalComposer : public Meta::Composer
{
    public:
        ScriptableServiceInternalComposer( Meta::ScriptableServiceTrack::Private *dptr )
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
                QString composer = d->m_data.composer;
                if( !composer.isEmpty() )
                    return composer;
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

        QPointer<Meta::ScriptableServiceTrack::Private> const d;
};

class ScriptableServiceInternalYear : public Meta::Year
{
    public:
        ScriptableServiceInternalYear( Meta::ScriptableServiceTrack::Private *dptr )
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
                const QString year = QString::number( d->m_data.year );
                if( !year.isEmpty()  )
                    return year;
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

        QPointer<Meta::ScriptableServiceTrack::Private> const d;
};


#endif
