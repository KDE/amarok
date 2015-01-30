/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#include "ScriptingDefines.h"

#include "core/support/Debug.h"

#include <QMetaEnum>
#include <QTimer>

using namespace AmarokScript;

AmarokScriptEngine::AmarokScriptEngine( QObject *parent )
: QScriptEngine(parent)
, internalObject( "UndocumentedAmarokScriptingInternals" )
{
    QScriptValue scriptObject = newQObject( this, QtOwnership,
                                            ExcludeChildObjects | ExcludeSuperClassContents );
    globalObject().setProperty( internalObject, scriptObject, QScriptValue::ReadOnly );
    QScriptValue setTimeoutObject = scriptObject.property( "setTimeout" );
    Q_ASSERT( !setTimeoutObject.isUndefined() );
    Q_ASSERT( !globalObject().property( internalObject ).property( "invokableDeprecatedCall" ).isUndefined() );
    globalObject().setProperty( "setTimeout", setTimeoutObject );
}

void
AmarokScriptEngine::setDeprecatedProperty( const QString &parent, const QString &name, const QScriptValue &property )
{
    const QString objName = QString( "%1%2" ).arg( name, QString::number( qrand() ) );
    globalObject().property( internalObject ).setProperty( objName, property, QScriptValue::ReadOnly | QScriptValue::SkipInEnumeration );
    const QString command = QString( "Object.defineProperty( %1, \"%2\", {get : function(){ var iobj= %3; iobj.invokableDeprecatedCall(\""
                                                                            " %1.%2 \"); return iobj.%4; },\
                                                                            enumerable : true,\
                                                                            configurable : false} );" )
                                    .arg( parent, name, internalObject, objName );
    evaluate( command );
}

void
AmarokScriptEngine::invokableDeprecatedCall( const QString &call )
{
    warning() << "Deprecated function " + call;
    emit deprecatedCall( call );
}

void
AmarokScriptEngine::setTimeout( const QScriptValue &function, int time, const QScriptValue &thisObject, const QScriptValue &args )
{
    QTimer *timer = new QTimer( this );
    timer->setSingleShot( true );
    timer->setInterval( time );
    m_callbacks[timer] = QScriptValueList() << function << thisObject << args;
    connect( timer, SIGNAL(timeout()), this, SLOT(slotTimeout()) );
    timer->start();
}

void
AmarokScriptEngine::slotTimeout()
{
    QObject *timer = sender();
    if( !timer )
        return;

    QScriptValueList args;
    QScriptValue thisObject;
    if( m_callbacks[timer].size() > 1 )
    {
        thisObject = m_callbacks[timer][1];
        if( m_callbacks[timer].size() == 3 )
            for ( quint32 i = 0; i < m_callbacks[timer][2].property("length").toUInt32(); ++i )
                args << m_callbacks[timer][2].property( i );
    }
    m_callbacks[timer][0].call( thisObject, args );
    m_callbacks.remove( timer );
    timer->deleteLater();
}

QScriptValue
AmarokScriptEngine::enumObject( const QMetaEnum &metaEnum )
{
    QScriptValue enumObj = newObject();
    for( int i = 0; i< metaEnum.keyCount(); ++i )
        enumObj.setProperty( metaEnum.key(i), QScriptEngine::toScriptValue( metaEnum.value(i) ) );
    return enumObj;
}


AmarokScriptEngine::~AmarokScriptEngine()
{}

#include "ScriptingDefines.moc"
