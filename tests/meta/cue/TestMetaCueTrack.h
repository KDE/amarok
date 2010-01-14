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
#include "TestBase.h"

#include <QtCore/QStringList>

class QString;

class TestMetaCueTrack : public TestBase
{
Q_OBJECT

public:
    TestMetaCueTrack( const QStringList args, const QString &logPath );

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testCueItems();
    void testLocateCueSheet();
    void testValidateCueSheet();

private:
    QString *m_isoCuePath, *m_utfCuePath, *m_testSongPath;
};

#endif // TESTMETACUETRACK_H
