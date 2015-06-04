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

#ifndef TESTMETACAPABILITY_H
#define TESTMETACAPABILITY_H

#include <QtTest>

class TestMetaCapability : public QObject
{
    Q_OBJECT

    public:
        TestMetaCapability();

    private Q_SLOTS:
        /**
         * Test whether has() properly delegates work to hasCapabilityInterface()
         */
        void testHas();

        /**
         * Test whether create() properly delegates work to createCapabilityInterface()
         */
        void testCreate();

        /* hasCapabilityInterface() and createCapabilityInterface() are both meant to be
         * overridden in the subclasses, so no need to test them here. */
};

#endif // TESTMETACAPABILITY_H
