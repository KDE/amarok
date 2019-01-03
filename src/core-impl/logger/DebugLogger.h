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

#ifndef DEBUGLOGGER_H
#define DEBUGLOGGER_H

#include <core/logger/Logger.h>

#include <QObject>


/**
 * This class is for debugging messages send to the logging system.
 */
class DebugLogger : public Amarok::Logger, public QObject
{
public:
    DebugLogger( QObject *parent = nullptr );

protected:
    void shortMessageImpl( const QString& text ) override;
    void longMessageImpl( const QString& text, Amarok::Logger::MessageType type ) override;
    void newProgressOperationImpl( QObject* sender, const QMetaMethod& increment, const QMetaMethod& end, const QString& text, int maximum, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type ) override;
    void newProgressOperationImpl( QNetworkReply* reply, const QString& text, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type ) override;
    void newProgressOperationImpl( KJob* job, const QString& text, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type ) override;
};

#endif // DEBUGLOGGER_H
