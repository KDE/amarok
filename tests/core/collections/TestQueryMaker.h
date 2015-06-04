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

#ifndef TESTQUERYMAKER_H
#define TESTQUERYMAKER_H

#include <QtTest>

class MockQueryMaker;

class TestQueryMaker : public QObject
{
    Q_OBJECT

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();

        // Data driven testing of setAutoDelete
        void testSetAutoDelete_data();
        void testSetAutoDelete();

    private:
        MockQueryMaker *m_mockQueryMaker;
};

#endif // TESTQUERYMAKER_H
