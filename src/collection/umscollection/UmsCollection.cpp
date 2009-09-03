/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#define DEBUG_PREFIX "UmsCollection"

#include "UmsCollection.h"
#include "UmsConnectionAssistant.h"
#include "UmsDeviceInfo.h"
#include "MediaDeviceInfo.h"

#include "amarokconfig.h"
#include "Debug.h"

#include <KUrl>

AMAROK_EXPORT_PLUGIN( UmsCollectionFactory )

UmsCollectionFactory::UmsCollectionFactory()
    : MediaDeviceCollectionFactory<UmsCollection> ( new UmsConnectionAssistant() )
{
}

UmsCollectionFactory::~UmsCollectionFactory()
{
}

//UmsCollection

UmsCollection::UmsCollection(MediaDeviceInfo* info)
    : MediaDeviceCollection()
{
    DEBUG_BLOCK
    /** Fetch Info needed to construct UmsCollection */
    UmsDeviceInfo *umsinfo = qobject_cast<UmsDeviceInfo *>( info );

    m_mountPoint = umsinfo->mountpoint();
    debug() << "Mounted at: " << m_mountPoint;
    m_udi = umsinfo->udi();

    m_handler = new Meta::UmsHandler( this, m_mountPoint );
}


bool
UmsCollection::possiblyContainsTrack( const KUrl &url ) const
{
    QString u = QUrl::fromPercentEncoding( url.url().toUtf8() );
    return u.startsWith( m_mountPoint ) || u.startsWith( "file://" + m_mountPoint );
}

Meta::TrackPtr
UmsCollection::trackForUrl( const KUrl &url )
{
    QString uid = QUrl::fromPercentEncoding( url.url().toUtf8() );
    if( uid.startsWith("file://") )
        uid = uid.remove( 0, 7 );
    Meta::TrackPtr umsTrack = m_trackMap.value( uid );
    return umsTrack ? umsTrack : Collection::trackForUrl(url);
}

UmsCollection::~UmsCollection()
{
    DEBUG_BLOCK
}

QString
UmsCollection::collectionId() const
{
     return m_mountPoint;
}

QString
UmsCollection::prettyName() const
{
    return m_handler->prettyName();
}

#include "UmsCollection.moc"

