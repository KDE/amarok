/***************************************************************************
 *   Copyright (c) 2024 Tuomas Nurmi <tuomas@norsumanageri.org>            *
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

#include "TestPlaylistLayout.h"
#include "config-amarok-test.h"
#include "playlist/layouts/LayoutManager.h"
#include "support/Amarok.h"

#include <QDir>
#include <QStandardPaths>
#include <QTest>


QTEST_GUILESS_MAIN( TestPlaylistLayout )

using namespace Playlist;

TestPlaylistLayout::TestPlaylistLayout()
{}

void TestPlaylistLayout::initTestCase()
{
    const QDir dir( QStandardPaths::locate( QStandardPaths::GenericDataLocation,
                                            QStringLiteral("amarok/data"),
                                            QStandardPaths::LocateDirectory ) );
    QVERIFY( dir.exists() );
    m_testPlaylistLayoutPath = dir.absolutePath();
}

void TestPlaylistLayout::testLoadingDefault()
{
    QCOMPARE( ( LayoutManager::instance()->layouts().length() > 3 ), true );
    QCOMPARE( LayoutManager::instance()->layout("Default").layoutForPart( PlaylistLayout::Head ).rows(), 2 );
    QCOMPARE( LayoutManager::instance()->layout("Default").layoutForPart( PlaylistLayout::Single ).rows(), 2 );
    QCOMPARE( LayoutManager::instance()->layout("Default").layoutForPart( PlaylistLayout::StandardBody ).rows(), 1 );
    QCOMPARE( LayoutManager::instance()->layout("Default").layoutForPart( PlaylistLayout::VariousArtistsBody ).rows(), 1 );
}

void TestPlaylistLayout::testCreatingLayout()
{
    PlaylistLayout ul;
    LayoutItemConfig c;
    LayoutItemConfigRow r;
    r.addElement( LayoutItemConfigRowElement( 1, 0.5, true, true, true, Qt::AlignLeft, "prefix", "suffix" ) );
    c.addRow(r);
    ul.setLayoutForPart( PlaylistLayout::Head, c );
    
    c.addRow(r);
    ul.setLayoutForPart( PlaylistLayout::Single, c );
    
    int len = LayoutManager::instance()->layouts().length();
    LayoutManager::instance()->addUserLayout( "_autotest layout", ul );
    QCOMPARE( ( LayoutManager::instance()->layouts().length() ), len + 1 );
    
    
    QCOMPARE( LayoutManager::instance()->layout("_autotest layout").layoutForPart( PlaylistLayout::Head ).rows(), 1 );
    QCOMPARE( LayoutManager::instance()->layout("_autotest layout").layoutForPart( PlaylistLayout::Single ).rows(), 2 );
    QCOMPARE( LayoutManager::instance()->layout("_autotest layout").layoutForPart( PlaylistLayout::StandardBody ).rows(), 0 );
    QCOMPARE( LayoutManager::instance()->layout("_autotest layout").layoutForPart( PlaylistLayout::VariousArtistsBody ).rows(), 0 );
}

void TestPlaylistLayout::testOrigins()
{
    QCOMPARE( LayoutManager::instance()->isDefaultLayout("Default"), true);
    QCOMPARE( LayoutManager::instance()->isDefaultLayout("Verbose"), true);
    QCOMPARE( LayoutManager::instance()->isDefaultLayout("_autotest layout"), false);
}

void TestPlaylistLayout::cleanupTestCase()
{
    // LayoutManager::deleteLayout opens a KMessageBox on failure, so it's maybe not good for tests
    QString layoutPath = QDir::toNativeSeparators( Amarok::saveLocation( QStringLiteral("playlist_layouts/") ) + QString( "_autotest layout.xml" ) );
    QCOMPARE( QFile::exists( layoutPath ), true );
    QCOMPARE( QFile::remove( layoutPath ), true);
    QCOMPARE( QFile::exists( layoutPath ), false );
}
