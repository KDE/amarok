/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef TESTONEWAYSYNCHRONIZATIONJOB_H
#define TESTONEWAYSYNCHRONIZATIONJOB_H

#include <QtTest/QTest>

class TestOneWaySynchronizationJob : public QObject
{
    Q_OBJECT
public:
    TestOneWaySynchronizationJob();

private slots:
    void init();

    void testEmptyTarget();
    void testEmptySourceWithNonEmptyTarget();
    void testAddTrackToTarget();
    void testAddAlbumToTarget();
    void testAddArtistToTarget();
    void testNoActionNecessary();
};

#endif
