/***************************************************************************
                      titleproxy.h  -  description
                         -------------------
begin                : Nov 20 14:35:18 CEST 2003
copyright            : (C) 2003 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TITLEPROXY_H
#define TITLEPROXY_H

#include <kurl.h>

#include <qobject.h>
#include <qserversocket.h>    //baseclass
#include <qsocket.h>          //stack allocated

class QString;
class MetaBundle;

/**
 * @brief Proxy for decoding Shoutcast metadata.
 */

namespace TitleProxy
{
    class Proxy : public QObject
    {
        Q_OBJECT
        public:
            Proxy( KURL url, int streamingMode );
            ~Proxy();

            KURL proxyUrl();

        signals:
            void metaData( const MetaBundle& );
            void streamData( char*, int size );
            
        public slots:

        private slots:
            void readRemote();
            bool processHeader( Q_LONG &index, Q_LONG bytesRead );
            void accept( int socket );
            void sendRequest( bool meta );

        private:
            void error();
            void transmitData( const QString &data );
            QString extractStr( const QString &str, const QString &key );

// ATTRIBUTES ------
            KURL            m_url;
            int             m_streamingMode;
            bool            m_initSuccess;
            int             m_metaInt;
            int             m_bitRate;
            int             m_byteCount;
            uint            m_metaLen;
            QString         m_metaData;
            bool            m_headerFinished;
            QString         m_headerStr;
            int             m_usedPort;
            QString         m_lastMetadata;

            QString         m_streamName;
            QString         m_streamGenre;
            QString         m_streamUrl;
            
            char            *m_pBuf;

            QSocket m_sockRemote;
            QSocket m_sockProxy;
    };


    class Server : public QServerSocket
    {
        Q_OBJECT
        
        public:   
            Server( Q_UINT16 port, QObject* parent )
            : QServerSocket( port, 1, parent ) {};
        
        signals:
            void connected( int socket );
            
        private:
            void newConnection( int socket ) { emit connected( socket ); } 
    };


} //namespace TitleProxy

#endif
