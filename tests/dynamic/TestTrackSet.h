/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef TEST_TRACKSET_H
#define TEST_TRACKSET_H

#include "dynamic/TrackSet.h"

#include <QTest>

class TestTrackSet : public QObject
{
    Q_OBJECT
public:
    TestTrackSet();

private Q_SLOTS:
    void init();
    void cleanup();

    void testOutstanding();

    void testEmptyFull();

    void testUnite();
    void testIntersect();
    void testSubtract();

    void testEqual();

private:
    Dynamic::TrackCollectionPtr m_trackCollection;
};

#endif
