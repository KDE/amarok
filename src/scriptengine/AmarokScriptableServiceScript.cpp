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

ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* engine )
: QObject( kapp )
, m_scriptEngine( engine )
{
    DEBUG_BLOCK

}

ScriptableServiceScript::~ScriptableServiceScript()
{
    DEBUG_BLOCK
}

int ScriptableServiceScript::insertItem( int level, const QString name, const QString infoHtml, const QString playableUrl, const QString callbackData )
{
    DEBUG_BLOCK
	
    debug() << "service name = " << m_serviceName;
    return The::scriptableServiceManager()->insertItem( m_serviceName, level, m_currentId, name, infoHtml, callbackData, playableUrl );
}

void ScriptableServiceScript::slotPopulate( QString name, int level, int parent_id, QString callbackData, QString filter )
{
    DEBUG_BLOCK
	
    debug() << "service name = " << name;
    debug() << "populating...";
    debug() << level << parent_id << callbackData << filter;
	
    m_serviceName = name;
    m_currentId = parent_id;
    QScriptValueList args;
    args << QScriptValue( m_scriptEngine, level ) << QScriptValue( m_scriptEngine, callbackData ) << QScriptValue( m_scriptEngine, filter );
    thisObject().property( "prototype" ).property( "populate" ).call( QScriptValue(), args );
	
    The::scriptableServiceManager()->donePopulating( m_serviceName, m_currentId );
}

void ScriptableServiceScript::setServiceName( QString name )
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        m_serviceName = name;
}

QString ScriptableServiceScript::serviceName() const
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        return m_serviceName;
    else
        return QString();
}

void ScriptableServiceScript::setLevels( int levels )
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        m_levels = levels;
}

int ScriptableServiceScript::levels() const
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        return m_levels;
    else
        return 0;
}

void ScriptableServiceScript::setShortDescription( QString shortDescription )
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        m_shortDescription = shortDescription;
}

QString ScriptableServiceScript::shortDescription() const
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        return m_shortDescription;
    else
        return QString();
}

void ScriptableServiceScript::setRootHtml( QString rootHtml )
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        m_rootHtml = rootHtml;
}

QString ScriptableServiceScript::rootHtml() const
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        return m_rootHtml;
    else
        return QString();
}

void ScriptableServiceScript::setShowSearchBar( bool showSearchBar )
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        m_showSearchBar = showSearchBar;
}

bool ScriptableServiceScript::showSearchBar() const
{
    ScriptableServiceScript* item = qscriptvalue_cast<ScriptableServiceScript*>( thisObject() );
    if ( item )
        return m_showSearchBar;
    else
        return 0;
}


#include "AmarokScriptableServiceScript.moc"
