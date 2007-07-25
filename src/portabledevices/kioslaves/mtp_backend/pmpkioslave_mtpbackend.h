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
#include <QMultiHash>

#include <solid/device.h>

#include "pmpbackend.h"

#include "libmtp.h"

class PMPProtocol;

class MTPBackend : public PMPBackend
{
    Q_OBJECT

    public:
        MTPBackend( PMPProtocol* slave, const Solid::Device &device );
        virtual ~MTPBackend();
        void initialize();
        QString getFriendlyName();
        void setFriendlyName( const QString &name );
        QString getModelName();

    protected:
        void get( const KUrl &url );
        void listDir( const KUrl &url );
        void stat( const KUrl &url );
        void rename( const KUrl &src, const KUrl &dest, bool overwrite );

    private:
        void listMusic( const QString &pathOffset );
        void buildMusicListing();
        void buildFolderList( LIBMTP_folder_t *folderList, const QString &parentPath );
        static int progressCallback( quint64 const sent, quint64 const total, void const * const data );

        LIBMTP_mtpdevice_t *m_deviceList;
        LIBMTP_mtpdevice_t *m_device;
        bool m_gotTracklisting;
        QString m_defaultMusicLocation;

        QMultiHash<QString, LIBMTP_track_t*> m_trackParentToPtrHash;
        QMultiHash<quint32, QString> m_folderIdToPathHash;
        QMultiHash<QString, LIBMTP_folder_t*> m_folderParentToPtrHash;

};

#endif /* MTP_BACKEND_H */

