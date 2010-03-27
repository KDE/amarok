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

#define DEBUG_PREFIX "MtpCollection"

#include "MtpCollection.h"
#include "MtpConnectionAssistant.h"
#include "MtpDeviceInfo.h"
#include "MediaDeviceInfo.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"

#include <KUrl>

AMAROK_EXPORT_COLLECTION( MtpCollectionFactory, mtpcollection )

MtpCollectionFactory::MtpCollectionFactory( QObject *parent, const QVariantList &args )
    : MediaDeviceCollectionFactory<MtpCollection> ( new MtpConnectionAssistant() )
{
    setParent( parent );
    Q_UNUSED( args )
}

MtpCollectionFactory::~MtpCollectionFactory()
{
    DEBUG_BLOCK
    // nothing to do
}

//MtpCollection

MtpCollection::MtpCollection( MediaDeviceInfo* info )
: MediaDeviceCollection()
{
    DEBUG_BLOCK
    /** Fetch Info needed to construct MtpCollection */
    debug() << "Getting mtp info";
    MtpDeviceInfo *mtpinfo = qobject_cast<MtpDeviceInfo *>( info );

    debug() << "Getting udi";
    m_udi = mtpinfo->udi();

    debug() << "constructing handler";

    m_handler = new Meta::MtpHandler( this );

    //startFullScan();// parse tracks
}

MtpCollection::~MtpCollection()
{
    DEBUG_BLOCK
    //if( m_handler )
    //    qobject_cast<Meta::MtpHandler*> ( m_handler )->terminate();
}

QString
MtpCollection::collectionId() const
{
     return m_udi;
}

QString
MtpCollection::prettyName() const
{
    return m_handler->prettyName();
}



#include "MtpCollection.moc"

