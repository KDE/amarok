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

#define DEBUG_PREFIX "UpnpCollection"

#include "UpnpCollection.h"

#include "core/support/Debug.h"
#include "MemoryQueryMaker.h"
#include "statusbar/StatusBar.h"

#include <QStringList>
#include <QTimer>

#include <KLocale>

using namespace Collections;

AMAROK_EXPORT_COLLECTION( UpnpCollectionFactory, upnpcollection )

UpnpCollectionFactory::UpnpCollectionFactory( QObject *parent, const QVariantList &args )
    : Collections::CollectionFactory()
{
    setParent( parent );
    Q_UNUSED( args );
}

UpnpCollectionFactory::~UpnpCollectionFactory()
{
}

void UpnpCollectionFactory::init()
{
    DEBUG_BLOCK
    emit newCollection( new UpnpCollection );
}

//UpnpCollection

UpnpCollection::UpnpCollection()
    : Collection()
{
    DEBUG_BLOCK
}

UpnpCollection::~UpnpCollection()
{
}

void
UpnpCollection::startFullScan()
{
    DEBUG_BLOCK
    //ignore
}

QueryMaker*
UpnpCollection::queryMaker()
{
    return new MemoryQueryMaker( m_mc.toWeakRef(), collectionId() );
}

QString
UpnpCollection::collectionId() const
{
    return QString("upnp://") + "bazinga";
}

QString
UpnpCollection::prettyName() const
{
    return i18n("Upnp Bazinga");
}

#include "UpnpCollection.moc"

