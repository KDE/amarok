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

#ifndef AMAROKTEST_H
#define AMAROKTEST_H

#include <QCoreApplication>
#include <QScriptEngine>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTime>

/**
 * @class AmarokTest
 * @short Runs test scripts for Amarok, e.g. for unit testing
 */

class AmarokTest : public QCoreApplication
{
    Q_OBJECT

public:
    AmarokTest( int &argc, char **argv );
//     ~AmarokTest();

// TODO: test utils for scripts, like in QTest


public slots:
    /**
     * Print debug output to the shell.
     * @text The text to print.
     */
    void debug( const QString& text ) const;

    /**
     * Lets a script tell us that from now on time critival tasks are being performed.
     * From now on the rime between each call to testResult(...) will be measured until
     * the script ends.
     */
    void startTimer();

    /**
     * Writes the results of a test to the log
     * @testName Human readable name of the test.
     * @expected Human readable expected test result.
     * @actualResult Human readable actual result of the test.
     */
    void testResult( QString testName, QString expected, QString actualResult );

private:
    /**
     * Actually runs a test script.
     */
    void runScript();

    /**
     * Prepares the engine for usage: adds bindings, etc.
     */
    void prepareTestEngine();

    // Disable copy constructor and assignment
    AmarokTest( const AmarokTest& );
    AmarokTest& operator= ( const AmarokTest& );

    bool          m_measurePerf;

    QScriptEngine m_engine;
    QString       m_currentlyRunning;
    QString       m_logsLocation;
    QStringList   m_allTests;
    QTextStream   m_log;
    QTime         m_testTime;
};

#endif // AMAROKTEST_H
