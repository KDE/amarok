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
#include "ScriptManager.h"

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

ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* engine )
: QObject( kapp )
, m_scriptEngine( engine )
{
    DEBUG_BLOCK
    m_scriptEngine = engine;
    engine->setDefaultPrototype( qMetaTypeId<ScriptableServiceScript*>(), QScriptValue() );
    const QScriptValue ctor = engine->newFunction( ScriptableServiceScript_prototype_ctor );
    const QScriptValue populate = engine->newFunction( ScriptableServiceScript_prototype_populate );
    ctor.property( "prototype" ).setProperty( "populate", populate );
    engine->globalObject().setProperty( "ScriptableServiceScript", ctor );
//    qScriptConnect( this, SIGNAL( populate( QString, int, int, QString, QString ) ), QScriptValue() ,populate );
}

ScriptableServiceScript::~ScriptableServiceScript()
{
    DEBUG_BLOCK
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine )
{
    QString serviceName = context->argument(0).toString();
    int levels = context->argument(1).toInt32();
    QString shortDescription = context->argument(2).toString();
    QString rootHtml = context->argument(3).toString();
    bool showSearchBar = context->argument(4).toBoolean();
    QObject* obj = engine->globalObject().property( "ScriptableServiceScript" ).toQObject();
    if ( obj == 0 ) debug() << "null pointer!"; else debug() << "not null pointer!";
//    engine->newQObject( context->thisObject(), obj );
    The::scriptableServiceManager()->initService( serviceName, levels, shortDescription, rootHtml, showSearchBar );
    return engine->undefinedValue();
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine )
{
    debug() << "prototype populating here!";
}

int ScriptableServiceScript::insertItem( QString serviceName, int level, const QString name, const QString infoHtml, const QString playableUrl, const QString callbackData )
{
    DEBUG_BLOCK
	
    debug() << "service name = " << m_serviceName;
    return The::scriptableServiceManager()->insertItem( m_serviceName, level, m_currentId, name, infoHtml, callbackData, playableUrl );
}

int ScriptableServiceScript::donePopulating( QString serviceName, int parent_id )
{
    DEBUG_BLOCK

    The::scriptableServiceManager()->donePopulating( serviceName, parent_id );
}

void ScriptableServiceScript::slotPopulate( QString name, int level, int parent_id, QString callbackData, QString filter )
{
    DEBUG_BLOCK
    emit( populate( name, level, parent_id, callbackData, filter ) );

    debug() << "service name = " << name;
    debug() << "populating...";
    debug() << level << parent_id << callbackData << filter;
/*
    QScriptValueList args;
    args << QScriptValue( m_scriptEngine, level ) << QScriptValue( m_scriptEngine, callbackData ) << QScriptValue( m_scriptEngine, filter );
    ScriptManager::instance()->m_script[serviceName].serviceContext.thisObject().property( "prototype" ).property( "populate" ).call( QScriptValue(), args );
	
    The::scriptableServiceManager()->donePopulating( name, parent_id );
*/
}

#include "AmarokScriptableServiceScript.moc"
