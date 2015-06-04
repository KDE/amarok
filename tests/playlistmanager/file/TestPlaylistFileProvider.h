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

#ifndef TESTPLAYLISTFILEPROVIDER_H
#define TESTPLAYLISTFILEPROVIDER_H

#include <QObject>
#include <QString>

namespace Playlists {
    class PlaylistFileProvider;
}

class TestPlaylistFileProvider : public QObject
{
Q_OBJECT

public:
    TestPlaylistFileProvider();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testPlaylists();
    void testSave_data();
    void testSave();
    void testImportAndDeletePlaylists();
    void testRename();

private:
    void removeTestPlaylist();
    void removeConfigPlaylistEntries();
    Playlists::PlaylistFileProvider *m_testPlaylistFileProvider;
    QString dataPath( const QString &relPath );
};

#endif // TESTPLAYLISTFILEPROVIDER_H
