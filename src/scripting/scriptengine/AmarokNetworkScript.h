/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_NETWORK_SCRIPT_H
#define AMAROK_NETWORK_SCRIPT_H

#include "network/NetworkAccessManagerProxy.h"

#include <QUrl>

#include <QHash>
#include <QList>
#include <QObject>
#include <QScriptValue>

class QScriptContext;
class QScriptEngine;

namespace AmarokScript
{
    class Downloader : public QObject
    {
        Q_OBJECT

        public:
            Downloader( QScriptEngine* scriptEngine );

        private:
            static QScriptValue dataDownloader_prototype_ctor( QScriptContext* context, QScriptEngine* engine );
            static QScriptValue stringDownloader_prototype_ctor( QScriptContext* context, QScriptEngine* engine );
            static QScriptValue init( QScriptContext* context, QScriptEngine* engine, bool stringResult );

            QScriptEngine* m_scriptEngine;
    };

    // this internal class manages multiple downloads from a script.
    // keeps track of each unique download
    class AmarokDownloadHelper : public QObject
    {
        Q_OBJECT

        static AmarokDownloadHelper *s_instance;

        public:
            AmarokDownloadHelper();

            static AmarokDownloadHelper *instance();

            // called by the wrapper class to register a new download
            void newStringDownload( const QUrl &url, QScriptEngine* engine, QScriptValue obj, QString encoding = "UTF-8" );
            void newDataDownload( const QUrl &url, QScriptEngine* engine, QScriptValue obj );

        private slots:
            void resultString( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
            void resultData( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

            void requestRedirected( const QUrl &sourceUrl, const QUrl &targetUrl );

        private:
            void cleanUp( const QUrl &url );
            void newDownload( const QUrl &url, QScriptEngine* engine, QScriptValue obj, const char *slot );

            /**
            * Template function which updates the given @p sourceUrl to the given
            * @p targetUrl. All entries in the given hash will be copied.
            * The old entries will be removed.
            *
            * @param hash The hash which contains all elements.
            * @param sourceUrl The old URL (= the old key).
            * @param targetUrl The new URL (= the new key).
            */
            template<typename T> void updateUrl( QHash< QUrl, T > &hash, const QUrl &sourceUrl, const QUrl &targetUrl )
            {
                // Get all entries with the source URL as key.
                QList< T > data = hash.values( sourceUrl );

                foreach( T entry, data )
                {
                    // Copy each entry to a new one with the
                    // new URL as key.
                    hash[ targetUrl ] = entry;
                }

                // Remove all entries which are still pointing
                // to the source URL.
                hash.remove( sourceUrl );
            };

            QHash<QUrl, QScriptEngine *> m_engines;
            QHash<QUrl, QScriptValue> m_values;
            QHash<QUrl, QString> m_encodings;
    };
}

Q_DECLARE_METATYPE( AmarokScript::Downloader* )

#endif
