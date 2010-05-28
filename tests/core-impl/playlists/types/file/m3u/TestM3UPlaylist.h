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

#ifndef TESTM3UPLAYLIST_H
#define TESTM3UPLAYLIST_H

#include <QtCore/QObject>
#include <QtCore/QString>

namespace Playlists { class M3UPlaylist; }

class TestM3UPlaylist : public QObject
{
Q_OBJECT

public:
    TestM3UPlaylist();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testSetAndGetName();
    void testTracks();
    void testUidUrl();
    void testSetAndGetGroups();
    void testIsWritable();
    void testSave();

private:
    Playlists::M3UPlaylist *m_testPlaylist;
    QString dataPath( const QString &relPath = QString() );
};

#endif // TESTM3UPLAYLIST_H
