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

/**
 * @class AmarokTest
 * @short Runs test scripts for Amarok, e.g. for unit testing
 */

class AmarokTest : public QCoreApplication
{
    Q_OBJECT // for slots

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


private:
    /**
     * Prepares the engine for usage: adds bindings, etc.
     */
    void prepareTestEngine();

    // Disable copy constructor and assignment
    AmarokTest( const AmarokTest& );
    AmarokTest& operator= ( const AmarokTest& );

    QScriptEngine m_engine;
    QString       m_currentlyRunning;

};


#endif // AMAROKTEST_H
