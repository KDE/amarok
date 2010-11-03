
/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef SCANMANAGERMOCK_H
#define SCANMANAGERMOCK_H

#include <gmock/gmock.h>

#include <core-impl/collections/sqlcollection/SqlCollection.h>

class ScanManagerMock : public ScanManager
{
public:

    MOCK_METHOD1( setBlockScan, void( bool ) );
    MOCK_METHOD1( isDirInCollection, bool( const QString& ) );

    MOCK_METHOD0( startFullScan, void() );
    MOCK_METHOD1( startIncrementalScan, void( const QString& ) );
    MOCK_METHOD1( abort, void( const QString& ) );
};

#endif
