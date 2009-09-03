/****************************************************************************************
 * Copyright (c) 2009 Martin Aumueller <aumuell@reserv.at>        y                     *
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

#define DEBUG_PREFIX "IphoneOsCollection"

#include "IphoneOsCollection.h"
#include "support/IphoneOsConnectionAssistant.h"
#include "support/IphoneOsDeviceInfo.h"
#include "handler/IphoneOsHandler.h"
#include "MediaDeviceInfo.h"

#include "amarokconfig.h"
#include "Debug.h"

#include <KUrl>
#include <KStandardDirs>

AMAROK_EXPORT_PLUGIN( IphoneOsCollectionFactory )

IphoneOsCollectionFactory::IphoneOsCollectionFactory()
    : MediaDeviceCollectionFactory<IphoneOsCollection> ( new IphoneOsConnectionAssistant() )
{
    DEBUG_BLOCK
    //nothing to do
}

IphoneOsCollectionFactory::~IphoneOsCollectionFactory()
{
    DEBUG_BLOCK
}

//IphoneOsCollection

IphoneOsCollection::IphoneOsCollection(MediaDeviceInfo* info)
: MediaDeviceCollection()
{
    DEBUG_BLOCK
    /** Fetch Info needed to construct IphoneOsCollection */
    debug() << "Getting ipod info";
    IphoneOsDeviceInfo *ipodinfo = qobject_cast<IphoneOsDeviceInfo *>( info );

    debug() << "Getting mountpoint";
    m_mountPoint = ipodinfo->mountpoint();

    if (m_mountPoint.isEmpty())
    {
        m_mountPoint = KStandardDirs::locateLocal( "data", "amarok/tmp/" );
        m_mountPoint += "iphone";
        debug() << "set mountpoint to " << m_mountPoint;

        QDir mp(m_mountPoint);
        if(!mp.exists())
        {
            mp.mkpath(m_mountPoint);
            debug() << "created " << m_mountPoint;
        }
    }


    debug() << "Getting udi";
    m_udi = ipodinfo->udi();

    debug() << "constructing handler";

    m_handler = new Meta::IphoneOsHandler( this, m_mountPoint );

    //emit attemptConnectionDone( m_handler->succeeded() );
}


bool
IphoneOsCollection::possiblyContainsTrack( const KUrl &url ) const
{
    // We could simply check for iPod_Control except that we could actually have multiple ipods connected
    return url.url().startsWith( m_mountPoint ) || url.url().startsWith( "file://" + m_mountPoint );
}

Meta::TrackPtr
IphoneOsCollection::trackForUrl( const KUrl &url )
{
    QString uid = url.url();
    if( uid.startsWith("file://") )
        uid = uid.remove( 0, 7 );
    Meta::TrackPtr iphoneTrack = m_trackMap.value( uid );
    return iphoneTrack ? iphoneTrack : Collection::trackForUrl(url);
}

IphoneOsCollection::~IphoneOsCollection()
{
    DEBUG_BLOCK
}

QString
IphoneOsCollection::collectionId() const
{
     return m_mountPoint;
}

QString
IphoneOsCollection::prettyName() const
{
//    return "IphoneOs at " + m_mountPoint;
    return m_handler->prettyName();
}

#include "IphoneOsCollection.moc"

