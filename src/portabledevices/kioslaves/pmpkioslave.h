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
#ifndef PMP_KIOSLAVE_H
#define PMP_KIOSLAVE_H

#include "pmpbackend.h"

#ifdef HAVE_MTP 
#include "mtp_backend/pmpkioslave_mtpbackend.h"
#endif

#include <QtCore/QByteRef>

#include <kurl.h>
#include <kio/slavebase.h>
#include <solid/device.h>

class PMPDevice;

class PMPProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

    public:
        PMPProtocol( const QByteArray &protocol, const QByteArray &pool,
                     const QByteArray &app );
        virtual ~PMPProtocol();
        bool mtpInitialized() const { return m_mtpInitialized; }
        void setMtpInitialized( const bool inited ) { m_mtpInitialized = inited; }

    protected:
        void setHost( const QString &host, quint16 port,
                      const QString &user, const QString &pass );

        void get( const KUrl &url );
        void listDir( const KUrl &url );
        void rename( const KUrl &src, const KUrl &dest, bool overwrite );
        void stat( const KUrl &url );

    private:
        QString getSolidFriendlyName( const Solid::Device &device ) const;
        void initialize( const KUrl &url );
        inline QString transUdi( const QString &udi ) const { return QString( udi ).replace( QChar( '/' ), QChar( '.' ) ); }
        QString udiFromUrl( const KUrl &url );
        PMPBackend* getBackendForUrl( const KUrl &url );

        QMap<QString,PMPDevice*> m_devices;

        //libmtp should never be inited more than once from the same process but we support
        //multiple devices, so make sure
        bool m_mtpInitialized;
};

#endif /* PMP_KIOSLAVE_H */

