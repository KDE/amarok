/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#define DEBUG_PREFIX "UpnpCollectionBase"

#include "UpnpCollectionBase.h"

#include <kio/upnptypes.h>
#include <kio/scheduler.h>
#include <kio/jobclasses.h>
#include <kio/slave.h>

#include "core/support/Debug.h"

namespace Collections {

UpnpCollectionBase::UpnpCollectionBase( Solid::Device dev )
    : Collection()
    , m_device( dev )
    , m_slave( 0 )
    , m_slaveConnected( false )
{
    KIO::Scheduler::connect( SIGNAL(slaveError(KIO::Slave*,int,QString)),
                             this, SLOT(slotSlaveError(KIO::Slave*,int,QString)) );
    KIO::Scheduler::connect( SIGNAL(slaveConnected(KIO::Slave*)),
                             this, SLOT(slotSlaveConnected(KIO::Slave*)) );
    m_slave = KIO::Scheduler::getConnectedSlave( collectionId() );
}

UpnpCollectionBase::~UpnpCollectionBase()
{
    if( m_slave ) {
        KIO::Scheduler::disconnectSlave( m_slave );
        m_slave = 0;
        m_slaveConnected = false;
    }
}

QString UpnpCollectionBase::collectionId() const
{
    return QString("upnp-ms://") + m_device.udi().replace("/org/kde/upnp/uuid:", "");
}

QString UpnpCollectionBase::prettyName() const
{
    return m_device.product();
}

bool UpnpCollectionBase::possiblyContainsTrack( const KUrl &url ) const
{
    if( url.scheme() == "upnp-ms" )
//         && url.host() == m_device.host()
//         && url.port() == m_device.port() )
        return true;
    return false;
}

void UpnpCollectionBase::assignJob( KIO::SimpleJob *job )
{
    KIO::Scheduler::assignJobToSlave( m_slave, job );
}

void UpnpCollectionBase::slotSlaveError(KIO::Slave* slave, int err, const QString& msg)
{
    debug() << "SLAVE ERROR" << slave << err << msg;
    if( m_slave != slave )
        return;

    if( err == KIO::ERR_COULD_NOT_CONNECT
        || err == KIO::ERR_CONNECTION_BROKEN ) {
        debug() << "COULD NOT CONNECT TO " << msg << "REMOVING THE COLLECTION";
        emit remove();
    }

    if( err == KIO::ERR_SLAVE_DIED ) {
        m_slave = 0;
        emit remove();
    }
}

void UpnpCollectionBase::slotSlaveConnected(KIO::Slave* slave)
{
    if( m_slave != slave )
        return;

    debug() << "SLAVE IS CONNECTED";
    m_slaveConnected = true;
}

} //namespace Collections
