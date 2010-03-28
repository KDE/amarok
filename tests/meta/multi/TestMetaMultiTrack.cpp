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

#include "TestMetaMultiTrack.h"

#include "core/support/Components.h"
#include "EngineController.h"
#include "config-amarok-test.h"
#include "core/meta/impl/multi/MultiTrack.h"
#include "core-implementations/playlists/file/PlaylistFileSupport.h"

#include <QtTest/QTest>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestMetaMultiTrack, GUI )

TestMetaMultiTrack::TestMetaMultiTrack()
    : m_testMultiTrack( 0 )
{}

void TestMetaMultiTrack::initTestCase()
{
    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );
  
    const QString path = QString( AMAROK_TEST_DIR ) + "/data/playlists/test.pls";
    const QFileInfo file( QDir::toNativeSeparators( path ) );
    QVERIFY( file.exists() );
    const QString filePath = file.absoluteFilePath();
    Playlists::PlaylistPtr playlist = Playlists::PlaylistPtr::dynamicCast( Playlists::loadPlaylistFile( filePath ) );

    if( !playlist )
        QVERIFY( false ); // no playlist -> no test. that's life ;)

    m_testMultiTrack = new Meta::MultiTrack( playlist );
}

void TestMetaMultiTrack::cleanupTestCase()
{
    delete m_testMultiTrack;
}


void TestMetaMultiTrack::testFirst()
{
    QCOMPARE( m_testMultiTrack->first().pathOrUrl(), QString( "http://85.214.44.27:8000" ) );
    m_testMultiTrack->next();
    m_testMultiTrack->setSource( 2 );
    QCOMPARE( m_testMultiTrack->first().pathOrUrl(), QString( "http://85.214.44.27:8000" ) );
}

void TestMetaMultiTrack::testNext()
{
    QCOMPARE( m_testMultiTrack->first().pathOrUrl(), QString( "http://85.214.44.27:8000" ) );
    QCOMPARE( m_testMultiTrack->next().pathOrUrl(), QString( "http://217.20.121.40:8000" ) );
    QCOMPARE( m_testMultiTrack->next().pathOrUrl(), QString( "http://85.214.44.27:8100" ) );
    QCOMPARE( m_testMultiTrack->next().pathOrUrl(), QString( "http://85.214.44.27:8200" ) );
    QCOMPARE( m_testMultiTrack->next().pathOrUrl(), QString( "" ) );
    QCOMPARE( m_testMultiTrack->next().pathOrUrl(), QString( "" ) );
}

void TestMetaMultiTrack::testCurrentAndSetSource()
{
    m_testMultiTrack->first();
    QCOMPARE( m_testMultiTrack->current(), 0 );

    m_testMultiTrack->setSource( 1 );
    QCOMPARE( m_testMultiTrack->current(), 1 );

    m_testMultiTrack->setSource( 2 );
    QCOMPARE( m_testMultiTrack->current(), 2 );

    m_testMultiTrack->setSource( 3 );
    QCOMPARE( m_testMultiTrack->current(), 3 );

    m_testMultiTrack->setSource( 4 );
    QCOMPARE( m_testMultiTrack->current(), 3 );
}

void TestMetaMultiTrack::testSources()
{
    QStringList sources = m_testMultiTrack->sources();
    QCOMPARE( sources.size(), 4 );
    QCOMPARE( sources.at( 0 ), QString( "http://85.214.44.27:8000" ) );
    QCOMPARE( sources.at( 1 ), QString( "http://217.20.121.40:8000" ) );
    QCOMPARE( sources.at( 2 ), QString( "http://85.214.44.27:8100" ) );
    QCOMPARE( sources.at( 3 ), QString( "http://85.214.44.27:8200" ) );
}

void TestMetaMultiTrack::testHasCapabilityInterface()
{
    QVERIFY( m_testMultiTrack->hasCapabilityInterface( Capabilities::Capability::MultiSource ) );
}
