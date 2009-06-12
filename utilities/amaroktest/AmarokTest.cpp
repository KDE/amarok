/***************************************************************************
 *   Copyright (C) 2009 Sven Krohlas <sven@getamarok.com>                  *
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

#include "AmarokTest.h"
#include "AmarokTest.moc"

// Amarok includes
#include "../../src/Amarok.h"
#include "../../src/Debug.h"

#include <KStandardDirs>

#include <QDateTime>
#include <QFile>


int
main( int argc, char *argv[] )
{
    AmarokTest tester( argc, argv );
    return tester.exec();
}


AmarokTest::AmarokTest( int &argc, char **argv )
        : QCoreApplication( argc, argv )
{
    int i;
    QString logsLocation = Amarok::saveLocation( "testresults/" );
    QString currentTime = QDateTime::currentDateTime().toString( "yyyy-MM-dd.HH-mm-ss" );
    QFile logFile( logsLocation + currentTime + ".log" );

    if( !logFile.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        ::debug() << "Unable to open log!";
        exit( 1 );
    }

    m_log.setDevice( &logFile );
    m_log << "<testrun>" << endl;

    prepareTestEngine();

    if( arguments().size() == 1 ) /** only our own program name: run all available tests */
    {
        m_allTests = KGlobal::dirs()->findAllResources( "data", "amarok/tests/*.js", KStandardDirs::Recursive );
        i = 0;

        while( i < m_allTests.size() ) {
            m_currentlyRunning = m_allTests.at( i );
            m_log << "<testscript name=\"" << m_currentlyRunning << "\">" << endl;
            runScript();
            i++;
            m_log << "</teststript>" << endl << endl;
        }
    }

    else /** run given test(s) */
    {
        i = 1;

        while( i < arguments().size() )
        {
            m_currentlyRunning = arguments().at( i );
            runScript();
            i++;
        }
    }

    m_log << "</testrun>\n";

    QString linkLocation = logsLocation + "LATEST";
    #ifdef Q_WS_WIN /** .lnk extension is required for links on Windows */
    linkLocation = linkLocation + ".lnk";
    #endif

    QFile symlink( linkLocation );
    symlink.remove();
    if( !logFile.link( linkLocation ) )
        ::debug() << "Unable to create link to log!";

    logFile.close();
    exit( 0 );
}


// AmarokTest::~AmarokTest()
// {
//  // cleanup
// }


/**
 * Utility functions for test scripts: public slots
 */

void
AmarokTest::debug( const QString& text ) const // Slot
{
    ::debug() << "SCRIPT" << m_currentlyRunning << ": " << text;
}


void
AmarokTest::startTimer() // Slot
{
    if( m_testTime.isNull() )
        m_testTime.start();
    else
        m_testTime.restart();
}


void
AmarokTest::testResult( QString testName, QString expected, QString actualResult ) // Slot
{
    m_log << "  <test>" << endl;
    m_log << "    <name>" << testName << "</name>" << endl;

    if( expected != actualResult )
    {
        m_log << "    <status>FAILED</status>" << endl;
        m_log << "    <expected>" << expected << "</expected>" << endl;
        m_log << "    <result>" << actualResult << "</result>" << endl;
    }

    else
        m_log << "    <status>PASSED</status>" << endl;

    m_log << "    <time>" << m_testTime.elapsed() << "</time>" << endl; // in milliseconds
    m_log << "  </test>" << endl;

    m_testTime.restart();
}


/**
 * Private
 */


void
AmarokTest::prepareTestEngine()
{
    /** Give test scripts access to everything in qt they might need */
    m_engine.importExtension( "qt.core" );
    m_engine.importExtension( "qt.gui" );
    m_engine.importExtension( "qt.network" );
    m_engine.importExtension( "qt.sql" );
    m_engine.importExtension( "qt.uitools" );
    m_engine.importExtension( "qt.webkit" );
    m_engine.importExtension( "qt.xml" );

    m_engine.setProcessEventsInterval( 100 );
}


void
AmarokTest::runScript()
{
    QFile testScript;

    testScript.setFileName( m_currentlyRunning );
    testScript.open( QIODevice::ReadOnly );
    m_engine.evaluate( testScript.readAll() );

    if( m_engine.hasUncaughtException() ) {
        ::debug() << "Uncaught exception in test script: " << m_currentlyRunning;
        ::debug() << "Line: " << m_engine.uncaughtExceptionLineNumber();
        ::debug() << "Exception: " << m_engine.uncaughtException().toString();
        ::debug() << "Backtrace: " << m_engine.uncaughtExceptionBacktrace();
    }

    testScript.close();
}
