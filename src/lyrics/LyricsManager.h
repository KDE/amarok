/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef LYRICS_MANAGER_H
#define LYRICS_MANAGER_H

#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QMap>
#include <QStringList>
#include <QUrl>
#include <QVariant>


class AMAROK_EXPORT LyricsManager : public QObject
{
    Q_OBJECT

    public:
        static LyricsManager* instance()
        { 
            if( !s_self )
                s_self = new LyricsManager();

            return s_self; 
        }

        /**
         * Tests if the given lyrics are empty.
         *
         * @param lyrics The lyrics which will be tested.
         *
         * @return true if the given lyrics are empty, otherwise false.
         */
        bool isEmpty( const QString &lyrics ) const;

        void newTrack( Meta::TrackPtr track );
        void lyricsResult( const QByteArray& lyrics, Meta::TrackPtr track );
        void lyricsLoaded( const QUrl &url, const QByteArray &data, NetworkAccessManagerProxy::Error err );
        void loadLyrics( Meta::TrackPtr track, bool overwrite = false );

    Q_SIGNALS:
        void newLyrics( Meta::TrackPtr );
        void newSuggestions( const QVariantList& );
        void error( const QString &);

    private:
        LyricsManager();

        void sanitizeTitle( QString &title );
        void sanitizeArtist( QString &artist );
        
        static LyricsManager* s_self;

        QMap<QUrl, Meta::TrackPtr> m_trackMap;
};

#endif
