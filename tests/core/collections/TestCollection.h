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

#ifndef TESTCOLLECTION_H
#define TESTCOLLECTION_H

#include <QtTest>

namespace Collections
{
    class Collection;
    class TrackProvider;

class TestCollection : public QObject
{
    Q_OBJECT

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();

        // TrackProvider
        void testTrackForUrl();

        //Collection
        void testLocation();

        /**
         * Contains different return values for isWritable() to be tested with
         */
        void testIsWritable_data();
        void testIsWritable();

        /**
         * Contains different return values for isWritable() to be tested with
         */
        void testIsOrganizable_data();
        void testIsOrganizable();

    private:
        Collection *m_collection1, *m_collection2;
        TrackProvider *m_trackProvider;
};

} //namespace Collections

#endif // TESTCOLLECTION_H
