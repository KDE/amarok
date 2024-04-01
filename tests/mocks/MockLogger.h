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

#include "core/logger/Logger.h"

using ::testing::Return;
using ::testing::An;
using ::testing::_;

namespace Amarok
{
    class MockLogger : public Amarok::Logger
    {
    public:
        MockLogger() : Amarok::Logger()
        {
            ON_CALL( *this, shortMessageImpl( _ ) ).WillByDefault( Return() );
            ON_CALL( *this, longMessageImpl( _, _ ) ).WillByDefault( Return() );
            ON_CALL( *this, newProgressOperationImpl( An<KJob*>(), _, _, _, _ ) ).WillByDefault( Return() );
            ON_CALL( *this, newProgressOperationImpl( An<QNetworkReply*>(), _, _, _, _ ) ).WillByDefault( Return() );
            ON_CALL( *this, newProgressOperationImpl( An<QObject *>(), _, _, _, _, _, _, _ ) ).WillByDefault( Return() );
        }

        MOCK_METHOD( void, shortMessageImpl, ( const QString& ), (override) );
        MOCK_METHOD( void, longMessageImpl, ( const QString&, Amarok::Logger::MessageType ), (override) );
        MOCK_METHOD( void, newProgressOperationImpl, ( KJob*, const QString&, QObject*, const std::function<void ()>&,
                                                      Qt::ConnectionType ), (override) );
        MOCK_METHOD( void, newProgressOperationImpl, ( QNetworkReply*, const QString&, QObject*,
                                                      const std::function<void ()>&, Qt::ConnectionType ), (override) );
        MOCK_METHOD( void, newProgressOperationImpl, ( QObject *, const QMetaMethod &, const QMetaMethod &, const QString&,
                                                      int, QObject*, const std::function<void ()>&, Qt::ConnectionType ), (override) );
    };
}

#endif

