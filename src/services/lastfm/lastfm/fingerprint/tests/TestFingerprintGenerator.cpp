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

#include "lib/fingerprint/FingerprintGenerator.h"
#include "lib/types/Track.h"

class TestFingerprintGenerator : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase();
    
    void testValidMp3();
    void testInvalidMp3();
    void testShortMp3();
    void testInvalidMp3Location();
};


void
TestFingerprintGenerator::initTestCase()
{
    qRegisterMetaType<FingerprintGenerator::Error>( "FingerprintGenerator::Error" );
}


void
TestFingerprintGenerator::testValidMp3()
{
    FingerprintGenerator* fpGen = new FingerprintGenerator( QFileInfo( "../lib/fingerprint/tests/data/05 - You Lot.mp3"), FingerprintGenerator::Query, this );
    QSignalSpy spy( fpGen, SIGNAL(success()) );

    fpGen->wait();
    
    QVERIFY2( spy.count() == 1, "No success SIGNAL was emitted by the FingerprintGenerator" );
    
    QVERIFY2( fpGen->data().length() > 0, "Fingerprint data is empty" );
}


void
TestFingerprintGenerator::testInvalidMp3()
{
    FingerprintGenerator* fpGen = new FingerprintGenerator( QFileInfo( "../lib/fingerprint/tests/data/urandom.mp3"), FingerprintGenerator::Query, this );
    QSignalSpy spy( fpGen, SIGNAL(failed( FingerprintGenerator::Error)) );
    
    fpGen->wait();

    QVERIFY2( spy.count() == 1, "No failed SIGNAL was emitted by the FingerprintGenerator" );
    QVERIFY2( fpGen->data().length() == 0, "Fingerprint data is not empty" );
}


void
TestFingerprintGenerator::testShortMp3()
{

    FingerprintGenerator* fpGen = new FingerprintGenerator( QFileInfo( "../lib/fingerprint/tests/data/Anal Cunt - Beating Up Hippies For Their Drugs At A Phish Concert.mp3"), FingerprintGenerator::Query, this );
    QSignalSpy spy( fpGen, SIGNAL(failed( FingerprintGenerator::Error)) );

    fpGen->wait();

    QVERIFY2( spy.count() == 1, "No failed SIGNAL was emitted by the FingerprintGenerator" );
    QVERIFY2( fpGen->data().length() == 0, "Fingerprint data is not empty" );
}


void
TestFingerprintGenerator::testInvalidMp3Location()
{
    FingerprintGenerator* fpGen = new FingerprintGenerator( QFileInfo("This track does not exist.mp3"), FingerprintGenerator::Query, this );
    QSignalSpy spy( fpGen, SIGNAL(failed( FingerprintGenerator::Error)) );

    fpGen->wait();

    QVERIFY2( spy.count() == 1, "No failed SIGNAL was emitted by the FingerprintGenerator" );
    QVERIFY2( fpGen->data().length() == 0, "Fingerprint data is not empty" );
}


QTEST_MAIN( TestFingerprintGenerator )
#include "TestFingerprintGenerator.moc"