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

#ifndef TESTXSPFPLAYLIST_H
#define TESTXSPFPLAYLIST_H

#include <QtCore/QObject>
#include <QtCore/QString>

namespace Playlists {
    class XSPFPlaylist;
}

class TestXSPFPlaylist : public QObject
{
Q_OBJECT

public:
    TestXSPFPlaylist();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testSetAndGetName();
    void prettyName();
    void testSetAndGetTracks();
    void testSetAndGetTitle();
    void testSetAndGetCreator();
    void testSetAndGetAnnotation();
    void testSetAndGetInfo();
    void testSetAndGetLocation();
    void testSetAndGetIdentifier();
    void testSetAndGetImage();
    void testSetAndGetDate();
    void testSetAndGetLicense();
    void testSetAndGetAttribution();
    void testSetAndGetLink();
    void testHasCapabilityInterface();
    void testRetrievableUrl();
    void testIsWritable();
    void testSave();

private:
    Playlists::XSPFPlaylist *m_testPlaylist1;
    QString dataPath( const QString &relPath );
};

#endif // TESTXSPFPLAYLIST_H
