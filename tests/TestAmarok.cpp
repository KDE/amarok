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

#include "TestAmarok.h"
#include "Amarok.h"

#include <KStandardDirs>

#include <QtTest/QTest>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDateTime>

TestAmarok::TestAmarok( const QStringList args, const QString &logPath )
{
    QStringList combinedArgs = args;
    if( !logPath.isEmpty() )
        combinedArgs << QString( "-o" ) << QString( logPath + "Amarok.xml" );
    QTest::qExec( this, combinedArgs );
}

void TestAmarok::testAsciiPath()
{
    QCOMPARE( Amarok::asciiPath( "" ), QString( "" ) );
    QCOMPARE( Amarok::asciiPath( "/home/sven" ), QString( "/home/sven" ) );

    QCOMPARE( Amarok::asciiPath( "/äöü" ), QString( "/___" ) );
    QCOMPARE( Amarok::asciiPath( "/äöütest" ), QString( "/___test" ) );
    QCOMPARE( Amarok::asciiPath( "/äöü/test" ), QString( "/___/test" ) );
    QCOMPARE( Amarok::asciiPath( "/123/" ), QString( "/123/" ) );
    QCOMPARE( Amarok::asciiPath( "/.hidden" ), QString( "/.hidden" ) );
    QCOMPARE( Amarok::asciiPath( "/here be dragons" ), QString( "/here be dragons" ) );
    QCOMPARE( Amarok::asciiPath( "/!important/some%20stuff/what's this?" ), QString( "/!important/some%20stuff/what's this?" ) );

    /* 0x7F = 127 = DEL control character, explicitly ok on *nix file systems */
    QCOMPARE( Amarok::asciiPath( QString( "/abc" ) + QChar( 0x7F ) + ".1" ), QString( QString( "/abc" ) + QChar( 0x7F ) + ".1" ) );

    /* random control character: ok */
    QCOMPARE( Amarok::asciiPath( QString( "/abc" ) + QChar( 0x07 ) + ".1" ), QString( QString( "/abc" ) + QChar( 0x07 ) + ".1" ) );

    /* null byte is not ok */
    QCOMPARE( Amarok::asciiPath( QString( "/abc" ) + QChar( 0x00 ) + ".1" ), QString( "/abc_.1" ) );
}

void TestAmarok::testCleanPath()
{
    /* no changes expected here */
    QCOMPARE( Amarok::cleanPath( QString( "" ) ), QString( "" ) );
    QCOMPARE( Amarok::cleanPath( QString( "abcdefghijklmnopqrstuvwxyz" ) ), QString( "abcdefghijklmnopqrstuvwxyz" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) ), QString( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) );
    QCOMPARE( Amarok::cleanPath( QString( "/\\.,-+" ) ), QString( "/\\.,-+" ) );

    /* German */
    QCOMPARE( Amarok::cleanPath( QString( "äöüß" ) ), QString( "aeoeuess" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÄÖÜß" ) ), QString( "AeOeUess" ) ); // capital ß only exists in theory in the German language, but had been defined some time ago, iirc

    /* French */
    QCOMPARE( Amarok::cleanPath( QString( "áàéèêô" ) ), QString( "aaeeeo" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÁÀÉÈÊÔ" ) ), QString( "AAEEEO" ) );
    QCOMPARE( Amarok::cleanPath( QString( "æ" ) ), QString( "ae" ) );
    QCOMPARE( Amarok::cleanPath( QString( "Æ" ) ), QString( "AE" ) );

    /* Czech and other east European languages */
    QCOMPARE( Amarok::cleanPath( QString( "çńǹýỳź" ) ), QString( "cnnyyz" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÇŃǸÝỲŹ" ) ), QString( "CNNYYZ" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ěĺľôŕřůž" ) ), QString( "ellorruz" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ" ) ), QString( "ACDEEINORSTUUYZ" ) );

    /* Skandinavian languages */
    QCOMPARE( Amarok::cleanPath( QString( "åø" ) ), QString( "aoe" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÅØ" ) ), QString( "AOE" ) );

    /* Spanish */
    QCOMPARE( Amarok::cleanPath( QString( "ñóÿ" ) ), QString( "noy" ) );
    QCOMPARE( Amarok::cleanPath( QString( "ÑÓŸ" ) ), QString( "NOY" ) );

    /* add missing ones here */
}

void TestAmarok::testComputeScore()
{
     QVERIFY( 50 < Amarok::computeScore( 50, 1,  1 ) ); // greater score if played completely
     QVERIFY(  0 < Amarok::computeScore(  0, 1,  1 ) ); // handle 0 score
     QVERIFY( 50 > Amarok::computeScore( 50, 1,  0.1 ) ); // lower score if aborted early
     QVERIFY( 50 > Amarok::computeScore( 50, 1,  0 ) ); // handle 0% played fraction
     QVERIFY( 50 > Amarok::computeScore( 50, 0,  0 ) ); // handle 0 playcount
}

void TestAmarok::testConciseTimeSince()
{
    QCOMPARE( Amarok::conciseTimeSince( 0 ).isEmpty(), false );
    QCOMPARE( Amarok::conciseTimeSince( 10 ).isEmpty(), false );
    QCOMPARE( Amarok::conciseTimeSince( 100 ).isEmpty(), false );
    QCOMPARE( Amarok::conciseTimeSince( 1000 ).isEmpty(), false );
    /* any other good ideas what to test here? */
}

void TestAmarok::testDecapitateString()
{
    QCOMPARE( Amarok::decapitateString( "", "" ), QString( "" ) );

    QCOMPARE( Amarok::decapitateString( "abc123", "abc456" ), QString( "123" ) );
    QCOMPARE( Amarok::decapitateString( "abc", "123" ), QString( "abc" ) );
    QCOMPARE( Amarok::decapitateString( "abc", "" ), QString( "abc" ) );
    QCOMPARE( Amarok::decapitateString( "", "abc" ), QString( "" ) );
}

void TestAmarok::testEscapeHTMLAttr()
{
    QCOMPARE( Amarok::escapeHTMLAttr( "" ), QString( "" ) );

    QCOMPARE( Amarok::escapeHTMLAttr( "test\"fu=bar" ), QString( "test%22fu=bar" ) );
    QCOMPARE( Amarok::escapeHTMLAttr( "test#fu=bar" ), QString( "test%23fu=bar" ) );
    QCOMPARE( Amarok::escapeHTMLAttr( "test%fu=bar" ), QString( "test%25fu=bar" ) );
    QCOMPARE( Amarok::escapeHTMLAttr( "test\'fu=bar" ), QString( "test%27fu=bar" ) );
    QCOMPARE( Amarok::escapeHTMLAttr( "test?fu=bar" ), QString( "test%3Ffu=bar" ) );
}

void TestAmarok::testExtension()
{
    QCOMPARE( Amarok::extension( "" ), QString( "" ) );
    QCOMPARE( Amarok::extension( "..." ), QString( "" ) );
    QCOMPARE( Amarok::extension( "test" ), QString( "" ) );
    QCOMPARE( Amarok::extension( "test." ), QString( "" ) );
    QCOMPARE( Amarok::extension( "test.mp3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "test.mP3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "test.MP3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "test.longextension" ), QString( "longextension" ) );
    QCOMPARE( Amarok::extension( "test.long.extension" ), QString( "extension" ) );
    QCOMPARE( Amarok::extension( "test.m" ), QString( "m" ) );
    QCOMPARE( Amarok::extension( "test.äöü" ), QString( "äöü" ) );
    QCOMPARE( Amarok::extension( "test.ÄÖÜ" ), QString( "äöü" ) );
    QCOMPARE( Amarok::extension( "..test.mp3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "..te st.mp3" ), QString( "mp3" ) );
    QCOMPARE( Amarok::extension( "..te st.m p3" ), QString( "m p3" ) );
}

void TestAmarok::testManipulateThe()
{
    QString teststring;

    Amarok::manipulateThe( teststring = "", true );
    QCOMPARE( teststring, QString( "" ) );

    Amarok::manipulateThe( teststring = "", false );
    QCOMPARE( teststring, QString( "" ) );

    Amarok::manipulateThe( teststring = 'A', true );
    QCOMPARE( teststring, QString( "A" ) );

    Amarok::manipulateThe( teststring = 'A', false );
    QCOMPARE( teststring, QString( "A" ) );

    Amarok::manipulateThe( teststring = "ABC", true );
    QCOMPARE( teststring, QString( "ABC" ) );

    Amarok::manipulateThe( teststring = "ABC", false );
    QCOMPARE( teststring, QString( "ABC" ) );

    Amarok::manipulateThe( teststring = "The Eagles", true );
    QCOMPARE( teststring, QString( "Eagles, The" ) );

    Amarok::manipulateThe( teststring = "Eagles, The", false );
    QCOMPARE( teststring, QString( "The Eagles" ) );

    Amarok::manipulateThe( teststring = "The The", true );
    QCOMPARE( teststring, QString( "The, The" ) );

    Amarok::manipulateThe( teststring = "The, The", false );
    QCOMPARE( teststring, QString( "The The" ) );

    Amarok::manipulateThe( teststring = "Something else", true );
    QCOMPARE( teststring, QString( "Something else" ) );

    Amarok::manipulateThe( teststring = "The Äöü", true );
    QCOMPARE( teststring, QString( "Äöü, The" ) );

    Amarok::manipulateThe( teststring = "Äöü, The", false );
    QCOMPARE( teststring, QString( "The Äöü" ) );
}

void TestAmarok::testRecursiveUrlExpand()
{
    /* There are two overloaded variants of this function */
    KUrl url( "" );
    KUrl::List urlList, resultList;

    resultList = Amarok::recursiveUrlExpand( url );
    QCOMPARE( resultList.isEmpty(), true );

    resultList = Amarok::recursiveUrlExpand( urlList );
    QCOMPARE( resultList.isEmpty(), true );

    urlList.append( url );
    resultList = Amarok::recursiveUrlExpand( urlList );
    QCOMPARE( resultList.isEmpty(), true );

    url = KStandardDirs::locate( "data", QDir::toNativeSeparators( "amarok/testdata/playlists/" ) );
    resultList = Amarok::recursiveUrlExpand( url );
    QCOMPARE( resultList.size(), 1 );
    QCOMPARE( resultList.at( 0 ).pathOrUrl(), url.pathOrUrl() + QString( "no-playlist.png" ) ); // only non-playlist file in that dir

    url = KStandardDirs::locate( "data", QDir::toNativeSeparators( "amarok/testdata/" ) );
    resultList = Amarok::recursiveUrlExpand( url );
    QCOMPARE( resultList.size(), 17 );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "cue/test_silence.ogg" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "cue/testsheet01-iso8859-1.cue" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "cue/testsheet01-utf8.cue" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "playlists/no-playlist.png" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 01.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 02.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 03.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 04.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 05.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 06.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 07.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 08.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 09.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "audio/Platz 10.mp3" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "album/Track01.ogg" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "album/Track02.ogg" ) ) ) );
    QVERIFY( resultList.contains( url.pathOrUrl() + QString( QDir::toNativeSeparators( "album/Track03.ogg" ) ) ) );
}

void TestAmarok::testSaveLocation()
{
    QString saveLocation = Amarok::saveLocation();
    QDir saveLocationDir( saveLocation );

    QCOMPARE( saveLocationDir.exists(), true );
    QCOMPARE( QDir::isAbsolutePath( saveLocation ), true );
    QCOMPARE( saveLocationDir.isReadable(), true );
    /* any other good ideas what to test here? */
}

void TestAmarok::testUnescapeHTMLAttr()
{
    QCOMPARE( Amarok::unescapeHTMLAttr( "" ), QString( "" ) );

    QCOMPARE( Amarok::unescapeHTMLAttr( "test%22fu=bar" ), QString( "test\"fu=bar" ) );
    QCOMPARE( Amarok::unescapeHTMLAttr( "test%23fu=bar" ), QString( "test#fu=bar" ) );
    QCOMPARE( Amarok::unescapeHTMLAttr( "test%25fu=bar" ), QString( "test%fu=bar" ) );
    QCOMPARE( Amarok::unescapeHTMLAttr( "test%27fu=bar" ), QString( "test\'fu=bar" ) );
    QCOMPARE( Amarok::unescapeHTMLAttr( "test%3Ffu=bar" ), QString( "test?fu=bar" ) );
}

void TestAmarok::testVerboseTimeSince()
{
    /* There are two overloaded variants of this function */
    QCOMPARE( Amarok::verboseTimeSince( 0 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 0 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 10 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 10 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 100 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 100 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 1000 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromTime_t( 1000 ) ).isEmpty(), false );
    /* any other good ideas what to test here? */
}

void TestAmarok::testVfatPath()
{
    QCOMPARE( Amarok::vfatPath( "" ), QString( "" ) );

    /* allowed characters */
    QCOMPARE( Amarok::vfatPath( "abcdefghijklmnopqrstuvwxyz" ), QString( "abcdefghijklmnopqrstuvwxyz" ) );
    QCOMPARE( Amarok::vfatPath( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ), QString( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) );
    QCOMPARE( Amarok::vfatPath( "0123456789" ), QString( "0123456789" ) );
    QCOMPARE( Amarok::vfatPath( "! # $ % & ' ( ) - @ ^ _ ` { } ~" ), QString( "! # $ % & ' ( ) - @ ^ _ ` { } ~" ) );

    /* only allowed in long file names */
    QCOMPARE( Amarok::vfatPath( "+,.;=[]" ), QString( "+,.;=[]" ) );

    /* illegal characters, without / and \ (see later tests) */
    QCOMPARE( Amarok::vfatPath( "\"_*_:_<_>_?_|" ), QString( "_____________" ) );

    /* illegal control characters: 0-31, 127 */
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x00 ) + QChar( 0x01 ) + QChar( 0x02 ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x03 ) + QChar( 0x04 ) + QChar( 0x05 ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x06 ) + QChar( 0x07 ) + QChar( 0x08 ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x09 ) + QChar( 0x0A ) + QChar( 0x0B ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x0C ) + QChar( 0x0D ) + QChar( 0x0E ) + ".1" ), QString( "abc___.1" ) );

    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x0F ) + QChar( 0x10 ) + QChar( 0x11 ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x12 ) + QChar( 0x13 ) + QChar( 0x14 ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x15 ) + QChar( 0x16 ) + QChar( 0x17 ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x18 ) + QChar( 0x19 ) + QChar( 0x1A ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x1B ) + QChar( 0x1C ) + QChar( 0x1D ) + ".1" ), QString( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QString( "abc" ) + QChar( 0x1E ) + QChar( 0x7F ) + ".1" ), QString( "abc__.1" ) ); // 0x7F = 127 = DEL control character

    /* trailing spaces in extension, directory and file names are being ignored (!) */
    QCOMPARE( Amarok::vfatPath( "test   " ), QString( "test  _" ) );
    QCOMPARE( Amarok::vfatPath( "   test   " ), QString( "   test  _" ) );
    QCOMPARE( Amarok::vfatPath( "test.ext   " ), QString( "test.ext  _" ) );
    QCOMPARE( Amarok::vfatPath( "test   .ext   " ), QString( "test  _.ext  _" ) );
    QCOMPARE( Amarok::vfatPath( "   test   .ext   " ), QString( "   test  _.ext  _" ) ); // yes, really!

#ifdef Q_WS_WIN // interpret / as part of the name, not directory separator
    QCOMPARE( Amarok::vfatPath( "\\some\\folder   \\" ), QString( "\\some\\folder  _\\" ) );
    QCOMPARE( Amarok::vfatPath( "\\some   \\folder   \\" ), QString( "\\some  _\\folder  _\\" ) );
    QCOMPARE( Amarok::vfatPath( "\\...some   \\ev  il   \\folders...\\" ), QString( "\\...some  _\\ev  il  _\\folders...\\" ) );
    QCOMPARE( Amarok::vfatPath( "\\some\\fol/der   \\" ), QString( "\\some\\fol_der  _\\" ) );
#else // interpret \ as part of the name, not directory separator
    QCOMPARE( Amarok::vfatPath( "/some/folder   /" ), QString( "/some/folder  _/" ) );
    QCOMPARE( Amarok::vfatPath( "/some   /folder   /" ), QString( "/some  _/folder  _/" ) );
    QCOMPARE( Amarok::vfatPath( "/...some   /ev  il   /folders.../" ), QString( "/...some  _/ev  il  _/folders.../" ) );
    QCOMPARE( Amarok::vfatPath( "/some/fol\\der   /" ), QString( "/some/fol_der  _/" ) );
#endif

    /* Stepping deeper into M$ hell: reserved device names
     * See http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx */
    QCOMPARE( Amarok::vfatPath( "CLOCK$" ), QString( "_CLOCK$" ) );
    /* this one IS allowed according to
     * http://en.wikipedia.org/w/index.php?title=Filename&oldid=303934888#Comparison_of_file_name_limitations */
    QCOMPARE( Amarok::vfatPath( "CLOCK$.ext" ), QString( "CLOCK$.ext" ) );

    QCOMPARE( Amarok::vfatPath( "CON" ), QString( "_CON" ) );
    QCOMPARE( Amarok::vfatPath( "CON.ext" ), QString( "_CON.ext" ) );

    QCOMPARE( Amarok::vfatPath( "PRN" ), QString( "_PRN" ) );
    QCOMPARE( Amarok::vfatPath( "PRN.ext" ), QString( "_PRN.ext" ) );

    QCOMPARE( Amarok::vfatPath( "AUX" ), QString( "_AUX" ) );
    QCOMPARE( Amarok::vfatPath( "AUX.ext" ), QString( "_AUX.ext" ) );

    QCOMPARE( Amarok::vfatPath( "NUL" ), QString( "_NUL" ) );
    QCOMPARE( Amarok::vfatPath( "NUL.ext" ), QString( "_NUL.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM1" ), QString( "_COM1" ) );
    QCOMPARE( Amarok::vfatPath( "COM1.ext" ), QString( "_COM1.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM2" ), QString( "_COM2" ) );
    QCOMPARE( Amarok::vfatPath( "COM2.ext" ), QString( "_COM2.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM3" ), QString( "_COM3" ) );
    QCOMPARE( Amarok::vfatPath( "COM3.ext" ), QString( "_COM3.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM4" ), QString( "_COM4" ) );
    QCOMPARE( Amarok::vfatPath( "COM4.ext" ), QString( "_COM4.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM5" ), QString( "_COM5" ) );
    QCOMPARE( Amarok::vfatPath( "COM5.ext" ), QString( "_COM5.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM6" ), QString( "_COM6" ) );
    QCOMPARE( Amarok::vfatPath( "COM6.ext" ), QString( "_COM6.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM7" ), QString( "_COM7" ) );
    QCOMPARE( Amarok::vfatPath( "COM7.ext" ), QString( "_COM7.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM8" ), QString( "_COM8" ) );
    QCOMPARE( Amarok::vfatPath( "COM8.ext" ), QString( "_COM8.ext" ) );

    QCOMPARE( Amarok::vfatPath( "COM9" ), QString( "_COM9" ) );
    QCOMPARE( Amarok::vfatPath( "COM9.ext" ), QString( "_COM9.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT1" ), QString( "_LPT1" ) );
    QCOMPARE( Amarok::vfatPath( "LPT1.ext" ), QString( "_LPT1.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT2" ), QString( "_LPT2" ) );
    QCOMPARE( Amarok::vfatPath( "LPT2.ext" ), QString( "_LPT2.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT3" ), QString( "_LPT3" ) );
    QCOMPARE( Amarok::vfatPath( "LPT3.ext" ), QString( "_LPT3.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT4" ), QString( "_LPT4" ) );
    QCOMPARE( Amarok::vfatPath( "LPT4.ext" ), QString( "_LPT4.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT5" ), QString( "_LPT5" ) );
    QCOMPARE( Amarok::vfatPath( "LPT5.ext" ), QString( "_LPT5.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT6" ), QString( "_LPT6" ) );
    QCOMPARE( Amarok::vfatPath( "LPT6.ext" ), QString( "_LPT6.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT7" ), QString( "_LPT7" ) );
    QCOMPARE( Amarok::vfatPath( "LPT7.ext" ), QString( "_LPT7.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT8" ), QString( "_LPT8" ) );
    QCOMPARE( Amarok::vfatPath( "LPT8.ext" ), QString( "_LPT8.ext" ) );

    QCOMPARE( Amarok::vfatPath( "LPT9" ), QString( "_LPT9" ) );
    QCOMPARE( Amarok::vfatPath( "LPT9.ext" ), QString( "_LPT9.ext" ) );

    /* Device names in different cases: */
    QCOMPARE( Amarok::vfatPath( "con" ), QString( "_con" ) );
    QCOMPARE( Amarok::vfatPath( "con.ext" ), QString( "_con.ext" ) );

    QCOMPARE( Amarok::vfatPath( "cON" ), QString( "_cON" ) );
    QCOMPARE( Amarok::vfatPath( "cON.ext" ), QString( "_cON.ext" ) );

    /* This one is ok :) */
    QCOMPARE( Amarok::vfatPath( "CONCERT" ), QString( "CONCERT" ) );
    QCOMPARE( Amarok::vfatPath( "CONCERT.ext" ), QString( "CONCERT.ext" ) );
}
