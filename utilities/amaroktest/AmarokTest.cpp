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
#include "../../src/Debug.h"

#include <QDirIterator>
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
    prepareTestEngine();

   /** run given test */
   m_currentlyRunning = "";

   /** run all tests */
   m_currentlyRunning = "";
}


// AmarokTest::~AmarokTest()
// {
//  // cleanup
// }


void
AmarokTest::prepareTestEngine()
{
    /** Give test scripts access to everything in qt they might need */
    m_engine.importExtension( "qt.core" );
    m_engine.importExtension( "qt.gui" );
    m_engine.importExtension( "qt.sql" );
    m_engine.importExtension( "qt.webkit" );
    m_engine.importExtension( "qt.xml" );
    m_engine.importExtension( "qt.uitools" );
    m_engine.importExtension( "qt.network" );
}

void
AmarokTest::debug( const QString& text ) const
{
    ::debug() << "SCRIPT" << m_currentlyRunning << ": " << text;
}