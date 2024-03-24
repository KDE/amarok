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

#ifndef TESTMETATRACKKEY_H
#define TESTMETATRACKKEY_H

#include <QtTest>

class TestMetaTrackKey : public QObject
{
    Q_OBJECT
    public:
        TestMetaTrackKey();

    private Q_SLOTS:
        /**
         * Test constructor TrackKey( const TrackPtr &track )
         */
        void testTrackKey();
        void testOperatorAssignment();

    private:
        /**
         * For portability in fetching tracks from disk
         */
        QString dataPath( const QString &relPath );

};

#endif // TESTMETATRACKKEY_H
