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

    ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
        ScriptEngine->setDefaultPrototype( qMetaTypeId<ScriptableServiceScript*>(), QScriptValue() );
    }

    ScriptableServiceScript::~ScriptableServiceScript()
    {
    }

    bool ScriptableServiceScript::initService( const QString &name, int levels, const QString &shortDescription, const QString &rootHtml, bool showSearchBar )
    {
        DEBUG_BLOCK
        return The::scriptableServiceManager()->initService( name, levels, shortDescription, rootHtml, showSearchBar );
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
        emit populate( level, parent_id, path, filter );
    }

#include "AmarokScriptableServiceScript.moc"
