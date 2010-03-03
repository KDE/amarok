/****************************************************************************************
* Copyright (c) 2010 Nathan Sala <sala.nathan@gmail.com>                               *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#ifndef TESTDATAENGINE_H
#define TESTDATAENGINE_H
#include <QString>
#include <Plasma/DataEngine>

/**
 * This class provides a method to perform "black box" unit tests on data engines
 * by retrieving the DataEngine through the KDE plugin manager
 */
class TestDataEngine
{
    public:
        TestDataEngine( const QString identifier);
        ~TestDataEngine();
        
    protected:
        Plasma::DataEngine* m_engine;
};

#endif // TESTDATAENGINE_H
