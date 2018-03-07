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

#ifndef TESTACTIONSCAPABILITY_H
#define TESTACTIONSCAPABILITY_H

#include "amarok_export.h"
#include "core/capabilities/Capability.h"

#include <QAction>
#include <QList>
#include <QTemporaryDir>
#include <QTest>


class TestActionsCapability : public QObject
{
    Q_OBJECT

public:
    TestActionsCapability();

private Q_SLOTS:
    void testActions();
    void testCapabilityInterfaceType();

private:
    QList< QAction* > m_actions;
};

#endif // TESTACTIONSCAPABILITY_H
