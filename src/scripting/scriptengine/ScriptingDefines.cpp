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
#include <QQmlEngine>
#include <QRandomGenerator>
#include <QTimer>

using namespace AmarokScript;

AmarokScriptEngine::AmarokScriptEngine( QObject *parent )
: QJSEngine(parent)
, internalObject( QStringLiteral("UndocumentedAmarokScriptingInternals") )
{
    installExtensions(QJSEngine::TranslationExtension | QJSEngine::ConsoleExtension);
    QJSValue scriptObject = newQObject( this );
    QQmlEngine::setObjectOwnership( this, QQmlEngine::CppOwnership);
    globalObject().setProperty( internalObject, scriptObject );
    QJSValue setTimeoutObject = scriptObject.property( QStringLiteral("setTimeout") );
    Q_ASSERT( !setTimeoutObject.isUndefined() );
    Q_ASSERT( !globalObject().property( internalObject ).property( QStringLiteral("invokableDeprecatedCall") ).isUndefined() );
    globalObject().setProperty( QStringLiteral("setTimeout"), setTimeoutObject );
}

void
AmarokScriptEngine::setDeprecatedProperty( const QString &parent, const QString &name, const QJSValue &property )
{
    const QString objName = QStringLiteral( "%1%2" ).arg( name, QString::number( QRandomGenerator::global()->generate() ) );
    globalObject().property( internalObject ).setProperty( objName, property );
    const QString command = QStringLiteral( "Object.defineProperty( %1, \"%2\", {get : function(){ var iobj= %3; iobj.invokableDeprecatedCall(\""
                                                                            " %1.%2 \"); return iobj.%4; },\
                                                                            enumerable : true,\
                                                                            configurable : false} );" )
                                    .arg( parent, name, internalObject, objName );
    evaluate( command );
}

void
AmarokScriptEngine::invokableDeprecatedCall( const QString &call )
{
    warning() << "Deprecated function " << call;
    Q_EMIT deprecatedCall( call );
}

void
AmarokScriptEngine::setTimeout( const QJSValue &function, int time, const QJSValue &thisObject, const QJSValue &args )
{
    QTimer *timer = new QTimer( this );
    timer->setSingleShot( true );
    timer->setInterval( time );
    m_callbacks[timer] = QJSValueList() << function << thisObject << args;
    connect( timer, &QTimer::timeout, this, &AmarokScriptEngine::slotTimeout );
    timer->start();
}

void
AmarokScriptEngine::slotTimeout()
{
    QObject *timer = sender();
    if( !timer )
        return;

    QJSValueList args;
    QJSValue thisObject;
    if( m_callbacks[timer].size() > 1 )
    {
        thisObject = m_callbacks[timer][1];
        if( m_callbacks[timer].size() == 3 )
            for ( quint32 i = 0; i < m_callbacks[timer][2].property(QStringLiteral("length")).toUInt(); ++i )
                args << m_callbacks[timer][2].property( i );
    }
    m_callbacks[timer][0].callWithInstance( thisObject, args );
    m_callbacks.remove( timer );
    timer->deleteLater();
}

QJSValue
AmarokScriptEngine::enumObject( const QMetaEnum &metaEnum )
{
    QJSValue enumObj = newObject();
    for( int i = 0; i< metaEnum.keyCount(); ++i )
        enumObj.setProperty( QLatin1String(metaEnum.key(i)), QJSEngine::toScriptValue( metaEnum.value(i) ) );
    return enumObj;
}


AmarokScriptEngine::~AmarokScriptEngine()
{}

