/*
 * Copyright 2018  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DebugLogger.h"

#include "core/support/Debug.h"


DebugLogger::DebugLogger( QObject *parent )
    : QObject( parent )
{
    DEBUG_BLOCK
}

void DebugLogger::shortMessageImpl( const QString& text )
{
    DEBUG_BLOCK

    debug() << "Short message:" << text;
}

void DebugLogger::longMessageImpl( const QString& text, Amarok::Logger::MessageType type )
{
    DEBUG_BLOCK

    debug() << "Long message:" << text << type;
}

void DebugLogger::newProgressOperationImpl( QObject* sender, const QMetaMethod& increment, const QMetaMethod& end, const QString& text, int maximum, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type )
{
    Q_UNUSED( increment )
    Q_UNUSED( end )

    DEBUG_BLOCK

    debug() << "New progress operation with generic QObject:" << sender;
    debug() << "Text:" << text;
    debug() << "Maximum:" << maximum;
    debug() << "Object to call when canceled:" << context;
    debug() << "Member function to call when canceled:" << function.target_type().name();
    debug() << "Connection type:" << type;
}

void DebugLogger::newProgressOperationImpl( QNetworkReply* reply, const QString& text, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type )
{
    DEBUG_BLOCK

    debug() << "New progress operation with QNetworkReply:" << reply;
    debug() << "Text:" << text;
    debug() << "Object to call when canceled:" << context;
    debug() << "Member function to call when canceled:" << function.target_type().name();
    debug() << "Connection type:" << type;
}

void DebugLogger::newProgressOperationImpl( KJob* job, const QString& text, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type )
{
    DEBUG_BLOCK

    debug() << "New progress operation with KJob:" << job;
    debug() << "Text:" << text;
    debug() << "Object to call when canceled:" << context;
    debug() << "Member function to call when canceled:" << function.target_type().name();
    debug() << "Connection type:" << type;
}
