/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef DAAPPROXY_H
#define DAAPPROXY_H

#include <qobject.h>
#include <qserversocket.h>
class DaapClient;

namespace Daap {

    class Proxy : public QObject
    {
        Q_OBJECT

        public:
            Proxy(KURL stream, DaapClient* client, const char* name);
            ~Proxy();
            KURL proxyUrl() { return m_proxyUrl; }
            static KURL realStreamUrl( KURL fakeStream, int sessionId );

        public slots:
            void playbackStopped();
            void readProxy();

        private:
            KURL m_proxyUrl;
            Amarok::ProcIO* m_proxy;
    };

    // We must implement this because QServerSocket has one pure virtual method.
    // It's just used for finding a free port.
    class MyServerSocket : public QServerSocket
    {
        public:
            MyServerSocket() : QServerSocket( Q_UINT16( 0 ) ) {}

        private:
            void newConnection( int ) {}

    };

}

#endif
