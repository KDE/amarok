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

#include "mtp_kioslave.h"

#include <QCoreApplication>
#include <QString>

#include <kcomponentdata.h>
#include <kdebug.h>
#include <kio/ioslave_defaults.h>

using namespace KIO;

extern "C" int KDE_EXPORT kdemain( int argc, char **argv )
{
    QCoreApplication app( argc, argv );
    KComponentData( "kio_mtp", "kdelibs4" );
    (void) KGlobal::locale();

    if( argc != 4 )
    {
        fprintf(stderr, "Usage: kio_mtp protocol domain-socket1 domain-socket2\n" );
        exit( -1 );
    }

    MTPProtocol slave( argv[1], argv[2], argv[3] );
    slave.dispatchLoop();
    return 0;
}

MTPProtocol::MTPProtocol( const QByteArray &protocol, const QByteArray &pool,
                          const QByteArray &app )
                        : SlaveBase( protocol, pool, app )
                        , m_device( 0 )
                        , m_deviceCount( 0 )
{
    kDebug() << "Creating MTPProtocol kioslave" << endl;

    LIBMTP_Init();

    if( LIBMTP_Get_Connected_Devices( &m_deviceList ) != LIBMTP_ERROR_NONE )
        error( KIO::ERR_INTERNAL, "Could not get a connected device list from libmtp." );
    m_deviceCount = LIBMTP_Number_Devices_In_List( m_deviceList );
    if( m_deviceCount == 0 )
        error( KIO::ERR_INTERNAL, "libmtp found no devices." );
}

MTPProtocol::~MTPProtocol()
{
}

void
MTPProtocol::setHost( const QString &host, quint16 port,
                      const QString &user, const QString &pass )
{
    Q_UNUSED(port);
    Q_UNUSED(user);
    Q_UNUSED(pass);
    kDebug() << "host = " << host << endl;
    LIBMTP_mtpdevice_t *currdevice;
    for( currdevice = m_deviceList; currdevice != NULL; currdevice = currdevice->next )
    {
        kDebug() << "currdevice serial number = " << LIBMTP_Get_Serialnumber( currdevice ) << endl;
        if( QString( LIBMTP_Get_Serialnumber( currdevice ) ).compare( host, Qt::CaseInsensitive ) == 0 )
            m_device = currdevice;
        else
            LIBMTP_Release_Device( currdevice );
    }

    if( m_device )
        kDebug() << "FOUND THE MTP DEVICE WE WERE LOOKING FOR!" << endl;
}

#include "mtp_kioslave.moc"

