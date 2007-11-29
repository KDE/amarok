/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/
#ifndef DAAPPROXY_H
#define DAAPPROXY_H

class ProcIO;

#include <QObject>
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
            ProcIO* m_proxy;
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
