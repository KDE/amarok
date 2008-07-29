/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "AmarokScriptableServiceScript.h"

#include "App.h"
#include "Debug.h"
#include "browsers/servicebrowser/scriptableservice/ScriptableServiceManager.h"

#include <QtScript>

StreamItem::StreamItem()
{
}

StreamItem::~StreamItem()
{
}

QString StreamItem::name() const
{
    return m_name;
}

QString StreamItem::infoHtml() const
{
    return m_infoHtml;
}

QString StreamItem::playableUrl() const
{
    return m_playableUrl;
}

QString StreamItem::callbackData() const
{
    return m_callbackData;
}

void StreamItem::setName( QString name )
{
    m_name = name;
}

void StreamItem::setInfoHtml( QString infoHtml )
{
    m_infoHtml = infoHtml;
}

void StreamItem::setPlayableUrl( QString playableUrl )
{
    m_playableUrl = playableUrl;
}

void StreamItem::setCallbackData( QString callbackData )
{
    m_callbackData = callbackData;
}


QString ScriptableServiceScript::m_serviceName;

ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* ScriptEngine )
: QObject( kapp )
{
    ScriptEngine->setDefaultPrototype( qMetaTypeId<ScriptableServiceScript*>(), QScriptValue() );
    QScriptValue ctor = ScriptEngine->newFunction( ScriptableServiceScript_ctor );
    QScriptValue populate = ScriptEngine->newFunction( ScriptableServiceScript_prototype_populate );
    ctor.property( "prototype" ).setProperty( "populate", populate );
    ScriptEngine->globalObject().setProperty( "ScriptableServiceScript", ctor );
}

ScriptableServiceScript::~ScriptableServiceScript()
{
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_ctor( QScriptContext *context, QScriptEngine *engine )
{
    QString name = context->argument( 0 ).toString();
    context->thisObject().setProperty( "name", QScriptValue( engine, name ) );
    int levels = context->argument( 1 ).toInt32();
    context->thisObject().setProperty( "levels", QScriptValue( engine, levels ) );
    QString shortDescription = context->argument( 2 ).toString();
    context->thisObject().setProperty( "shortDescription", QScriptValue( engine, shortDescription ) );
    QString rootHtml = context->argument( 3 ).toString();
    context->thisObject().setProperty( "rootHtml", QScriptValue( engine, rootHtml ) );
    bool showSearchBar = context->argument( 4 ).toBoolean();
    context->thisObject().setProperty( "showSearchBar", QScriptValue( engine, showSearchBar ) );
    m_serviceName = name;
    The::scriptableServiceManager()->initService( name, levels, shortDescription, rootHtml, showSearchBar );

    return engine->undefinedValue();
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine )
{
    DEBUG_BLOCK
}

int ScriptableServiceScript::insertItem( int level, const QString &name, const QString &infoHtml, const QString &playableUrl, const QString &callbackData )
{
    DEBUG_BLOCK

    debug() << "service name = " << m_serviceName;
    return The::scriptableServiceManager()->insertItem( m_serviceName, level, m_currentId, name, infoHtml, callbackData, playableUrl );
}

void ScriptableServiceScript::slotPopulate( int level, int parent_id, QString callbackData, QString filter )
{
    DEBUG_BLOCK

    debug() << "service name = " << m_serviceName;
    debug() << "populating...";
    m_currentId = parent_id;
    emit populate( level, callbackData, filter );

//    The::scriptableServiceManager()->donePopulating( m_serviceName, m_currentId );
}

#include "AmarokScriptableServiceScript.moc"
