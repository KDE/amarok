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

#ifndef AMAROK_DAAPSERVER_H
#define AMAROK_DAAPSERVER_H

#include <daapclient.h>

class ProcIO;

namespace DNSSD {
    class PublicService;
}

class DaapServer : public QObject
{
    Q_OBJECT

    public:
        DaapServer(QObject* parent, char* name);
        ~DaapServer();
    public slots:
        void readSql();
    private:
        ProcIO* m_server;
        #ifdef DNSSD_SUPPORT
            DNSSD::PublicService* m_service;
        #endif
};

#endif /* AMAROK_DAAPSERVER_H */

