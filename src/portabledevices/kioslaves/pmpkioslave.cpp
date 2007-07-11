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

#include "pmpbackend.h"
#include "pmpkioslave.h"

#include <QCoreApplication>
#include <QString>

#include <kcomponentdata.h>
#include <kdebug.h>
#include <kio/ioslave_defaults.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/portablemediaplayer.h>

using namespace KIO;

extern "C" int KDE_EXPORT kdemain( int argc, char **argv )
{
    QCoreApplication app( argc, argv );
    KComponentData( "kio_pmp", "kdelibs4" );
    (void) KGlobal::locale();

    fprintf(stderr, "ENTERED kdemain of pmp slave\n" );

    if( argc != 4 )
    {
        fprintf(stderr, "Usage: kio_pmp protocol domain-socket1 domain-socket2\n" );
        exit( -1 );
    }

    PMPProtocol slave( argv[1], argv[2], argv[3] );
    slave.dispatchLoop();
    return 0;
}

PMPProtocol::PMPProtocol( const QByteArray &protocol, const QByteArray &pool,
                          const QByteArray &app )
                        : SlaveBase( protocol, pool, app )
                        , m_backend( 0 )
{
    kDebug() << "Creating PMPPProtocol kioslave" << endl;

}

PMPProtocol::~PMPProtocol()
{
}

void
PMPProtocol::setHost( const QString &host, quint16 port,
         const QString &user, const QString &pass)
{
    Solid::Device sd = Solid::Device( host );
    if( !sd.isValid() )
        error( KIO::ERR_INTERNAL, "Valid UDI not passed in." );

    Solid::PortableMediaPlayer *pmp = sd.as<Solid::PortableMediaPlayer>();
    if( !pmp->isValid() )
        error( KIO::ERR_INTERNAL, "UDI does not describe a Portable Media Player." );

#ifdef HAVE_MTP
    if( pmp->supportedProtocols().contains( "mtp", Qt::CaseInsensitive ) )
        m_backend = MTPType();
    m_backend->setHost( host, port, user, pass );
    return;
#endif

    error( KIO::ERR_INTERNAL, "No supported protocol found." );
}

#include "pmpkioslave.moc"

