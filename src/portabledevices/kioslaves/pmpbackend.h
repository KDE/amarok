/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef PMP_BACKEND_H
#define PMP_BACKEND_H

#include <QObject>
#include <QString>

#include <kio/slavebase.h>

class PMPBackend : public QObject
{
    Q_OBJECT

    public:
        PMPBackend();
        virtual ~PMPBackend();

        void setSlave( KIO::SlaveBase *slave ) { m_slave = slave; }
        virtual void setUdi( const QString &udi );

        virtual void get( const KUrl &url ) {};
        virtual void listDir( const KUrl &url ) {};

        KIO::SlaveBase *m_slave;

};

#endif /* PMP_BACKEND_H */

