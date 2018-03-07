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

#include "ProxyLogger.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QNetworkReply>

ProxyLogger::ProxyLogger()
        : Amarok::Logger()
        , m_logger( 0 )
        , m_timer( 0 )
{
    // ensure that the object livs in the GUI thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );

    m_timer = new QTimer( this );
    connect( m_timer, &QTimer::timeout, this, &ProxyLogger::forwardNotifications );
    m_timer->setSingleShot( true );
    m_timer->setInterval( 0 );

    connect( this, &ProxyLogger::startTimer, this, &ProxyLogger::slotStartTimer );
}

ProxyLogger::~ProxyLogger()
{
    //nothing to do
}

void
ProxyLogger::setLogger( Amarok::Logger *logger )
{
    m_logger = logger;
    emit startTimer();
}

Amarok::Logger *
ProxyLogger::logger() const
{
    return m_logger;
}

void
ProxyLogger::slotStartTimer()
{
    if( m_logger && !m_timer->isActive() )
        m_timer->start();
}

void
ProxyLogger::shortMessage( const QString &text )
{
    QMutexLocker locker( &m_lock );
    m_shortMessageQueue.enqueue( text );
    emit startTimer();
}

void
ProxyLogger::longMessage( const QString &text, MessageType type )
{
    QMutexLocker locker( &m_lock );
    LongMessage msg;
    msg.first = text;
    msg.second = type;
    m_longMessageQueue.enqueue( msg );
    emit startTimer();
}

void
ProxyLogger::newProgressOperation( KJob *job, const QString &text, QObject *obj, const char *slot, Qt::ConnectionType type )
{
    QMutexLocker locker( &m_lock );
    ProgressData data;
    data.job = job;
    data.text = text;
    data.cancelObject = obj;
    data.slot = slot;
    data.type = type;
    m_progressQueue.enqueue( data );
    emit startTimer();
}

void
ProxyLogger::newProgressOperation( QNetworkReply *reply, const QString &text, QObject *obj, const char *slot, Qt::ConnectionType type )
{
    QMutexLocker locker( &m_lock );
    ProgressData data;
    data.reply = reply;
    data.text = text;
    data.cancelObject = obj;
    data.slot = slot;
    data.type = type;
    m_progressQueue.enqueue( data );
    emit startTimer();
}

void
ProxyLogger::newProgressOperation( QObject *sender, const QString &text, int maximum, QObject *obj,
                                   const char *slot, Qt::ConnectionType type )
{
    QMutexLocker locker( &m_lock );
    ProgressData data;
    data.sender = sender;
    data.text = text;
    data.maximum = maximum;
    data.cancelObject = obj;
    data.slot = slot;
    data.type = type;
    m_progressQueue.enqueue( data );
    connect( sender, SIGNAL(totalSteps(int)), SLOT(slotTotalSteps(int)) );
    emit startTimer();
}

void
ProxyLogger::forwardNotifications()
{
    QMutexLocker locker( &m_lock );
    if( !m_logger )
        return; //can't do anything before m_logger is created.

    while( !m_shortMessageQueue.isEmpty() )
    {
        m_logger->shortMessage( m_shortMessageQueue.dequeue() );
    }
    while( !m_longMessageQueue.isEmpty() )
    {
        LongMessage msg = m_longMessageQueue.dequeue();
        m_logger->longMessage( msg.first, msg.second );
    }
    while( !m_progressQueue.isEmpty() )
    {
        ProgressData d = m_progressQueue.dequeue();
        if( d.job )
        {
            m_logger->newProgressOperation( d.job.data(), d.text, d.cancelObject.data(),
                                            d.cancelObject.data() ? d.slot : 0 , d.type );
        }
        else if( d.reply )
        {
            m_logger->newProgressOperation( d.reply.data(), d.text, d.cancelObject.data(),
                                            d.cancelObject.data() ? d.slot : 0 , d.type );
        }
        else if( d.sender )
        {
            // m_logger handles the signals from now on
            disconnect( d.sender.data(), 0, this, 0 );
            m_logger->newProgressOperation( d.sender.data(), d.text, d.maximum,
                                            d.cancelObject.data(),
                                            d.cancelObject.data() ? d.slot : 0 , d.type );
        }
    }
}

void
ProxyLogger::slotTotalSteps( int totalSteps )
{
    QObject *operation = sender();
    if( !operation )
        // warning, slotTotalSteps can only be connected to progress operation QObject signal
        return;
    QMutableListIterator<ProgressData> it( m_progressQueue );
    while( it.hasNext() )
    {
        ProgressData &data = it.next();
        if( data.sender.data() != operation )
            continue;
        data.maximum = totalSteps;
        return;
    }
    // warning, operation not found in m_progressQueue
}

