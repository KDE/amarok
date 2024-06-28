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

#include "Logger.h"

#include <QNetworkReply>
#include <QPointer>
#include <QTimer>

#include <KJob>


// Durations for which messages are being saved.
#define SHORT_MESSAGE_DURATION 10000
#define LONG_MESSAGE_DURATION 10000

struct LongMessage
{
    QString text;
    Amarok::Logger::MessageType type;
};

struct ProgressData
{
    QPointer<QObject> sender;
    QMetaMethod increment;
    QMetaMethod end;
    QPointer<KJob> job;
    QPointer<QNetworkReply> reply;
    QString text;
    int maximum;
    QPointer<QObject> cancelObject;
    std::function<void ()> function;
    Qt::ConnectionType type;

    bool operator==( const ProgressData &other ) const
    {
        return sender == other.sender &&
               job == other.job &&
               reply == other.reply &&
               increment == other.increment &&
               end == other.end &&
               text == other.text &&
               maximum == other.maximum &&
               cancelObject == other.cancelObject &&
               function.target_type() == other.function.target_type() &&
               type == other.type;
    }
};

QMutex Amarok::Logger::s_mutex;
QList<Amarok::Logger*> Amarok::Logger::s_loggers;
QList<QString> Amarok::Logger::s_shortMessageList;
QList<LongMessage> Amarok::Logger::s_longMessageList;
QList<ProgressData> Amarok::Logger::s_progressList;

Amarok::Logger::Logger()
{
    QMutexLocker locker( &s_mutex );
    s_loggers << this;

    QTimer::singleShot( 0, [this] () { this->loadExistingMessages(); } );
}

Amarok::Logger::~Logger()
{
    QMutexLocker locker( &s_mutex );
    s_loggers.removeAll( this );
}

void Amarok::Logger::addProgressOperation( KJob* job, QNetworkReply* reply, QObject* sender, QMetaMethod increment, const QMetaMethod& end,
                                           const QString& text, int maximum, QObject* context, const std::function<void ()>& function, Qt::ConnectionType type )
{
    ProgressData data;
    data.sender = sender;
    data.job = job;
    data.reply = reply;
    data.increment = increment;
    data.end = end;
    data.text = text;
    data.maximum = maximum;
    data.cancelObject = context;
    data.function = function;
    data.type = type;

    QMutexLocker locker( &s_mutex );
    s_progressList << data;

    auto removeFunction = [data] () {
        QMutexLocker locker( &s_mutex );
        s_progressList.removeAll( data );
    };

    if( job )
    {
        QObject::connect( job, &QObject::destroyed, removeFunction );
        for( const auto &logger : s_loggers )
            logger->newProgressOperationImpl( job, text, context, function, type );
    }
    else if( reply )
    {
        QObject::connect( reply, &QObject::destroyed, removeFunction );
        for( const auto &logger : s_loggers )
            logger->newProgressOperationImpl( reply, text, context, function, type );
    }
    else if( sender )
    {
        QObject::connect( sender, &QObject::destroyed, removeFunction );
        for( const auto &logger : s_loggers )
            logger->newProgressOperationImpl( sender, increment, end, text, maximum, context, function, type );
    }
}

void Amarok::Logger::shortMessage( const QString& text )
{
    if( text.isEmpty() )
        return;

    QMutexLocker locker( &s_mutex );
    s_shortMessageList << text;

    for( const auto &logger : s_loggers )
        logger->shortMessageImpl( text );

    auto removeFunction = [text] () {
        QMutexLocker locker( &s_mutex );
        s_shortMessageList.removeAll( text );
    };

    QTimer::singleShot( SHORT_MESSAGE_DURATION, removeFunction );
}

void Amarok::Logger::longMessage( const QString& text, Amarok::Logger::MessageType type )
{
    if( text.isEmpty() )
        return;

    LongMessage message;
    message.text = text;
    message.type = type;
    QMutexLocker locker( &s_mutex );
    s_longMessageList << message;

    for( const auto &logger : s_loggers )
        logger->longMessageImpl( text, type );

    auto removeFunction = [text] () {
        QMutexLocker locker( &s_mutex );
        s_shortMessageList.removeAll( text );
    };

    QTimer::singleShot( LONG_MESSAGE_DURATION, removeFunction );
}

void Amarok::Logger::loadExistingMessages()
{
    QMutexLocker locker( &s_mutex );
    for( const auto &data : s_progressList )
    {
        if( data.job )
            newProgressOperationImpl( data.job, data.text, data.cancelObject, data.function, data.type );
        else if( data.reply )
            newProgressOperationImpl( data.reply, data.text, data.cancelObject, data.function, data.type );
        else if( data.sender )
            newProgressOperationImpl( data.sender, data.increment, data.end, data.text, data.maximum, data.cancelObject, data.function, data.type );
    }

    for( const auto &data : s_shortMessageList )
        shortMessageImpl( data );

    for( const auto &data : s_longMessageList )
        longMessageImpl( data.text, data.type );
}
