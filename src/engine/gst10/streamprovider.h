/***************************************************************************
                     streamprovider.h - shoutcast streaming client
                        -------------------
begin                : Nov 20 14:35:18 CEST 2003
copyright            : (C) 2003 by Mark Kretschmann, (small) portions (C) 2006 by Paul Cifarelli
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_STREAMPROVIDER_H
#define AMAROK_STREAMPROVIDER_H

#include <kurl.h>             //stack allocated

#include <QObject>
#include <q3serversocket.h>    //baseclass
#include <q3socket.h>          //stack allocated
#include <kresolver.h> // namespace

class MetaBundle;
class GstEngine;

using namespace KNetwork;
    /**
     * Stream radio client, receives shoutcast streams and extracts metadata.
     *
     * Provider Concept:
     * 1. Connect to streamserver
     * 2. Write GET request to streamserver, containing Icy-MetaData:1 token
     * 3. Read MetaInt token from streamserver (==metadata offset)
     *
     * 4. Read stream data (mp3 + metadata) from streamserver
     * 5. Filter out metadata, send to app
     * 6. Send stream data to application
     * 7. Goto 4
     *
     * Some info on the shoutcast metadata protocol can be found at:
     * @see http://www.smackfu.com/stuff/programming/shoutcast.html
     *
     * @short A class for handling Shoutcast streams and metadata.
     */
    class StreamProvider : public QObject
    {
            Q_OBJECT
        public:
            /**
             * Constructs a StreamProvider.
             * @param url URL of stream server
             * @param streamingMode The class has two modes of transferring stream data to the application:
             *                      signal: Transfer the data directly via Qt SIGNAL.
             *                              (This mode is to be preferred.)
             *                      socket: Sets up proxy server and writes the data to the proxy.
             *                              (This mode is only needed by aRts-engine.)
             */
            StreamProvider( KUrl url, const QString& streamingMode, GstEngine &engine );
            ~StreamProvider();

            /** Returns true if initialisation was successful */
            bool initSuccess() { return m_initSuccess; }

        signals:
            /** Transmits new metadata bundle */
            void metaData( const MetaBundle& );

            /** Transmits chunk of audio data */
            void streamData( char*, int size );

            /** Signals an error: StreamProvider cannot operate on this stream. */
            void sigError();

        private slots:
            void resolved(KResolverResults result);
            void connectToHost();
            void sendRequest();
            void readRemote();
            void connectError();

        private:
            // const symbols
            static const uint MIN_PROXYPORT = 6700;
            static const uint MAX_PROXYPORT = 7777;
            static const int  BUFSIZE       = 16384;

            bool processHeader( Q_LONG &index, Q_LONG bytesRead );
            void transmitData( const QString &data );
            void restartNoIcy();

            /**
             * Find key/value pair in string and return value.
             * @param str String to search through.
             * @param key Key to find.
             * @return The value, QString:null if key not found.
             */
            QString extractStr( const QString &str, const QString &key ) const;

        //ATTRIBUTES:
            KUrl m_url;
            QString m_streamingMode;
            bool m_initSuccess;
            bool m_connectSuccess;

            int m_metaInt;
            int m_bitRate;
            int m_byteCount;
            uint m_metaLen;

            QString m_metaData;
            bool m_headerFinished;
            QString m_headerStr;
            int m_usedPort;
            bool m_icyMode;

            QString m_streamName;
            QString m_streamGenre;
            QString m_streamUrl;

            char *m_pBuf;

            Q3Socket m_sockRemote;
            Q3Socket m_sockProxy;
            KResolver m_resolver;

            GstEngine &m_engine;
    };

#endif /*AMAROK_STREAMPROVIDER_H*/

