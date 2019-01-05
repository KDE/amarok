/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_SCRIPTABLE_SERVICE_META_P_H
#define AMAROK_SCRIPTABLE_SERVICE_META_P_H


#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core/capabilities/SourceInfoCapability.h"

#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QString>

#include <KLocalizedString>



// internal helper classes


/**
 * Base class for the private meta types. This is used to give these private items source info capability which is needed in some cases,for instance when bookmarking.
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


class AMAROK_EXPORT ScriptableServiceInternalSourceInfoCapability : public Capabilities::SourceInfoCapability
{
    public:
        explicit ScriptableServiceInternalSourceInfoCapability( ScriptableServiceInternalMetaItem * sourceInfoProvider )
        {
            m_sourceInfoProvider = sourceInfoProvider;
        }
        ~ScriptableServiceInternalSourceInfoCapability() {}

        QString sourceName() override { return m_sourceInfoProvider->serviceName(); }
        QString sourceDescription() override { return m_sourceInfoProvider->serviceDescription(); }
        QPixmap emblem() override { return m_sourceInfoProvider->serviceEmblem(); }
        QString scalableEmblem() override { return m_sourceInfoProvider->serviceScalableEmblem(); }
        

    private:
        ScriptableServiceInternalMetaItem * m_sourceInfoProvider;

};


class ScriptableServiceInternalArtist : public Meta::Artist, public ScriptableServiceInternalMetaItem
{
    public:
        explicit ScriptableServiceInternalArtist( const QString &name = QString() )
        : Meta::Artist()
        , m_name( name )
        {}

        Meta::TrackList tracks() override
        {
            return Meta::TrackList();
        }

        QString name() const override
        {
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const override
        {
            return name();
        }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::SourceInfo );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

private:
    QString m_name;

};

class ScriptableServiceInternalAlbum : public Meta::ServiceAlbumWithCover, public ScriptableServiceInternalMetaItem
{
    public:
        explicit ScriptableServiceInternalAlbum( const QString &name = QString() )
        : Meta::ServiceAlbumWithCover( QString() )
        , m_name( name )
        {}

        bool isCompilation() const override
        {
            return false;
        }

        bool hasAlbumArtist() const override
        {
            return false;
        }

        Meta::ArtistPtr albumArtist() const override
        {
            return Meta::ArtistPtr();
        }

        Meta::TrackList tracks() override
        {
            return Meta::TrackList();
        }

        QString name() const override
        {
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const override
        {
            return name();
        }

        QString downloadPrefix() const override { return "script"; }
        void setCoverUrl( const QString &coverUrl ) override { m_coverUrl = coverUrl; }
        QString coverUrl() const override { return m_coverUrl; }

        QUrl imageLocation( int size = 1 ) override { Q_UNUSED( size ); return QUrl( coverUrl() ); }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::SourceInfo );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::SourceInfo )
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
        explicit ScriptableServiceInternalGenre( const QString &name = QString() )
        : Meta::Genre()
        , m_name( name )
        {}

        Meta::TrackList tracks() override
        {
            return Meta::TrackList();
        }

        QString name() const override
        {
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const override
        {
            return name();
        }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::SourceInfo );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
};

class ScriptableServiceInternalComposer : public Meta::Composer, public ScriptableServiceInternalMetaItem
{
    public:
        explicit ScriptableServiceInternalComposer( const QString &name = QString() )
        : Meta::Composer()
        , m_name( name )
        {}

        Meta::TrackList tracks() override
        {
            return Meta::TrackList();
        }

        QString name() const override
        {

            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const override
        {
            return name();
        }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::SourceInfo );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
};

class ScriptableServiceInternalYear : public Meta::Year, public ScriptableServiceInternalMetaItem
{
    public:
        explicit ScriptableServiceInternalYear( const QString &name = QString() )
        : Meta::Year()
        , m_name( name )
        {}

        Meta::TrackList tracks() override
        {
            return Meta::TrackList();
        }

        QString name() const override
        {
            if( !m_name.isEmpty() )
                return m_name;
            else
                return i18nc( "The value is not known", "Unknown" );
        }

        QString prettyName() const override
        {
            return name();
        }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            return ( type == Capabilities::Capability::SourceInfo );
        }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            if ( type == Capabilities::Capability::SourceInfo )
                return new ScriptableServiceInternalSourceInfoCapability( this );
            return 0;
        }

    private:
        QString m_name;
};


#endif
