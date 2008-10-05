/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef CACHEDHTTP_H
#define CACHEDHTTP_H

#include "UnicornDllExportMacro.h"

#include <QDebug>
#include <QHttp>
#include <QHash>
#include <QStack>
#include <QString>

#include "RedirectHttp.h"

class UNICORN_DLLEXPORT CachedHttp : public RedirectHttp
{
    Q_OBJECT

    public:

        enum ProxyOverrideState
        {
            NONE = 0,
            PROXYON,
            PROXYOFF
        };

        CachedHttp( QObject* parent = 0 );
        CachedHttp( const QString& hostName, int port = 80, QObject* parent = 0, ProxyOverrideState proxyOverride = NONE );
        ~CachedHttp();

        int get( const QString& path, bool useCache = false );
        int get( const QString& path, QIODevice* to );
        int post( const QString& path, QIODevice* data );
        int post( const QString& path, const QByteArray& data );
        int request( const QHttpRequestHeader& header, QIODevice* data = 0, QIODevice* to = 0 );
        int request( const QHttpRequestHeader& header, const QByteArray& data, QIODevice* to = 0, bool useCache = false );

        int setHost( const QString & hostName, quint16 port = 80 ) { m_hostname = hostName; return QHttp::setHost( hostName, port ); }
        QByteArray readAll() { checkBuffer(); QByteArray b = m_buffer; m_buffer.clear(); return b; }

        virtual qint64 bytesAvailable() { return m_buffer.isEmpty() ? QHttp::bytesAvailable() : m_buffer.size(); }

        QString host() const { return m_hostname; }

        /// The application needs to call these at initialisation time if it
        /// doesn't want to use the default strings for user agent and cache path.
        /// Default strings are constructed using organizationName and applicationName.
        static void setCustomUserAgent( QString a ) { s_customUserAgent = a; }
        static void setCustomCachePath( QString p ) { s_customCachePath = p; }

    public slots:
        void abort();

    signals:
        void errorOccured( int error, const QString& errorString );
        void dataAvailable( const QByteArray& buffer );

    private slots:
        void requestDone( bool error );
        void dataFinished( int id, bool error );
        void headerReceived( const QHttpResponseHeader & resp );
        void getFromCache();

    private:
        class CachedRequestData
        {
        public:
            int m_id;
            QString m_cacheKey;

            CachedRequestData()
                    : m_id( -1 )
            {}

            CachedRequestData( int id, QString cacheKey )
                    : m_id( id )
                    , m_cacheKey( cacheKey )
            {}
        };

        int m_dataID;
        QByteArray m_buffer;
        QString m_hostname;
        int m_statuscode;
        int m_expireDate;

        ProxyOverrideState m_proxyOverride;

        // Used for cached metadata requests
        QHash<int, CachedRequestData> m_requestStack;
        QStack<CachedRequestData> m_cacheStack;
        int m_nextId; // returned by RequestMetaDataArtist & Track

        void init();
        void applyProxy();
        void applyUserAgent( QHttpRequestHeader& header );
        void checkBuffer() { if ( QHttp::bytesAvailable() ) m_buffer = QHttp::readAll(); }

        QString userAgent();
        QString cachePath();

        QString pathToCachedCopy( QString cacheKey );
        bool haveCachedCopy( QString url );
        void putCachedCopy( QString cacheKey, const QByteArray& data );

        bool m_inProgress;

        static QString s_customUserAgent; // whole name
        static QString s_customCachePath; // whole path
};


inline QDebug operator<<( QDebug& d, CachedHttp* http )
{
    d << "  Http response: " << http->lastResponse().statusCode() << "\n"
      << "  QHttp error code: " << http->error() << "\n"
      << "  QHttp error text: " << http->errorString() << "\n"
      << "  Request: " << http->host() + http->currentRequest().path() << "\n"
      << "  Bytes returned: " << http->bytesAvailable();

    return d;
}

#endif // CACHEDHTTP_H
