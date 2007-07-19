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
#ifndef MTP_BACKEND_H
#define MTP_BACKEND_H

#include <QtCore/QByteRef>

#include <solid/device.h>

#include "pmpbackend.h"

#include "libmtp.h"

class MTPBackend : public PMPBackend
{
    Q_OBJECT

    public:
        MTPBackend();
        virtual ~MTPBackend();

    protected:
        void setUdi( const QString &udi);
        void get( const KUrl &url );
        void listDir( const KUrl &url );
        void stat( const KUrl &url );

    private:
        LIBMTP_mtpdevice_t *m_deviceList;
        LIBMTP_mtpdevice_t *m_device;
        Solid::Device       m_solidDevice;

};

#endif /* MTP_BACKEND_H */

