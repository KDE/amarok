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

ProxyLogger::ProxyLogger()
        : Amarok::Logger()
        , m_logger( 0 )
        , m_initComplete( false )
        , m_timer( 0 )
{
    qRegisterMetaType<Logger *>( "Amarok::Logger*" );
    //ensure that the object livs in the GUI thread
    if( thread() == QCoreApplication::instance()->thread() )
    {
        init();
    }
    else
    {
        this->moveToThread( QCoreApplication::instance()->thread() );
    }
}

ProxyLogger::~ProxyLogger()
{
    //nothing to do
}

bool
ProxyLogger::event( QEvent *event )
{
    if( event->type() == QEvent::ThreadChange )
    {
        QTimer::singleShot( 0, this, SLOT( init() ) );
        return true;
    }
    else
    {
        return QObject::event( event );
    }
}

void
ProxyLogger::init()
{
    if( !m_initComplete )
    {
        m_timer = new QTimer( this );
        connect( m_timer, SIGNAL( timeout() ), this, SLOT( forwardNotifications() ) );
        m_timer->setSingleShot( true );
        m_timer->setInterval( 0 );
        m_initComplete = true;
    }
}

void
ProxyLogger::setLogger( Amarok::Logger *logger )
{
    m_logger = logger;
    startTimer();
}

Amarok::Logger*
ProxyLogger::logger() const
{
    return m_logger;
}

void
ProxyLogger::startTimer()
{
    if( m_initComplete && m_timer && m_logger && !m_timer->isActive() )
    {
        m_timer->start();
    }
}

void
ProxyLogger::shortMessage( const QString &text )
{
    QMutexLocker locker( &m_lock );
    m_shortMessageQueue.enqueue( text );
    startTimer();
}

void
ProxyLogger::longMessage( const QString &text, MessageType type )
{
    QMutexLocker locker( &m_lock );
    LongMessage msg;
    msg.first = text;
    msg.second = type;
    m_longMessageQueue.enqueue( msg );
    startTimer();
}

void
ProxyLogger::newProgressOperation( KJob *job, const QString &text, QObject *obj, const char *slot, Qt::ConnectionType type )
{
    QMutexLocker locker( &m_lock );
    ProgressData data;
    data.job = job;
    data.text = text;
    data.object = obj;
    data.slot = slot;
    data.type = type;
    m_progressQueue.enqueue( data );
    startTimer();
}

void
ProxyLogger::forwardNotifications()
{
    QMutexLocker locker( &m_lock );
    if( m_logger )
    {
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
                m_logger->newProgressOperation( d.job, d.text, d.object, d.object ? d.slot : 0 , d.type );
            }
        }
    }
}

#include "ProxyLogger.moc"
