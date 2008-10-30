/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include <QtTest/QtTest>
#include <QUrl>

#include "mocks/Collection.h"
#include "lib/fingerprint/FingerprintIdRequest.h"
#include "lib/types/Track.h"

class UnitTestsFingerprintIdRequest : public QObject
{
    Q_OBJECT
    
private slots:
    void testValidMp3();

};

void
UnitTestsFingerprintIdRequest::testValidMp3()
{
    MutableTrack t;
    t.setUrl( QUrl::fromLocalFile( "../lib/fingerprint/tests/data/05 - You Lot.mp3" ) );
    t.setAlbum( "Blue Album" );
    t.setArtist( "Orbital" );
    t.setTitle( "You Lot" );      
    t.setDuration( 427 );
    t.setTrackNumber( 5 );
    
    FingerprintIdRequest f( t );
    QSignalSpy spy( &f, SIGNAL(FpIDFound( QString )) );
    
    QTest::qWait( 5000 );
    
    QVERIFY2( spy.count() == 1, "Did not receive FpIdFound signal" );
    QVERIFY( spy.takeFirst().takeFirst().toString() != "0" );
}


// 
// void
// TestFingerprintGenerator::testInvalidMp3()
// {
//     MutableTrack t;
//     t.setUrl( QUrl::fromLocalFile( "../lib/fingerprint/tests/data/urandom.mp3" ) );
//     t.setAlbum( "uRandom's Greatest Hits" );
//     t.setArtist( "The artist formally known as urandom" );
//     t.setTitle( "skadjhf98sdfs" );      
//     t.setDuration( 427 );
//     t.setTrackNumber( 1 );
// 
//     FingerprintIdRequest f( t );
//     QSignalSpy spy( &f, SIGNAL(failed( QString)) );
// 
//     QTest::qWait( 5000 );
// 
//     QVERIFY2( spy.count() == 1, "No failed SIGNAL was emitted by the FingerprintGenerator" );
//     QVERIFY2( f, "Fingerprint data is not empty" );
// }
// 
// 
// void
// TestFingerprintGenerator::testShortMp3()
// {
//     MutableTrack t;
//     t.setUrl( QUrl::fromLocalFile( "../lib/fingerprint/tests/data/Anal Cunt - Beating Up Hippies For Their Drugs At A Phish Concert.mp3" ) );
//     t.setAlbum( "Defenders of the Hate" );
//     t.setArtist( "Anal Cunt" );
//     t.setTitle( "Beating Up Hippies For Their Drugs At A Phish Concert" );      
//     t.setDuration( 28 );
//     t.setTrackNumber( 14 );
// 
//     FingerprintGenerator fpGen;
//     QSignalSpy spy( &fpGen, SIGNAL(failed( QString)) );
//     fpGen.fingerprint( t, FingerprintGenerator::Query );
// 
//     fpGen.wait();
// 
//     QVERIFY2( spy.count() == 1, "No failed SIGNAL was emitted by the FingerprintGenerator" );
//     QVERIFY2( fpGen.data().length() == 0, "Fingerprint data is not empty" );
// }
// 
// 
// void
// TestFingerprintGenerator::testInvalidMp3Location()
// {
//     MutableTrack t;
//     t.setUrl( QUrl::fromLocalFile( "This track does not exist.mp3" ) );
//     t.setAlbum( "Unknown" );
//     t.setArtist( "Unknown" );
//     t.setTitle( "Unknown" );      
//     t.setDuration( 123 );
//     t.setTrackNumber( 4 );
// 
//     FingerprintGenerator fpGen;
//     QSignalSpy spy( &fpGen, SIGNAL(failed( QString)) );
//     fpGen.fingerprint( t, FingerprintGenerator::Query );
// 
//     fpGen.wait();
// 
//     QVERIFY2( spy.count() == 1, "No failed SIGNAL was emitted by the FingerprintGenerator" );
//     QVERIFY2( fpGen.data().length() == 0, "Fingerprint data is not empty" );
// }


QTEST_MAIN( UnitTestsFingerprintIdRequest )
#include "TestFingerprintIdRequest.moc"
