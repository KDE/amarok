/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_SCRIPTABLE_SERVICE_META_P_H
#define AMAROK_SCRIPTABLE_SERVICE_META_P_H


#include "Debug.h"
#include "Meta.h"
#include "SourceInfoCapability.h"

#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QString>

#include <KLocale>



// internal helper classes


/**
 * Base class for the private meta types. This is used to give these private items source info capability which is neede in some cases,for instance when bookmarking.
 */
class ScriptableServiceInternalMetaItem
{
    public:

        QString serviceName() { return m_serviceName; }
        QString serviceDescription() { return m_serviceDescription; }
        QPixmap serviceEmblem() { return m_serviceEmblem; }
        QString serviceScalableEmblem() { return m_serviceScalableEmblem; }

        void setServiceName( const QString &name ) { m_serviceName = name; }
        void setServiceDescription( const QString &description ) { m_serviceDescription = description; }
        void setServiceEmblem( const QPixmap &emblem ) { m_serviceEmblem = emblem; }
        void setServiceScalableEmblem( const QString &emblemPath ) { m_serviceScalableEmblem = emblemPath; }

    protected:
        QString m_serviceName;
        QString m_serviceDescription;
        QPixmap m_serviceEmblem;
        QString m_serviceScalableEmblem;
};


class AMAROK_EXPORT ScriptableServiceInternalSourceInfoCapability : public Meta::SourceInfoCapability
{
    public:
        ScriptableServiceInternalSourceInfoCapability( ScriptableServiceInternalMetaItem * sourceInfoProvider )
        {
            m_sourceInfoProvider = sourceInfoProvider;
        }
        ~ScriptableServiceInternalSourceInfoCapability() {};

        QString sourceName() { return m_sourceInfoProvider->serviceName(); }
        QString sourceDescription() { return m_sourceInfoProvider->serviceDescription(); }
        QPixmap emblem() { return m_sourceInfoProvider->serviceEmblem(); }
        QString scalableEmblem() { return m_sourceInfoProvider->serviceScalableEmblem(); }
        

    private:
        ScriptableServiceInternalMetaItem * m_sourceInfoProvider;

};


class ScriptableServiceInternalArtist : public Meta::Artist, public ScriptableServiceInternalMetaItem
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

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const
        {
            return ( type == Meta::Capability::SourceInfo );
        }

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type )
        {
            if ( type == Meta::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

private:
    QString m_name;

};

class ScriptableServiceInternalAlbum : public Meta::ServiceAlbumWithCover, public ScriptableServiceInternalMetaItem
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

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const
        {
            return ( type == Meta::Capability::SourceInfo );
        }

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type )
        {
            if ( type == Meta::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
        QString m_coverUrl;
};

class ScriptableServiceInternalGenre : public Meta::Genre, public ScriptableServiceInternalMetaItem
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

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const
        {
            return ( type == Meta::Capability::SourceInfo );
        }

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type )
        {
            if ( type == Meta::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
};

class ScriptableServiceInternalComposer : public Meta::Composer, public ScriptableServiceInternalMetaItem
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

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const
        {
            return ( type == Meta::Capability::SourceInfo );
        }

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type )
        {
            if ( type == Meta::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
};

class ScriptableServiceInternalYear : public Meta::Year, public ScriptableServiceInternalMetaItem
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

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const
        {
            return ( type == Meta::Capability::SourceInfo );
        }

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type )
        {
            if ( type == Meta::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
};


#endif
