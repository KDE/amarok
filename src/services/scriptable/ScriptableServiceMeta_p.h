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



// internal helper classes

class ScriptableServiceInternalArtist : public Meta::Artist
{
    public:
        ScriptableServiceInternalArtist( const QString &name = QString() )
        : Meta::Artist()
        , m_name( name )
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
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );;
        }

        QString prettyName() const
        {
            return name();
        }

private:
    QString m_name;

};

class ScriptableServiceInternalAlbum : public Meta::ServiceAlbumWithCover
{
    public:
        ScriptableServiceInternalAlbum( const QString &name = QString() )
        : Meta::ServiceAlbumWithCover( QString() )
        , m_name( name )
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
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const
        {
            return name();
        }

        virtual QString downloadPrefix() const { return "script"; }
        virtual void setCoverUrl( const QString &coverUrl ) { m_coverUrl = coverUrl; }
        virtual QString coverUrl() const { return m_coverUrl; }

    private:
        QString m_name;
        QString m_coverUrl;
};

class ScriptableServiceInternalGenre : public Meta::Genre
{
    public:
        ScriptableServiceInternalGenre( const QString &name = QString() )
        : Meta::Genre()
        , m_name( name )
        {}

        Meta::TrackList tracks()
        {
            return Meta::TrackList();
        }

        QString name() const
        {
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const
        {
            return name();
        }
    private:
        QString m_name;
};

class ScriptableServiceInternalComposer : public Meta::Composer
{
    public:
        ScriptableServiceInternalComposer( const QString &name = QString() )
        : Meta::Composer()
        , m_name( name )
        {}

        Meta::TrackList tracks()
        {
            return Meta::TrackList();
        }

        QString name() const
        {

            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const
        {
            return name();
        }
    private:
        QString m_name;
};

class ScriptableServiceInternalYear : public Meta::Year
{
    public:
        ScriptableServiceInternalYear( const QString &name = QString() )
        : Meta::Year()
        , m_name( name )
        {}

        Meta::TrackList tracks()
        {
            return Meta::TrackList();
        }

        QString name() const
        {
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const
        {
            return name();
        }
    private:
        QString m_name;
};


#endif
