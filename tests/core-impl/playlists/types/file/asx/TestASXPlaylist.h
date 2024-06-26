/***************************************************************************
 *   Copyright (c) 2013 Tatjana Gornak <tgornak@gmail.com>                 *
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

#ifndef TESTASXPLAYLIST_H
#define TESTASXPLAYLIST_H

#include <QObject>
#include <QString>

class QTemporaryDir;

namespace Playlists { class ASXPlaylist; }

class TestASXPlaylist : public QObject
{
Q_OBJECT

public:
    TestASXPlaylist();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testSetAndGetName();
    void testTracks();
    void testUidUrl();
    void testSetAndGetGroups();
    void testIsWritable();
    void testSave();
    void testSaveAndReload();

private:
    Playlists::ASXPlaylist *m_testPlaylist;
    QString dataPath( const QString &relPath = QString() );
    QTemporaryDir *m_tempDir;
};

#endif // TESTASXPLAYLIST_H
