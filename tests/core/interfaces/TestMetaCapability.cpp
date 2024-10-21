/****************************************************************************************
 * Copyright (c) 2012 Jasneet Singh Bhatti <jazneetbhatti@gmail.com>                    *
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

#include "TestMetaCapability.h"

#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "core/capabilities/CollectionScanCapability.h"
#include "core/capabilities/MultiSourceCapability.h"
#include "core/interfaces/MetaCapability.h"

#include <QString>

/**
 * Ad-hoc mock to test MetaCapability
 */
class MetaCapabilityMock : public MetaCapability
{
    public:
        static Capabilities::ActionsCapability *actionsCapability;
        static Capabilities::BookmarkThisCapability *bookmarkThisCapability;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
        {
            switch( type )
            {
            case Capabilities::Capability::Actions:
            case Capabilities::Capability::BookmarkThis:
                return true;
            default:
                break;
            }
            return false;
        }

        Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type ) override
        {
            switch( type )
            {
            case Capabilities::Capability::Actions:
                return actionsCapability;
            case Capabilities::Capability::BookmarkThis:
                return bookmarkThisCapability;
            default:
                break;
            }
            return nullptr;
        }

};

static QAction *action;
static QList<QAction*> actionsList;

// Create the static instances to be returned for testing
Capabilities::ActionsCapability *MetaCapabilityMock::actionsCapability = new Capabilities::ActionsCapability( actionsList );
Capabilities::BookmarkThisCapability *MetaCapabilityMock::bookmarkThisCapability;

QTEST_MAIN( TestMetaCapability )

TestMetaCapability::TestMetaCapability()
{
     action = new QAction( nullptr );
     MetaCapabilityMock::bookmarkThisCapability = new Capabilities::BookmarkThisCapability( action );
}

void
TestMetaCapability::testHas()
{
    MetaCapability *metaCapability = new MetaCapabilityMock();
    QVERIFY( metaCapability );

    // these capabilities should be provided
    QVERIFY( metaCapability->has<Capabilities::ActionsCapability>() == true );
    QVERIFY( metaCapability->has<Capabilities::BookmarkThisCapability>() == true );

    // these should not
    QVERIFY( metaCapability->has<Capabilities::CollectionScanCapability>() == false );
    QVERIFY( metaCapability->has<Capabilities::MultiSourceCapability>() == false );
}

void
TestMetaCapability::testCreate()
{
    MetaCapability *metaCapability = new MetaCapabilityMock();
    QVERIFY( metaCapability );

    // these capabilities should be provided
    // check that the correct instances are returned
    QVERIFY( metaCapability->create<Capabilities::ActionsCapability>() == MetaCapabilityMock::actionsCapability );
    QVERIFY( metaCapability->create<Capabilities::BookmarkThisCapability>() == MetaCapabilityMock::bookmarkThisCapability );

    // these should not
    QVERIFY( metaCapability->create<Capabilities::CollectionScanCapability>() == nullptr );
    QVERIFY( metaCapability->create<Capabilities::MultiSourceCapability>() == nullptr );
}
