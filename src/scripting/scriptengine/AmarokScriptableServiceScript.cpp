/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "AmarokScriptableServiceScript"

#include "AmarokScriptableServiceScript.h"

#include "App.h"
#include "core/support/Debug.h"
#include "services/scriptable/ScriptableServiceManager.h"
#include "scripting/scriptengine/AmarokStreamItemScript.h"
#include "scripting/scriptmanager/ScriptManager.h"

#include <QScriptEngine>

using namespace AmarokScript;

ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* engine )
    : QObject( engine )
    , m_scriptEngine( engine )
{
    DEBUG_BLOCK
    m_scriptEngine = engine;
    engine->setDefaultPrototype( qMetaTypeId<ScriptableServiceScript*>(), QScriptValue() );
    const QScriptValue ctor = engine->newFunction( ScriptableServiceScript_prototype_ctor );
    engine->globalObject().setProperty( QStringLiteral("ScriptableServiceScript"), ctor );
}

QScriptValue
ScriptableServiceScript::ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine )
{
    DEBUG_BLOCK
    QString serviceName = context->argument(0).toString();
    int levels = context->argument(1).toInt32();
    QString shortDescription = context->argument(2).toString();
    QString rootHtml = context->argument(3).toString();
    bool showSearchBar = context->argument(4).toBoolean();
    if( !ScriptManager::instance()->m_scripts.contains( serviceName ) )
    {
        error() << "The name of the scriptable script should be the same with the one in the script.spec file!";
        return engine->undefinedValue();
    }
    QScriptValue obj = engine->newQObject( context->thisObject(), ScriptManager::instance()->m_scripts.value(serviceName)->service(),
                                           QScriptEngine::AutoOwnership, QScriptEngine::ExcludeSuperClassContents );
    engine->globalObject().setProperty( QStringLiteral("ScriptableServiceScript"), obj );
    The::scriptableServiceManager()->initService( serviceName, levels, shortDescription, rootHtml, showSearchBar );
    return engine->undefinedValue();
}

QScriptValue
ScriptableServiceScript::ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine )
{
    Q_UNUSED( context );
    debug() << "prototype populating here!";
    return engine->undefinedValue();
}

int
ScriptableServiceScript::insertItem( StreamItem* item )
{
    return The::scriptableServiceManager()->insertItem( m_serviceName, item->level(), m_currentId, item->itemName(), item->infoHtml(), item->callbackData(), item->playableUrl(),
                                                        item->album(), item->artist(), item->genre(), item->composer(), item->year(),
                                                        item->coverUrl() );
}

int
ScriptableServiceScript::donePopulating() const
{
    DEBUG_BLOCK

    The::scriptableServiceManager()->donePopulating( m_serviceName, m_currentId );
    return -1; // Fixme: return the right thing.
}

void
ScriptableServiceScript::slotPopulate( const QString &name, int level, int parent_id, const QString &callbackData, const QString &filter )
{
    DEBUG_BLOCK
    m_currentId = parent_id;
    m_serviceName = name;
    Q_EMIT( populate( level, callbackData, filter ) );
}

void
ScriptableServiceScript::slotRequestInfo( const QString &name, int level, const QString &callbackData )
{
    DEBUG_BLOCK
    m_serviceName = name;
    Q_EMIT( fetchInfo( level, callbackData ) );
}

void
ScriptableServiceScript::slotCustomize( const QString &name )
{
    DEBUG_BLOCK
    m_serviceName = name;
    Q_EMIT( customize() );
}


void
ScriptableServiceScript::setIcon( const QPixmap &icon )
{
    The::scriptableServiceManager()->setIcon( m_serviceName, icon );
}

void
ScriptableServiceScript::setEmblem( const QPixmap &emblem )
{
    The::scriptableServiceManager()->setEmblem( m_serviceName, emblem );
}

void
ScriptableServiceScript::setScalableEmblem ( const QString& emblemPath )
{
    The::scriptableServiceManager()->setScalableEmblem( m_serviceName, emblemPath );
}

void
ScriptableServiceScript::setCurrentInfo( const QString &infoHtml )
{
    The::scriptableServiceManager()->setCurrentInfo( m_serviceName, infoHtml );
}
