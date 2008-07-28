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

ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* ScriptEngine )
: QObject( kapp )
{
    ScriptEngine->setDefaultPrototype( qMetaTypeId<ScriptableServiceScript*>(), QScriptValue() );
    QScriptValue ctor = ScriptEngine->newFunction( ScriptableServiceScript_ctor );
    ctor.property( "prototype" ).setProperty( "populate", ScriptEngine->newFunction( ScriptableServiceScript_prototype_populate ) );
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
    The::scriptableServiceManager()->initService( name, levels, shortDescription, rootHtml, showSearchBar );

    return engine->undefinedValue();
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine )
{
    DEBUG_BLOCK
}

int ScriptableServiceScript::insertItem( const QString &serviceName, int level, int parentId, const QString &name, const QString &infoHtml, const QString &callbackData, const QString &playableUrl)
{
    DEBUG_BLOCK
    return The::scriptableServiceManager()->insertItem( serviceName, level, parentId, name, infoHtml, callbackData, playableUrl );
}

void ScriptableServiceScript::donePopulating( const QString &serviceName, int parentId )
{
    DEBUG_BLOCK
    The::scriptableServiceManager()->donePopulating( serviceName, parentId );
}

void ScriptableServiceScript::slotPopulate( int level, int parent_id, QString path, QString filter )
{
    DEBUG_BLOCK
    debug() << "populating...";
}

#include "AmarokScriptableServiceScript.moc"
