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

#include <QJSEngine>

using namespace AmarokScript;

ScriptableServiceScript::ScriptableServiceScript( QJSEngine* engine )
    : QObject( engine )
    , m_scriptEngine( engine )
{
    DEBUG_BLOCK
    QJSValue scriptObj = engine->newQObject( this );
    scriptObj.setPrototype( QJSValue() );
    const QJSValue ctor = scriptObj.property(QStringLiteral("ScriptableServiceScript_prototype_ctor"));
    engine->globalObject().setProperty( QStringLiteral("ScriptableServiceScript_prototype_ctor"), ctor );
    // Simulates creation of ScriptableServiceScript object of QTScript
    engine->evaluate( QStringLiteral("function ScriptableServiceScript( serviceName, levels, shortDescription, rootHtml, showSearchBar) {"
        "Object.assign( this, ScriptableServiceScript_prototype_ctor(serviceName, levels, shortDescription, rootHtml, showSearchBar) ); }")
    );
}

QObject*
ScriptableServiceScript::ScriptableServiceScript_prototype_ctor( QString serviceName, int levels, QString shortDescription, QString rootHtml, bool showSearchBar )
{
    DEBUG_BLOCK
    if( !ScriptManager::instance()->m_scripts.contains( serviceName ) )
    {
        error() << "The name of the scriptable script should be the same with the one in the script.spec file!";
        return nullptr;
    }
    QObject* qObj = ScriptManager::instance()->m_scripts.value(serviceName)->service();
    if (The::scriptableServiceManager()->service(serviceName)) {
        The::scriptableServiceManager()->removeRunningScript(serviceName);
    }
    The::scriptableServiceManager()->initService(serviceName, levels, shortDescription, rootHtml, showSearchBar);
    return qObj;
}

QJSValue
ScriptableServiceScript::ScriptableServiceScript_prototype_populate( QJSEngine *engine )
{
    Q_UNUSED( engine );
    debug() << "prototype populating here!";
    return QJSValue( QJSValue::UndefinedValue );
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
