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

#ifndef TESTSQLUSERPLAYLISTPROVIDER_H
#define TESTSQLUSERPLAYLISTPROVIDER_H

#include "playlistmanager/sql/SqlUserPlaylistProvider.h"

#include <QtCore/QStringList>

class TestSqlUserPlaylistProvider : public QObject
{
Q_OBJECT

public:
    TestSqlUserPlaylistProvider( QStringList testArgumentList );

private slots:
    void testPlaylists();
    void testSave();
    void testImportAndDeletePlaylists();
    void testRename();

private:
    SqlUserPlaylistProvider m_testSqlUserPlaylistProvider;
};

#endif // TESTSQLUSERPLAYLISTPROVIDER_H
