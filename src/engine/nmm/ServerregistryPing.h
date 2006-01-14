/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2002-2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * Maintainer:        Robert Gogolok <gogo@graphics.cs.uni-sb.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#ifndef SERVERREGISTRYPING_H 
#define SERVERREGISTRYPING_H 

#include <qsocket.h>

/**
 * Connects to a remote host on the default NMM serverregistry port.
 */
class ServerregistryPing
    : public QSocket
{
    Q_OBJECT

    public:
        ServerregistryPing(const QString & host, Q_UINT16 port = 22801);

    private slots:
        void socketConnected();
        void socketConnectionClosed();
        void socketError(int);

    signals:
        /**
         * This signal is emitted when the serverregistry gets available/unavailable.
         */
        void registryAvailable(bool);
};

#endif
