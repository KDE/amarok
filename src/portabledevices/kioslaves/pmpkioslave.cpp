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
#include <solid/genericinterface.h>
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
                        , m_initialized( false )
                        , m_backend( 0 )
{
    kDebug() << "Creating PMPPProtocol kioslave" << endl;

}

PMPProtocol::~PMPProtocol()
{
    if( m_backend )
        delete m_backend;

    m_backend = 0;
}

void
PMPProtocol::setHost( const QString &host, quint16 port,
                      const QString &user, const QString &pass )
{
    Q_UNUSED( port );
    Q_UNUSED( user );
    Q_UNUSED( pass );
    if( !host.isEmpty() )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING,
                "portable media player : Setting a hostname is not supported.  Use a URL of the form pmp:/<udi> or pmp:///<udi>" );
    }
}

void
PMPProtocol::get( const KUrl &url )
{
    kDebug() << endl << endl << "Entering get with url = " << url << endl << endl;
    if( !m_initialized )
        initialize( url );
    m_backend->get( url );
    kDebug() << endl << endl << "Leaving get with url = " << url << endl << endl;
}

void
PMPProtocol::listDir( const KUrl &url )
{
    kDebug() << endl << endl << "Entering listDir with url = " << url << endl;
    kDebug() << "path = " << url.path() << ", path.isEmpty = " << (url.path().isEmpty() ? "true" : "false" ) << endl;
    if( url.isEmpty() )
        return;
    if( !m_initialized )
        initialize( url );
    if( url.path().isEmpty() || url.path() == "/" )
    {
        kDebug() << "Listing devices" << endl;
        QList<Solid::Device> deviceList = Solid::Device::listFromQuery( "is PortableMediaPlayer" );
        foreach( Solid::Device device, deviceList )
        {
            UDSEntry entry;
            QString name = getFriendlyName( device );
            entry[ KIO::UDS_NAME ] = name.isEmpty() ? QString( "Portable Media Player at " + device.udi().replace( QChar( '/' ), QChar( '.' ) ) ) : name;
            entry[ KIO::UDS_URL ] = "pmp:///" + device.udi().replace( QChar( '/' ), QChar( '.' ) );
            entry[ KIO::UDS_FILE_TYPE ] = S_IFDIR;
            entry[ KIO::UDS_HIDDEN ] = 0;
            listEntry( entry, false );
        }
        listEntry( UDSEntry(), true );
        emit finished();
        return;
    }
    else
    {
        kDebug() << "Calling backend's listDir" << endl;
        m_backend->listDir( url );
    }
    kDebug() << endl << endl << "Leaving listDir with url = " << url << endl << endl;
}

QString
PMPProtocol::getFriendlyName( Solid::Device device )
{
    kDebug() << "Looking for a friendly name for " << device.udi() << "..." << endl;
    Solid::GenericInterface *gi = device.as<Solid::GenericInterface>();
    if( !gi )
    {
        kDebug() << "Couldn't get the device as a generic interface." << endl;
        return QString();
    }
    if( !gi->propertyExists( "portable_audio_player.access_method.drivers" ) )
    {
        kDebug() << "No access_method.drivers property detected." << endl;
        return QString();
    }
    QVariant possibleLibraries = gi->property( "portable_audio_player.access_method.drivers" );
    if( !possibleLibraries.isValid() || possibleLibraries.isNull() || !possibleLibraries.canConvert( QVariant::StringList ) )
    {
        kDebug() << "drivers list not valid, null, or can't convert" << endl;
        return QString();
    }
    QStringList libraries = possibleLibraries.toStringList();
    kDebug() << "Found libraries: " << libraries << endl;
    foreach( QString library, libraries )
    {
        QVariant possibleName = gi->property( "portable_audio_player." + library + ".name" );
        if( possibleName.isValid() && !possibleName.isNull() && possibleName.canConvert( QVariant::String ) )
        {
            QString name = possibleName.toString();
            if( !name.isEmpty() )
                return name;
        }
    }
    return QString();
}

void
PMPProtocol::stat( const KUrl &url )
{
    kDebug() << endl << endl << "Entering stat with url = " << url << endl << endl;
    if( !m_initialized )
        initialize( url );
    if( url.path().isEmpty() || url.path() == "/" )
    {
        KIO::UDSEntry entry;
        entry[ KIO::UDS_NAME ] = QString( "Available Devices" );
        entry[ KIO::UDS_FILE_TYPE ] = S_IFDIR;
        entry[ KIO::UDS_ACCESS ] = S_IRUSR | S_IRGRP | S_IROTH;
        statEntry( entry );
        emit finished();
    }
    else
        m_backend->stat( url );
    kDebug() << endl << endl << "Leaving stat with url = " << url << endl << endl;
}

void
PMPProtocol::initialize( const KUrl &url )
{

    kDebug() << endl << endl << "url: " << url << endl << endl;
    QString path = url.path( KUrl::RemoveTrailingSlash );
    if( url.isEmpty() )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, "portable media player : Empty UDI passed in" );
        return;
    }

    //start out clean
    while( path[0] == '.' || path[0] == '/' )
        path.remove( 0, 1 );

    kDebug() << endl << endl << "Path: " << path << endl << endl;

    int index = path.indexOf( '/' );
    //if not found, use the path as is; if it is truncate so we only get the udi
    QString transUdi = ( index == -1 ? path : path.left( index ) );
    //translate the udi to its required format
    transUdi.replace( QChar( '.' ), QChar( '/' ) );
    //and now make sure it starts with a /
    transUdi = '/' + transUdi;

    kDebug() << endl << endl << "Using udi: " << transUdi << endl << endl;

    //if they didn't enter a udi, display available ones -- listDir will take care of it
    if( transUdi.isEmpty() || transUdi == "/" )
        return;

    Solid::Device sd = Solid::Device( transUdi );
    if( !sd.isValid() )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING,
                "portable media player : Device not found by Solid.  Ensure the device is turned on, you have permission to access it, and that the UDI's forward slashes are replaced by periods" );
        return;
    }

    Solid::PortableMediaPlayer *pmp = sd.as<Solid::PortableMediaPlayer>();
    if( !pmp )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, "device : Device is not a portable media player" );
        return;
    }

#ifdef HAVE_MTP
    if( pmp->supportedProtocols().contains( "mtp", Qt::CaseInsensitive ) )
    {
        m_backend = MTPType();
        m_backend->setSlave( this );
        m_backend->setUdi( transUdi );
        m_initialized = true;
        return;
    }
#endif

    error( KIO::ERR_CANNOT_OPEN_FOR_READING, "device : No supported protocol found" );
    return;
}

#include "pmpkioslave.moc"

