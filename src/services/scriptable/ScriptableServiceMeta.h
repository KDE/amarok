/****************************************************************************************
 * Copyright (c) 2007-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef DYNAMICSCRIPTABLESERVICEMETA_H
#define DYNAMICSCRIPTABLESERVICEMETA_H

#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"

/**
    Meta types for use in the dynamic scriptable service. Nearly identical to 
    the ServiceMetaBse types, except for a field for storing data to 
    pass to the script to fetch child items for Genres, Artists and Albums 
    and a level specifier

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/


namespace Meta
{

class ScriptableServiceMetaItem
{
    public:
        explicit ScriptableServiceMetaItem( int level );

        void setCallbackString( const QString &callbackString );
        QString callbackString() const;
        int level() const;

        void setServiceName( const QString &name );
        void setServiceDescription( const QString &description );
        void setServiceEmblem( const QPixmap &emblem );
        void setServiceScalableEmblem( const QString &emblemPath );

    protected:
        /*
         * This is arbitrary string data to pass back to the script. This can be whatever
         * information the script needs to fetch the children of this item...
         */
        QString m_callbackString;
        int m_level;
        QString m_serviceName;
        QString m_serviceDescription;
        QPixmap m_serviceEmblem;
        QString m_serviceScalableEmblem;

};

class ScriptableServiceTrack : public Meta::ServiceTrack, public ScriptableServiceMetaItem
{
    public:
        explicit ScriptableServiceTrack( const QString &name );
        explicit ScriptableServiceTrack( const QStringList &resultRow );

        QString sourceName() override;
        QString sourceDescription() override;
        QPixmap emblem() override;
        QString scalableEmblem() override;

        void setAlbumName( const QString &newAlbum );
        void setArtistName( const QString &newArtist );
        void setGenreName( const QString &newGenre );
        void setComposerName( const QString &newComposer );
        void setYearNumber( int newYear );

        void setCustomAlbumCoverUrl( const QString &coverurl );

        QString collectionName() const override { return m_serviceName; }
        void setUidUrl( const QString &url ) override;

        /**
         * If this track is in fact a remote playlist, return Meta::MultiTrack that wraps
         * it here, else return pointer to self.
         */
        Meta::TrackPtr playableTrack() const;

    private:
        Meta::TrackPtr m_playableTrack;
};

class ScriptableServiceAlbum : public Meta::ServiceAlbumWithCover, public ScriptableServiceMetaItem
{
    public:
        explicit ScriptableServiceAlbum( const QString &name );
        explicit ScriptableServiceAlbum( const QStringList &resultRow );

        QString downloadPrefix() const override { return QStringLiteral("script"); }
        void setCoverUrl( const QString &coverUrl ) override { m_coverUrl = coverUrl; }
        QString coverUrl() const override { return m_coverUrl; }

        QUrl imageLocation( int size = 1 ) override { Q_UNUSED( size ); return QUrl( coverUrl() ); }

        QString sourceName() override;
        QString sourceDescription() override;
        QPixmap emblem() override;
        QString scalableEmblem() override;

        bool isBookmarkable() const override;
        QString collectionName() const override { return m_serviceName; }
        bool simpleFiltering() const  override { return true; }

    private:
        QString m_coverUrl;

};

class ScriptableServiceArtist : public Meta::ServiceArtist, public ScriptableServiceMetaItem
{
    public:
        explicit ScriptableServiceArtist( const QString &name );
        explicit ScriptableServiceArtist( const QStringList &resultRow );

        void setGenreId( int artistId );
        int genreId() const;

        QString sourceName() override;
        QString sourceDescription() override;
        QPixmap emblem() override;
        QString scalableEmblem() override;

        bool isBookmarkable() const override;
        QString collectionName() const override { return m_serviceName; }
        bool simpleFiltering() const override { return true; }

    private:
        int m_genreId;
};


class ScriptableServiceGenre : public Meta::ServiceGenre, public ScriptableServiceMetaItem
{
    public:
        explicit ScriptableServiceGenre( const QString &name );
        explicit ScriptableServiceGenre( const QStringList &resultRow );

        void setDescription( const QString &description );
        QString description();

        QString sourceName() override;
        QString sourceDescription() override;
        QPixmap emblem() override;
        QString scalableEmblem() override;

    private:
        QString m_description;
};

}

#endif
