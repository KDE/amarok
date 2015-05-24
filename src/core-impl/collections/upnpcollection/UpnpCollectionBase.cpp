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

#include "upnptypes.h"
#include <kio/scheduler.h>
#include <kio/jobclasses.h>
#include <kio/slave.h>

#include "core/support/Debug.h"

namespace Collections {

static const int MAX_JOB_FAILURES_BEFORE_ABORT = 5;

UpnpCollectionBase::UpnpCollectionBase( const DeviceInfo& dev )
    : Collection()
    , m_device( dev )
    , m_slave( 0 )
    , m_slaveConnected( false )
    , m_continuousJobFailureCount( 0 )
{
    KIO::Scheduler::connect( SIGNAL(slaveError(KIO::Slave*,int,QString)),
                             this, SLOT(slotSlaveError(KIO::Slave*,int,QString)) );
    KIO::Scheduler::connect( SIGNAL(slaveConnected(KIO::Slave*)),
                             this, SLOT(slotSlaveConnected(KIO::Slave*)) );
    m_slave = KIO::Scheduler::getConnectedSlave( collectionId() );
}

UpnpCollectionBase::~UpnpCollectionBase()
{
    foreach( KIO::SimpleJob *job, m_jobSet )
        KIO::Scheduler::cancelJob( job );
    m_jobSet.clear();
    if( m_slave ) {
        KIO::Scheduler::disconnectSlave( m_slave );
        m_slave = 0;
        m_slaveConnected = false;
    }
}

QString UpnpCollectionBase::collectionId() const
{
    return QString("upnp-ms://") + m_device.uuid();
}

QString UpnpCollectionBase::prettyName() const
{
    return m_device.friendlyName();
}

bool UpnpCollectionBase::possiblyContainsTrack( const QUrl &url ) const
{
    if( url.scheme() == "upnp-ms" )
//         && url.host() == m_device.host()
//         && url.port() == m_device.port() )
        return true;
    return false;
}

void UpnpCollectionBase::addJob( KIO::SimpleJob *job )
{
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotRemoveJob(KJob*)) );
    m_jobSet.insert( job );
    KIO::Scheduler::assignJobToSlave( m_slave, job );
}

void UpnpCollectionBase::slotRemoveJob(KJob* job)
{
    KIO::SimpleJob *sj = static_cast<KIO::SimpleJob*>( job );

    m_jobSet.remove( sj );

    if( sj->error() ) {
        m_continuousJobFailureCount++;
        if( m_continuousJobFailureCount >= MAX_JOB_FAILURES_BEFORE_ABORT ) {
            debug() << prettyName() << "Had" << m_continuousJobFailureCount << "continuous job failures, something wrong with the device. Removing this collection.";
            emit remove();
        }
    }
    else {
        m_continuousJobFailureCount = 0;
    }
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
