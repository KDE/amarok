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

#include "TestActionsCapability.h"

#include <core/capabilities/ActionsCapability.h>


QTEST_MAIN( TestActionsCapability )

TestActionsCapability::TestActionsCapability()
{
}

void
TestActionsCapability::testActions()
{
    QAction *action1 = new QAction( this );
    QAction *action2 = new QAction( this );
    QAction *action3 = new QAction( this );
    QAction *action4 = new QAction( this );
    QAction *action5 = new QAction( this );

    QList<QAction *> actions;
    actions<<action1<<action2<<action3<<action4<<action5;

    Capabilities::ActionsCapability *actions_capability = new Capabilities::ActionsCapability( actions );
    QVERIFY( actions_capability );

    QVERIFY( actions_capability->actions() == actions );
}

void
TestActionsCapability::testCapabilityInterfaceType()
{
    QList<QAction *> actions;

    Capabilities::ActionsCapability *actions_capability = new Capabilities::ActionsCapability( actions );
    QVERIFY( actions_capability );

    QVERIFY( actions_capability->capabilityInterfaceType() == Capabilities::Capability::Actions );
}

