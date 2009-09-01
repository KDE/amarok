/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef TESTMETACUETRACK_H
#define TESTMETACUETRACK_H

#include "meta/cue/Cue.h"

#include <QtCore/QStringList>

class QString;

class TestMetaCueTrack : public QObject
{
Q_OBJECT

public:
    TestMetaCueTrack( QStringList testArgumentList, bool stdout );

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testCueItems();
    void testLocateCueSheet();
    void testValidateCueSheet();
    //methods inherited from Meta::MetaBase
    void testName();
    void testPrettyName();
    void testFullPrettyName();
    void testSortableName();

    void testTrackNumber();
    void testLength();

    void testAlbum();
    void testArtist();

    void testHasCapabilityInterface();

private:
    QString *m_isoCuePath, *m_utfCuePath, *m_testSongPath;
    MetaCue::Track *m_testTrack1, *m_testTrack2;
};

#endif // TESTMETACUETRACK_H
