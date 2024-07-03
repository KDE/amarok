/*
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "DBusAbstractAdaptor.h"

#include <QMetaClassInfo>
#include <QDBusMessage>

#include "core/support/Debug.h"

DBusAbstractAdaptor::DBusAbstractAdaptor( QObject *parent )
    : QDBusAbstractAdaptor( parent )
    , m_connection( QDBusConnection::sessionBus() )
{
}

QDBusConnection DBusAbstractAdaptor::connection() const
{
    return m_connection;
}

void DBusAbstractAdaptor::setConnection( const QDBusConnection &conn )
{
    m_connection = conn;
}

QString DBusAbstractAdaptor::dBusPath() const
{
    return m_path;
}

void DBusAbstractAdaptor::setDBusPath( const QString &path )
{
    m_path = path;
}

void DBusAbstractAdaptor::signalPropertyChange( const QString &property, const QVariant &value )
{
    if ( m_updatedProperties.isEmpty() && m_invalidatedProperties.isEmpty() ) {
        QMetaObject::invokeMethod( this, "_m_emitPropertiesChanged", Qt::QueuedConnection );
        debug() << "MPRIS2: Queueing up a PropertiesChanged signal";
    }

    m_updatedProperties[property] = value;
}

void DBusAbstractAdaptor::signalPropertyChange( const QString &property )
{
    if ( !m_invalidatedProperties.contains( property ) ) {
        if ( m_updatedProperties.isEmpty() && m_invalidatedProperties.isEmpty() ) {
            QMetaObject::invokeMethod( this, "_m_emitPropertiesChanged", Qt::QueuedConnection );
            debug() << "MPRIS2: Queueing up a PropertiesChanged signal";
        }

        m_invalidatedProperties << property;
    }
}

void DBusAbstractAdaptor::_m_emitPropertiesChanged()
{
    Q_ASSERT( !m_path.isEmpty() );

    if( m_updatedProperties.isEmpty() && m_invalidatedProperties.isEmpty() ) {
        debug() << "MPRIS2: Nothing to do";
        return;
    }

    int ifaceIndex = metaObject()->indexOfClassInfo( "D-Bus Interface" );
    if ( ifaceIndex < 0 ) {
        warning() << "MPRIS2: No D-Bus interface given (missing Q_CLASSINFO)";
    } else {
        QDBusMessage signal = QDBusMessage::createSignal( m_path,
             QStringLiteral("org.freedesktop.DBus.Properties"),
             QStringLiteral("PropertiesChanged") );
        signal << QLatin1String(metaObject()->classInfo( ifaceIndex ).value());
        signal << m_updatedProperties;
        signal << m_invalidatedProperties;
        m_connection.send( signal );
    }

    m_updatedProperties.clear();
    m_invalidatedProperties.clear();
}

