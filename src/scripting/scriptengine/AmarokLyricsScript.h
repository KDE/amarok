/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_LYRICS_SCRIPT_H
#define AMAROK_LYRICS_SCRIPT_H

#include "core/meta/forward_declarations.h"

#include <QObject>

class QJSEngine;
class QByteArray;

namespace AmarokScript
{
    // SCRIPTDOX Amarok.Lyrics
    class AmarokLyricsScript : public QObject
    {
        Q_OBJECT

        public:
            explicit AmarokLyricsScript( QJSEngine* scriptEngine );

            Q_INVOKABLE void showLyrics( const QString& lyrics ) const;

            Q_INVOKABLE void showLyricsHtml( const QString& lyrics ) const;
            Q_INVOKABLE void showLyricsError( const QString& error ) const;
            Q_INVOKABLE void showLyricsNotFound( const QString& msg ) const;

            Q_INVOKABLE QString escape( const QString& str );

            Q_INVOKABLE void setLyricsForTrack( const QString& trackUrl , const QString& lyrics ) const;
            Q_INVOKABLE QString toUtf8( const QByteArray& lyrics, const QString& encoding = QStringLiteral("UTF-8") );
            Q_INVOKABLE QString QStringtoUtf8( const QString& lyrics, const QString& encoding = QStringLiteral("UTF-8") );
            Q_INVOKABLE QByteArray fromUtf8( const QString& str, const QString& encoding );

        Q_SIGNALS:
            void fetchLyrics( const QString& artist, const QString& title, const QString&, Meta::TrackPtr );
    };
}

#endif
