/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "AmarokScriptableServiceScript.h"

#include "App.h"
#include "Debug.h"
#include "services/scriptable/ScriptableServiceManager.h"
#include "ScriptManager.h"


StreamItem::StreamItem( QScriptEngine *engine )
    : m_year( 0 )
{
    Q_UNUSED( engine );
}

StreamItem::~StreamItem()
{
}

QString StreamItem::itemName() const
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

int StreamItem::level() const
{
    return m_level;
}


QString StreamItem::album() const
{
    return m_album;
}

QString StreamItem::artist() const
{
    DEBUG_BLOCK
    return m_artist;
}

QString StreamItem::genre() const
{
    return m_genre;
}

QString StreamItem::composer() const
{
    return m_composer;
}

int StreamItem::year() const
{
    return m_year;
}

QString StreamItem::coverUrl()
{
    return m_coverUrl;;
}


void StreamItem::setItemName( QString name )
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

void StreamItem::setLevel( int level )
{
    m_level = level;
}

void StreamItem::setAlbum( QString album )
{
    m_album = album;
}

void StreamItem::setArtist( QString artist )
{
    m_artist = artist;
}

void StreamItem::setGenre( QString genre )
{
    m_genre = genre;
}

void StreamItem::setComposer( QString composer )
{
    m_composer = composer;
}

void StreamItem::setYear( int year )
{
    m_year = year;
}

void StreamItem::setCoverUrl( QString url )
{
    m_coverUrl = url;
}


ScriptableServiceScript::ScriptableServiceScript( QScriptEngine* engine )
: QObject( kapp )
, m_scriptEngine( engine )
{
    DEBUG_BLOCK
    m_scriptEngine = engine;
    engine->setDefaultPrototype( qMetaTypeId<ScriptableServiceScript*>(), QScriptValue() );
    const QScriptValue ctor = engine->newFunction( ScriptableServiceScript_prototype_ctor );
//    const QScriptValue populate = engine->newFunction( ScriptableServiceScript_prototype_populate );
//    ctor.property( "prototype" ).setProperty( "populate", populate );
    engine->globalObject().setProperty( "ScriptableServiceScript", ctor );
//    qScriptConnect( this, SIGNAL( populate( QString, int, int, QString, QString ) ), QScriptValue() ,populate );
}

ScriptableServiceScript::~ScriptableServiceScript()
{
    DEBUG_BLOCK
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_prototype_ctor( QScriptContext *context, QScriptEngine *engine )
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
    QScriptValue obj = engine->newQObject( context->thisObject(), ScriptManager::instance()->m_scripts[serviceName].servicePtr );
    engine->globalObject().setProperty( "ScriptableServiceScript", obj );
    The::scriptableServiceManager()->initService( serviceName, levels, shortDescription, rootHtml, showSearchBar );
    return engine->undefinedValue();
}

QScriptValue ScriptableServiceScript::ScriptableServiceScript_prototype_populate( QScriptContext *context, QScriptEngine *engine )
{
    Q_UNUSED( context );
    debug() << "prototype populating here!";
    return engine->undefinedValue(); // Fixme: return something.
}

int ScriptableServiceScript::insertItem( StreamItem* item )
{
    DEBUG_BLOCK
            debug() << "artistOverride: item->artist()";
    return The::scriptableServiceManager()->insertItem( m_serviceName, item->level(), m_currentId, item->itemName(), item->infoHtml(), item->callbackData(), item->playableUrl(),
                                                        item->album(), item->artist(), item->genre(), item->composer(), item->year(),
                                                        item->coverUrl() );
}

int ScriptableServiceScript::donePopulating() const
{
    DEBUG_BLOCK

    The::scriptableServiceManager()->donePopulating( m_serviceName, m_currentId );
    return -1; // Fixme: return the right thing.
}

void ScriptableServiceScript::slotPopulate( QString name, int level, int parent_id, QString callbackData, QString filter )
{
    DEBUG_BLOCK
    m_currentId = parent_id;
    m_serviceName = name;
    emit( populate( level, callbackData, filter ) );

/*
    QScriptValueList args;
    args << QScriptValue( m_scriptEngine, level ) << QScriptValue( m_scriptEngine, callbackData ) << QScriptValue( m_scriptEngine, filter );
    ScriptManager::instance()->m_script[serviceName].serviceContext.thisObject().property( "prototype" ).property( "populate" ).call( QScriptValue(), args );
	
    The::scriptableServiceManager()->donePopulating( name, parent_id );
*/
}

void ScriptableServiceScript::slotRequestInfo( QString name, int level, QString callbackData )
{
    DEBUG_BLOCK
    m_serviceName = name;
    emit( fetchInfo( level, callbackData ) );
}

void ScriptableServiceScript::slotCustomize( const QString &name )
{
    DEBUG_BLOCK
    m_serviceName = name;
    emit( customize() );
}


void ScriptableServiceScript::setIcon( const QPixmap &icon )
{
    The::scriptableServiceManager()->setIcon( m_serviceName, icon );
}

void ScriptableServiceScript::setEmblem( const QPixmap &emblem )
{
    The::scriptableServiceManager()->setEmblem( m_serviceName, emblem );
}

void ScriptableServiceScript::setScalableEmblem ( const QString& emblemPath )
{
    The::scriptableServiceManager()->setScalableEmblem( m_serviceName, emblemPath );
}

void ScriptableServiceScript::setCurrentInfo( const QString &infoHtml )
{
    The::scriptableServiceManager()->setCurrentInfo( m_serviceName, infoHtml );
}




#include "AmarokScriptableServiceScript.moc"
