/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "IpodCollection"

#include "IpodCollection.h"
#include "IpodConnectionAssistant.h"
#include "IpodDeviceInfo.h"
#include "MediaDeviceInfo.h"

#include "amarokconfig.h"
#include "Debug.h"

#include <KUrl>
#include <KStandardDirs>

AMAROK_EXPORT_COLLECTION( IpodCollectionFactory, ipodcollection )

IpodCollectionFactory::IpodCollectionFactory( QObject *parent, const QVariantList &args )
    : MediaDeviceCollectionFactory<IpodCollection> ( new IpodConnectionAssistant() )
{
    setParent( parent );
    Q_UNUSED( args )
}

IpodCollectionFactory::~IpodCollectionFactory()
{
}

//IpodCollection

IpodCollection::IpodCollection( MediaDeviceInfo* info )
    : MediaDeviceCollection()
{
    DEBUG_BLOCK
    /** Fetch Info needed to construct IpodCollection */
    IpodDeviceInfo *ipodinfo = qobject_cast<IpodDeviceInfo *>( info );

    m_mountPoint = ipodinfo->mountPoint();
    debug() << "Mounted at: " << m_mountPoint;
    m_udi = ipodinfo->udi();

    bool isMounted = true;
    if (m_mountPoint.isEmpty())
    {
        isMounted = false;
        m_mountPoint = KStandardDirs::locateLocal( "tmp", "amarok/" );
        m_mountPoint += "imobiledevice";
        if( !ipodinfo->deviceUid().isEmpty() )
            m_mountPoint += '_' + ipodinfo->deviceUid();
        debug() << "set mountpoint to " << m_mountPoint;

        QDir mp(m_mountPoint);
        if(!mp.exists())
        {
            mp.mkpath(m_mountPoint);
            debug() << "created " << m_mountPoint;
        }

        ipodinfo->setMountPoint( m_mountPoint );
    }

    m_handler = new Meta::IpodHandler( this, ipodinfo );
}


bool
IpodCollection::possiblyContainsTrack( const KUrl &url ) const
{
    QString u = QUrl::fromPercentEncoding( url.url().toUtf8() );
    // We could simply check for iPod_Control except that we could actually have multiple ipods connected
    return u.startsWith( m_mountPoint ) || u.startsWith( "file://" + m_mountPoint );
}

Meta::TrackPtr
IpodCollection::trackForUrl( const KUrl &url )
{
    QString uid = QUrl::fromPercentEncoding( url.url().toUtf8() );
    if( uid.startsWith("file://") )
        uid = uid.remove( 0, 7 );
    Meta::TrackPtr ipodTrack = m_trackMap.value( uid );
    return ipodTrack ? ipodTrack : Collection::trackForUrl(url);
}

IpodCollection::~IpodCollection()
{
    DEBUG_BLOCK
}

QString
IpodCollection::collectionId() const
{
     return m_mountPoint;
}

QString
IpodCollection::prettyName() const
{
    return m_handler->prettyName();
}

#include "IpodCollection.moc"

