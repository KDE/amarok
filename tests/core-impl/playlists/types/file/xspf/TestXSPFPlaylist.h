/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
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

#include <QObject>
#include <QString>

class QTemporaryDir;

namespace Playlists {
    class XSPFPlaylist;
}

class TestXSPFPlaylist : public QObject
{
Q_OBJECT

public:
    TestXSPFPlaylist();

private Q_SLOTS:
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
    void testUidUrl();
    void testIsWritable();
    void testSave();
    void testSaveAndReload();

private:
    Playlists::XSPFPlaylist *m_testPlaylist1;
    QTemporaryDir *m_tempDir;
    QString dataPath( const QString &relPath );
};

#endif // TESTXSPFPLAYLIST_H
