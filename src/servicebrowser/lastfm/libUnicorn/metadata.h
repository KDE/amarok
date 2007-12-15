/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef METADATA_H
#define METADATA_H

#include "UnicornDllExportMacro.h"

#include "TrackInfo.h"
#include <QDate>


class UNICORN_DLLEXPORT MetaData : public TrackInfo
{
    public:
        MetaData() {}
        MetaData( const TrackInfo& i ) : TrackInfo( i ) {}
        #ifdef QT_XML_LIB
        MetaData( const QDomElement& e ) : TrackInfo( e ) {}
        #endif
       
        bool isTrackBuyable() const;
        bool isAlbumBuyable() const;

        // Track methods
        const QStringList trackTags() const { return m_trackTags; }
        void setTrackTags( QStringList trackTags ) { m_trackTags = trackTags; }

        const QString trackPageUrl() const { return m_trackUrl; }
        void setTrackPageUrl( QString url ) { m_trackUrl = url; }

        const QString buyTrackString() const { return m_buyTrackString; }
        void setBuyTrackString( QString str ) { m_buyTrackString = str; }
        const QString buyTrackUrl() const { return m_buyTrackUrl; }
        void setBuyTrackUrl( QString url ) { m_buyTrackUrl = url; }

        // Album methods
        const QUrl albumPicUrl() const { return m_albumPicUrl; }
        void setAlbumPicUrl( QString cover ) { m_albumPicUrl = QUrl( cover ); }
        void setAlbumPicUrl( QUrl cover ) { m_albumPicUrl = cover; }

        const QString albumPageUrl() const { return m_albumUrl; }
        void setAlbumPageUrl( QString url ) { m_albumUrl = url; }

        const QString label() const { return m_label; }
        void setLabel( QString lbl ) { m_label = lbl; }

        const QString labelUrl() const { return m_labelUrl; }
        void setLabelUrl( QString lblurl ) { m_labelUrl = lblurl; }

        const QDate releaseDate() const { return m_releaseDate; }
        void setReleaseDate( QDate date ) { m_releaseDate = date; }

        const int numTracks() const { return m_numTracks; }
        void setNumTracks( int num ) { m_numTracks = num; }

        const QString buyAlbumString() const { return m_buyAlbumString; }
        void setBuyAlbumString( QString str ) { m_buyAlbumString = str; }
        const QString buyAlbumUrl() const { return m_buyAlbumUrl; }
        void setBuyAlbumUrl( QString url ) { m_buyAlbumUrl = url; }

        // Artist methods
        const QStringList artistTags() const { return m_artistTags; }
        void setArtistTags( QStringList artistTags ) { m_artistTags = artistTags; }

        const QStringList similarArtists() const { return m_similarArtists; }
        void setSimilarArtists( QStringList similarArtists ) { m_similarArtists = similarArtists; }

        const QUrl artistPicUrl() const { return m_artistPicUrl; }
        void setArtistPicUrl( QString artistPicUrl ) { m_artistPicUrl = QUrl( artistPicUrl ); }
        void setArtistPicUrl( QUrl artistPic ) { m_artistPicUrl = artistPic; }

        const QString wiki() const { return m_wiki; }
        void setWiki( QString wiki ) { m_wiki = wiki; }

        const QString wikiPageUrl() const { return m_wikiUrl; }
        void setWikiPageUrl( QString wikiUrl ) { m_wikiUrl = wikiUrl; }

        const QString artistPageUrl() const { return m_artistUrl; }
        void setArtistPageUrl( QString url ) { m_artistUrl = url; }

        const QStringList topFans() const { return m_topFans; }
        void setTopFans( QStringList topFans ) { m_topFans = topFans; }

        const int numListeners() const { return m_numListeners; }
        void setNumListeners( int num ) { m_numListeners = num; }

        const int numPlays() const { return m_numPlays; }
        void setNumPlays( int num ) { m_numPlays = num; }

    private:
        // REFACTOR: refactor this fucker to Artist/Track/Album classes

        // Track data
        QStringList m_trackTags;
        QString m_trackUrl;
        QString m_buyTrackString;
        QString m_buyTrackUrl;

        // Album data
        QUrl m_albumPicUrl;
        QString m_albumUrl;
        QString m_label;
        QString m_labelUrl;
        QDate m_releaseDate;
        int m_numTracks;
        QString m_buyAlbumString;
        QString m_buyAlbumUrl;

        // Artist data
        QStringList m_artistTags;
        QStringList m_similarArtists;
        QString m_wiki;
        QUrl m_artistPicUrl;
        QString m_artistUrl;
        QString m_wikiUrl;
        QStringList m_topFans;
        int m_numListeners;
        int m_numPlays;
};


Q_DECLARE_METATYPE( MetaData );

#endif
