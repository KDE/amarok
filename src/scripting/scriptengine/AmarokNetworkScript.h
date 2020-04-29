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
#include <QJSValue>

class QScriptContext;
class QJSEngine;

namespace AmarokScript
{
    class Downloader : public QObject
    {
        Q_OBJECT

        public:
            explicit Downloader( QJSEngine* scriptEngine );

        private:
            static QJSValue dataDownloader_prototype_ctor( QScriptContext* context, QJSEngine* engine );
            static QJSValue stringDownloader_prototype_ctor( QScriptContext* context, QJSEngine* engine );
            static QJSValue init( QScriptContext* context, QJSEngine* engine, bool stringResult );

            QJSEngine* m_scriptEngine;
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
            void newStringDownload( const QUrl &url, QJSEngine* engine, const QJSValue &obj, const QString &encoding = QStringLiteral("UTF-8") );
            void newDataDownload( const QUrl &url, QJSEngine* engine, const QJSValue &obj );

        private Q_SLOTS:
            void resultString( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
            void resultData( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );

            void requestRedirected( const QUrl &sourceUrl, const QUrl &targetUrl );

        private:
            void cleanUp( const QUrl &url );

            template<typename Function>
            void newDownload( const QUrl &url, QJSEngine* engine, const QJSValue &obj, Function slot )
            {
                m_values[ url ] = obj;
                m_engines[ url ] = engine;

                The::networkAccessManager()->getData( url, this, slot );
            }

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

            QHash<QUrl, QJSEngine *> m_engines;
            QHash<QUrl, QJSValue> m_values;
            QHash<QUrl, QString> m_encodings;
    };
}

Q_DECLARE_METATYPE( AmarokScript::Downloader* )

#endif
