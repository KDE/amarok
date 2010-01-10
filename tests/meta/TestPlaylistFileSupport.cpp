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

#include "TestPlaylistFileSupport.h"
#include "meta/PlaylistFileSupport.h"

#include <KStandardDirs>

#include <QtTest/QTest>
#include <QtCore/QDir>


TestPlaylistFileSupport::TestPlaylistFileSupport( const QStringList args, const QString &logPath )
    : TestBase( "PlaylistFileSupport" )
{
    QStringList combinedArgs = args;
    addLogging( combinedArgs, logPath );
    QTest::qExec( this, combinedArgs );
}


void TestPlaylistFileSupport::initTestCase()
{
    m_testPlaylistPath = KStandardDirs::installPath( "data" ) + QDir::toNativeSeparators(  "amarok/testdata/playlists/" );
}

void TestPlaylistFileSupport::testGetFormat()
{
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "test.asx" ), Meta::ASX );
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "test.m3u" ), Meta::M3U );
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "test.pls" ), Meta::PLS );
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "test.ram" ), Meta::RAM );
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "test.smil" ), Meta::SMIL );
    // TODO: Meta::XML <- what kind of playlist format is that? example?
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "test.xspf" ), Meta::XSPF );
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "no-playlist.png" ), Meta::Unknown );
    QCOMPARE( Meta::getFormat( m_testPlaylistPath + "no-playlist.png" ), Meta::NotPlaylist );
}

void TestPlaylistFileSupport::testIsPlaylist()
{
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "test.asx" ), true );
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "test.m3u" ), true );
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "test.pls" ), true );
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "test.ram" ), true );
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "test.smil" ), true );
    // TODO: Meta::XML <- what kind of playlist format is that? example?
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "test.xspf" ), true );
    QCOMPARE( Meta::isPlaylist( m_testPlaylistPath + "no-playlist.png" ), false );
}

#include "TestPlaylistFileSupport.moc"
