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

#include "TestAmarok.h"
#include "core/support/Amarok.h"
#include "config-amarok-test.h"

#include <KLocalizedString>

#include <QTest>
#include <QDir>
#include <QDateTime>


QTEST_MAIN( TestAmarok )

TestAmarok::TestAmarok()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

QString
TestAmarok::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QLatin1Char('/') + relPath );
}

void TestAmarok::testAsciiPath()
{
    QCOMPARE( Amarok::asciiPath( QStringLiteral("") ), QStringLiteral( "" ) );
    QCOMPARE( Amarok::asciiPath( QStringLiteral("/home/sven") ), QStringLiteral( "/home/sven" ) );

    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/äöü" ) ), QStringLiteral( "/___" ) );
    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/äöütest" ) ), QStringLiteral( "/___test" ) );
    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/äöü/test" ) ), QStringLiteral( "/___/test" ) );
    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/123/" ) ), QStringLiteral( "/123/" ) );
    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/.hidden" ) ), QStringLiteral( "/.hidden" ) );
    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/here be dragons" ) ), QStringLiteral( "/here be dragons" ) );
    QCOMPARE( Amarok::asciiPath( QString::fromUtf8( "/!important/some%20stuff/what's this?" ) ), QStringLiteral( "/!important/some%20stuff/what's this?" ) );

    /* 0x7F = 127 = DEL control character, explicitly ok on *nix file systems */
    QCOMPARE( Amarok::asciiPath( QStringLiteral( "/abc" ) + QChar( 0x7F ) + QStringLiteral(".1") ), QString( QStringLiteral( "/abc" ) + QChar( 0x7F ) + QStringLiteral(".1") ) );

    /* random control character: ok */
    QCOMPARE( Amarok::asciiPath( QStringLiteral( "/abc" ) + QChar( 0x07 ) + QStringLiteral(".1") ), QString( QStringLiteral( "/abc" ) + QChar( 0x07 ) + QStringLiteral(".1") ) );

    /* null byte is not ok */
    QCOMPARE( Amarok::asciiPath( QStringLiteral( "/abc" ) + QChar( 0x00 ) + QStringLiteral(".1") ), QStringLiteral( "/abc_.1" ) );
}

void TestAmarok::testCleanPath()
{
    /* no changes expected here */
    QCOMPARE( Amarok::cleanPath( QStringLiteral( "" ) ), QStringLiteral( "" ) );
    QCOMPARE( Amarok::cleanPath( QStringLiteral( "abcdefghijklmnopqrstuvwxyz" ) ), QStringLiteral( "abcdefghijklmnopqrstuvwxyz" ) );
    QCOMPARE( Amarok::cleanPath( QStringLiteral( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) ), QStringLiteral( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) );
    QCOMPARE( Amarok::cleanPath( QStringLiteral( "/\\.,-+" ) ), QStringLiteral( "/\\.,-+" ) );

    /* German */
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "äöüß" ) ), QStringLiteral( "aeoeuess" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ÄÖÜß" ) ), QStringLiteral( "AeOeUess" ) ); // capital ß only exists in theory in the German language, but had been defined some time ago, iirc

    /* French */
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "áàéèêô" ) ), QStringLiteral( "aaeeeo" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ÁÀÉÈÊÔ" ) ), QStringLiteral( "AAEEEO" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "æ" ) ), QStringLiteral( "ae" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "Æ" ) ), QStringLiteral( "AE" ) );

    /* Czech and other east European languages */
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "çńǹýỳź" ) ), QStringLiteral( "cnnyyz" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ÇŃǸÝỲŹ" ) ), QStringLiteral( "CNNYYZ" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ěĺľôŕřůž" ) ), QStringLiteral( "ellorruz" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ" ) ), QStringLiteral( "ACDEEINORSTUUYZ" ) );

    /* Skandinavian languages */
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "åø" ) ), QStringLiteral( "aoe" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ÅØ" ) ), QStringLiteral( "AOE" ) );

    /* Spanish */
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ñóÿ" ) ), QStringLiteral( "noy" ) );
    QCOMPARE( Amarok::cleanPath( QString::fromUtf8( "ÑÓŸ" ) ), QStringLiteral( "NOY" ) );

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


void TestAmarok::testExtension()
{
    QCOMPARE( Amarok::extension( QStringLiteral("") ), QStringLiteral( "" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("...") ), QStringLiteral( "" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test") ), QStringLiteral( "" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.") ), QStringLiteral( "" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.mp3") ), QStringLiteral( "mp3" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.mP3") ), QStringLiteral( "mp3" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.MP3") ), QStringLiteral( "mp3" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.longextension") ), QStringLiteral( "longextension" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.long.extension") ), QStringLiteral( "extension" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("test.m") ), QStringLiteral( "m" ) );
    QCOMPARE( Amarok::extension( QString::fromUtf8( "test.äöü" ) ), QString::fromUtf8( "äöü" ) );
    QCOMPARE( Amarok::extension( QString::fromUtf8( "test.ÄÖÜ" ) ), QString::fromUtf8( "äöü" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("..test.mp3") ), QStringLiteral( "mp3" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("..te st.mp3") ), QStringLiteral( "mp3" ) );
    QCOMPARE( Amarok::extension( QStringLiteral("..te st.m p3") ), QStringLiteral( "m p3" ) );
}

void TestAmarok::testManipulateThe()
{
    QString teststring;

    Amarok::manipulateThe( teststring = QStringLiteral(""), true );
    QCOMPARE( teststring, QStringLiteral( "" ) );

    Amarok::manipulateThe( teststring = QStringLiteral(""), false );
    QCOMPARE( teststring, QStringLiteral( "" ) );

    Amarok::manipulateThe( teststring = QLatin1Char('A'), true );
    QCOMPARE( teststring, QStringLiteral( "A" ) );

    Amarok::manipulateThe( teststring = QLatin1Char('A'), false );
    QCOMPARE( teststring, QStringLiteral( "A" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("ABC"), true );
    QCOMPARE( teststring, QStringLiteral( "ABC" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("ABC"), false );
    QCOMPARE( teststring, QStringLiteral( "ABC" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("The Eagles"), true );
    QCOMPARE( teststring, QStringLiteral( "Eagles, The" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("Eagles, The"), false );
    QCOMPARE( teststring, QStringLiteral( "The Eagles" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("The The"), true );
    QCOMPARE( teststring, QStringLiteral( "The, The" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("The, The"), false );
    QCOMPARE( teststring, QStringLiteral( "The The" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("Something else"), true );
    QCOMPARE( teststring, QStringLiteral( "Something else" ) );

    Amarok::manipulateThe( teststring = QStringLiteral("The Äöü"), true );
    QCOMPARE( teststring, QStringLiteral( "Äöü, The" ) );

    Amarok::manipulateThe( teststring = QString::fromUtf8( "Äöü, The" ), false );
    QCOMPARE( teststring, QString::fromUtf8( "The Äöü" ) );
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

void TestAmarok::testVerboseTimeSince()
{
    /* There are two overloaded variants of this function */
    QCOMPARE( Amarok::verboseTimeSince( 0 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromSecsSinceEpoch( 0 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 10 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromSecsSinceEpoch( 10 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 100 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromSecsSinceEpoch( 100 ) ).isEmpty(), false );

    QCOMPARE( Amarok::verboseTimeSince( 1000 ).isEmpty(), false );
    QCOMPARE( Amarok::verboseTimeSince( QDateTime::fromSecsSinceEpoch( 1000 ) ).isEmpty(), false );
    /* any other good ideas what to test here? */
}

void TestAmarok::testVfatPath()
{
    QCOMPARE( Amarok::vfatPath( QStringLiteral("") ), QStringLiteral( "" ) );

    /* allowed characters */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("abcdefghijklmnopqrstuvwxyz") ), QStringLiteral( "abcdefghijklmnopqrstuvwxyz" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ") ), QStringLiteral( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("0123456789") ), QStringLiteral( "0123456789" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("! # $ % & ' ( ) - @ ^ _ ` { } ~") ), QStringLiteral( "! # $ % & ' ( ) - @ ^ _ ` { } ~" ) );

    /* only allowed in long file names */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("+,.;=[]") ), QStringLiteral( "+,.;=()" ) );

    /* illegal characters, without / and \ (see later tests) */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\"_*_:_<_>_?_|") ), QStringLiteral( "_____________" ) );

    /* illegal control characters: 0-31, 127 */
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x00 ) + QChar( 0x01 ) + QChar( 0x02 ) + QStringLiteral(".1")), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x03 ) + QChar( 0x04 ) + QChar( 0x05 ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x06 ) + QChar( 0x07 ) + QChar( 0x08 ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x09 ) + QChar( 0x0A ) + QChar( 0x0B ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x0C ) + QChar( 0x0D ) + QChar( 0x0E ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x0F ) + QChar( 0x10 ) + QChar( 0x11 ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x12 ) + QChar( 0x13 ) + QChar( 0x14 ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x15 ) + QChar( 0x16 ) + QChar( 0x17 ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x18 ) + QChar( 0x19 ) + QChar( 0x1A ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x1B ) + QChar( 0x1C ) + QChar( 0x1D ) + QStringLiteral(".1") ), QStringLiteral( "abc___.1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral( "abc" ) + QChar( 0x1E ) + QChar( 0x7F ) + QStringLiteral(".1") ), QStringLiteral( "abc__.1" ) ); // 0x7F = 127 = DEL control character

    /* trailing spaces in extension, directory and file names are being ignored (!) */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("test   ") ), QStringLiteral( "test  _" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("   test   ") ), QStringLiteral( "   test  _" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("test.ext   ") ), QStringLiteral( "test.ext  _" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("test   .ext   ") ), QStringLiteral( "test  _.ext  _" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("   test   .ext   ") ), QStringLiteral( "   test  _.ext  _" ) ); // yes, really!

    /* trailing dot in directory and file names are unsupported are being ignored (!) */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("test...") ), QStringLiteral( "test.._" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("...test...") ), QStringLiteral( "...test.._" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("test.ext...") ), QStringLiteral( "test.ext.._" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("test....ext...") ), QStringLiteral( "test....ext.._" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("...test....ext...") ), QStringLiteral( "...test....ext.._" ) );

    /* more tests of trailing spaces and dot in directory names for Windows */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\some\\folder   \\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\some\\folder  _\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\some   \\folder   \\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\some  _\\folder  _\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\...some   \\ev  il   \\folders...\\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\...some  _\\ev  il  _\\folders.._\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\some\\fol/der   \\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\some\\fol_der  _\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\some...\\folder...\\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\some.._\\folder.._\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\some\\fol/der...\\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\some\\fol_der.._\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("\\so..me.\\folder  .\\"), Amarok::WindowsBehaviour ), QStringLiteral( "\\so..me_\\folder  _\\" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral(".\\any"), Amarok::WindowsBehaviour ), QStringLiteral( ".\\any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("..\\any"), Amarok::WindowsBehaviour ), QStringLiteral( "..\\any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("...\\any"), Amarok::WindowsBehaviour ), QStringLiteral( ".._\\any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("a..\\any"), Amarok::WindowsBehaviour ), QStringLiteral( "a._\\any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any\\.\\any."), Amarok::WindowsBehaviour ), QStringLiteral( "any\\.\\any_" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any\\..\\any "), Amarok::WindowsBehaviour ), QStringLiteral( "any\\..\\any_" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any.\\...\\any"), Amarok::WindowsBehaviour ), QStringLiteral( "any_\\.._\\any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any \\a..\\any"), Amarok::WindowsBehaviour ), QStringLiteral( "any_\\a._\\any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("Music\\R.E.M.\\Automatic for the people"), Amarok::WindowsBehaviour ), QStringLiteral( "Music\\R.E.M_\\Automatic for the people" ) );

    /* more tests of trailing spaces and dot in directory names for Unix */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/some/folder   /"), Amarok::UnixBehaviour ), QStringLiteral( "/some/folder  _/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/some   /folder   /"), Amarok::UnixBehaviour ), QStringLiteral( "/some  _/folder  _/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/...some   /ev  il   /folders.../"), Amarok::UnixBehaviour ), QStringLiteral( "/...some  _/ev  il  _/folders.._/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/some/fol\\der   /"), Amarok::UnixBehaviour ), QStringLiteral( "/some/fol_der  _/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/some.../folder.../"), Amarok::UnixBehaviour ), QStringLiteral( "/some.._/folder.._/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/some/fol\\der.../"), Amarok::UnixBehaviour ), QStringLiteral( "/some/fol_der.._/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("/so..me./folder  ./"), Amarok::UnixBehaviour ), QStringLiteral( "/so..me_/folder  _/" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("./any"), Amarok::UnixBehaviour ), QStringLiteral( "./any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("../any"), Amarok::UnixBehaviour ), QStringLiteral( "../any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral(".../any"), Amarok::UnixBehaviour ), QStringLiteral( ".._/any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("a../any"), Amarok::UnixBehaviour ), QStringLiteral( "a._/any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any/./any."), Amarok::UnixBehaviour ), QStringLiteral( "any/./any_" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any/../any "), Amarok::UnixBehaviour ), QStringLiteral( "any/../any_" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any./.../any"), Amarok::UnixBehaviour ), QStringLiteral( "any_/.._/any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("any /a../any"), Amarok::UnixBehaviour ), QStringLiteral( "any_/a._/any" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("Music/R.E.M./Automatic for the people"), Amarok::UnixBehaviour ), QStringLiteral( "Music/R.E.M_/Automatic for the people" ) );

    /* Stepping deeper into M$ hell: reserved device names
     * See http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("CLOCK$") ), QStringLiteral( "_CLOCK$" ) );
    /* this one IS allowed according to
     * http://en.wikipedia.org/w/index.php?title=Filename&oldid=303934888#Comparison_of_file_name_limitations */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("CLOCK$.ext") ), QStringLiteral( "CLOCK$.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("CON") ), QStringLiteral( "_CON" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("CON.ext") ), QStringLiteral( "_CON.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("PRN") ), QStringLiteral( "_PRN" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("PRN.ext") ), QStringLiteral( "_PRN.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("AUX") ), QStringLiteral( "_AUX" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("AUX.ext") ), QStringLiteral( "_AUX.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("NUL") ), QStringLiteral( "_NUL" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("NUL.ext") ), QStringLiteral( "_NUL.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM1") ), QStringLiteral( "_COM1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM1.ext") ), QStringLiteral( "_COM1.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM2") ), QStringLiteral( "_COM2" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM2.ext") ), QStringLiteral( "_COM2.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM3") ), QStringLiteral( "_COM3" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM3.ext") ), QStringLiteral( "_COM3.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM4") ), QStringLiteral( "_COM4" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM4.ext") ), QStringLiteral( "_COM4.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM5") ), QStringLiteral( "_COM5" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM5.ext") ), QStringLiteral( "_COM5.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM6") ), QStringLiteral( "_COM6" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM6.ext") ), QStringLiteral( "_COM6.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM7") ), QStringLiteral( "_COM7" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM7.ext") ), QStringLiteral( "_COM7.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM8") ), QStringLiteral( "_COM8" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM8.ext") ), QStringLiteral( "_COM8.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM9") ), QStringLiteral( "_COM9" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("COM9.ext") ), QStringLiteral( "_COM9.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT1") ), QStringLiteral( "_LPT1" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT1.ext") ), QStringLiteral( "_LPT1.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT2") ), QStringLiteral( "_LPT2" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT2.ext") ), QStringLiteral( "_LPT2.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT3") ), QStringLiteral( "_LPT3" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT3.ext") ), QStringLiteral( "_LPT3.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT4") ), QStringLiteral( "_LPT4" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT4.ext") ), QStringLiteral( "_LPT4.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT5") ), QStringLiteral( "_LPT5" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT5.ext") ), QStringLiteral( "_LPT5.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT6") ), QStringLiteral( "_LPT6" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT6.ext") ), QStringLiteral( "_LPT6.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT7") ), QStringLiteral( "_LPT7" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT7.ext") ), QStringLiteral( "_LPT7.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT8") ), QStringLiteral( "_LPT8" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT8.ext") ), QStringLiteral( "_LPT8.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT9") ), QStringLiteral( "_LPT9" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("LPT9.ext") ), QStringLiteral( "_LPT9.ext" ) );

    /* Device names in different cases: */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("con") ), QStringLiteral( "_con" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("con.ext") ), QStringLiteral( "_con.ext" ) );

    QCOMPARE( Amarok::vfatPath( QStringLiteral("cON") ), QStringLiteral( "_cON" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("cON.ext") ), QStringLiteral( "_cON.ext" ) );

    /* This one is ok :) */
    QCOMPARE( Amarok::vfatPath( QStringLiteral("CONCERT") ), QStringLiteral( "CONCERT" ) );
    QCOMPARE( Amarok::vfatPath( QStringLiteral("CONCERT.ext") ), QStringLiteral( "CONCERT.ext" ) );
}
