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
        ScriptableServiceMetaItem( int level );
    
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

        Meta::AlbumPtr m_albumPtr;
        Meta::ArtistPtr m_artistPtr;
        Meta::GenrePtr m_genrePtr;
        Meta::ComposerPtr m_composerPtr;
        Meta::YearPtr m_yearPtr;
};


class ScriptableServiceTrack : public Meta::ServiceTrack, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceTrack( const QString & name );
        ScriptableServiceTrack( const QStringList & resultRow );

        virtual QString sourceName();
        virtual QString sourceDescription();
        virtual QPixmap emblem();
        virtual QString scalableEmblem();

        virtual Meta::AlbumPtr album() const;
        virtual Meta::ArtistPtr artist() const;
        virtual Meta::GenrePtr genre() const;
        virtual Meta::ComposerPtr composer() const;
        virtual Meta::YearPtr year() const;
        
        void setAlbumName( const QString &newAlbum );
        void setArtistName( const QString &newArtist );
        void setGenreName( const QString &newGenre );
        void setComposerName( const QString &newComposer );
        void setYearNumber( int newYear );

        void setCustomAlbumCoverUrl( const QString &coverurl );

        virtual QString collectionName() const { return m_serviceName; }

    private:

        

};

class ScriptableServiceAlbum : public Meta::ServiceAlbumWithCover, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceAlbum( const QString &name );
        ScriptableServiceAlbum( const QStringList &resultRow );

        virtual QString downloadPrefix() const { return "script"; }
        virtual void setCoverUrl( const QString &coverUrl ) { m_coverUrl = coverUrl; }
        virtual QString coverUrl() const { return m_coverUrl; }

        virtual KUrl imageLocation( int size = 1 ) { Q_UNUSED( size ); return KUrl( coverUrl() ); }

        virtual QString sourceName();
        virtual QString sourceDescription();
        virtual QPixmap emblem();
        virtual QString scalableEmblem();

        virtual bool isBookmarkable() const;
        virtual QString collectionName() const { return m_serviceName; }
        virtual bool simpleFiltering() const  { return true; }

    private:
        QString m_coverUrl;

};

class ScriptableServiceArtist : public Meta::ServiceArtist, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceArtist( const QString &name );
        ScriptableServiceArtist( const QStringList &resultRow );

        void setGenreId( int artistId );
        int genreId() const;

        virtual QString sourceName();
        virtual QString sourceDescription();
        virtual QPixmap emblem();
        virtual QString scalableEmblem();

        virtual bool isBookmarkable() const;
        virtual QString collectionName() const { return m_serviceName; }
        virtual bool simpleFiltering() const { return true; }

    private:
        int m_genreId;
};


class ScriptableServiceGenre : public Meta::ServiceGenre, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceGenre( const QString &name );
        ScriptableServiceGenre( const QStringList &resultRow );

        void setDescription( const QString &description );
        QString description();

        virtual QString sourceName();
        virtual QString sourceDescription();
        virtual QPixmap emblem();
        virtual QString scalableEmblem();

    private:
        QString m_description;
};

}

#endif
