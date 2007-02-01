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
#include <q3serversocket.h>
class DaapClient;

namespace Daap {

    class Proxy : public QObject
    {
        Q_OBJECT

        public:
            Proxy(KUrl stream, DaapClient* client, const char* name);
            ~Proxy();
            KUrl proxyUrl() { return m_proxyUrl; }
            static KUrl realStreamUrl( KUrl fakeStream, int sessionId );

        public slots:
            void playbackStopped();
            void readProxy();

        private:
            KUrl m_proxyUrl;
            Amarok::ProcIO* m_proxy;
    };

    // We must implement this because QServerSocket has one pure virtual method.
    // It's just used for finding a free port.
    class MyServerSocket : public Q3ServerSocket
    {
        public:
            MyServerSocket() : Q3ServerSocket( quint16( 0 ) ) {}

        private:
            void newConnection( int ) {}

    };

}

#endif
