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

#ifndef AMAROK_MOCKLOGGER_H
#define AMAROK_MOCKLOGGER_H

#include <gmock/gmock.h>

#include "core/interfaces/Logger.h"

using ::testing::Return;
using ::testing::_;

namespace Amarok
{
    class MockLogger : public Amarok::Logger
    {
    public:
        MockLogger() : Amarok::Logger()
        {
            ON_CALL( *this, shortMessage( _ ) ).WillByDefault( Return() );
            ON_CALL( *this, longMessage( _, _ ) ).WillByDefault( Return() );
            ON_CALL( *this, newProgressOperation( _, _, _, _, _ ) ).WillByDefault( Return() );
        }

        MOCK_METHOD1( shortMessage, void( const QString& ) );
        MOCK_METHOD2( longMessage, void( const QString&, Amarok::Logger::MessageType ) );
        MOCK_METHOD5( newProgressOperation, void( KJob*, const QString&, QObject*, const char*, Qt::ConnectionType ) );
    };
}

#endif

