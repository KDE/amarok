/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TESTCUEFILESUPPORT_H
#define TESTCUEFILESUPPORT_H

#include <QtCore/QObject>
#include <QtCore/QString>

class TestCueFileSupport : public QObject
{
Q_OBJECT

public:
    TestCueFileSupport();
  
private slots:
    void testLocateCueFile();
    void testIso88591Cue();
    void testUtf8Cue();

private:
    QString dataPath( const QString &relPath );
};

#endif // TESTCUEFILESUPPORT_H
