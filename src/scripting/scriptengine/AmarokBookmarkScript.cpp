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

#include "AmarokBookmarkScript.h"

#include "amarokconfig.h"
#include "amarokurls/AmarokUrl.h"
#include "amarokurls/BookmarkGroup.h"
#include "amarokurls/BookmarkModel.h"
#include "amarokurls/ContextUrlGenerator.h"
#include "amarokurls/NavigationUrlGenerator.h"
#include <amarokurls/PlayUrlGenerator.h>
#include "playlist/PlaylistViewUrlGenerator.h"
#include "scripting/scriptengine/ScriptingDefines.h"

#include <QScriptEngine>

Q_DECLARE_METATYPE( StringMap )

using namespace AmarokScript;

AmarokBookmarkScript::AmarokBookmarkScript( QScriptEngine *engine )
    : QObject( engine )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("Bookmark"), scriptObject );
    qScriptRegisterMetaType<BookmarkGroupPtr>( engine, toScriptValue<BookmarkGroupPtr, BookmarkGroupPrototype>, fromScriptValue<BookmarkGroupPtr, BookmarkGroupPrototype> );
    qScriptRegisterMetaType<AmarokUrlPtr>( engine, toScriptValue<AmarokUrlPtr, BookmarkPrototype>, fromScriptValue<AmarokUrlPtr, BookmarkPrototype> );
    engine->globalObject().setProperty( QStringLiteral("BookmarkGroup"), engine->newFunction( BookmarkGroupPrototype::bookmarkGroupCtor ) );
    engine->globalObject().setProperty( QStringLiteral("Bookmark"), engine->newFunction( BookmarkPrototype::bookmarkCtor ) );
    qScriptRegisterMetaType<BookmarkGroupList>( engine, toScriptArray, fromScriptArray );
    qScriptRegisterMetaType<BookmarkList>( engine, toScriptArray, fromScriptArray );
}

AmarokUrlPtr
AmarokBookmarkScript::contextView()
{
    return AmarokUrlPtr( new AmarokUrl( ContextUrlGenerator::instance()->createContextBookmark() ) );
}

AmarokUrlPtr
AmarokBookmarkScript::currentPlaylistView()
{
    return AmarokUrlPtr( new AmarokUrl( Playlist::ViewUrlGenerator::instance()->createUrl() ) );
}

AmarokUrlPtr
AmarokBookmarkScript::browserView()
{
    return AmarokUrlPtr( new AmarokUrl( NavigationUrlGenerator::instance()->CreateAmarokUrl() ) );
}

AmarokUrlPtr
AmarokBookmarkScript::createCurrentTrackBookmark()
{
    return AmarokUrlPtr( new AmarokUrl( PlayUrlGenerator::instance()->createCurrentTrackBookmark() ) );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// BookmarkGroupPrototype
//////////////////////////////////////////////////////////////////////////////////////////////////

QScriptValue
BookmarkGroupPrototype::bookmarkGroupCtor( QScriptContext *context, QScriptEngine *engine )
{
    BookmarkGroup *group = 0;
    switch( context->argumentCount() )
    {
        case 0:
            return context->throwError( QScriptContext::SyntaxError, QStringLiteral("Not enough arguments!!") );

        case 1:
            if( context->argument( 0 ).isString() )
                group = new BookmarkGroup( context->argument( 0 ).toString() );
            break;

        case 2:
            if( context->argument( 0 ).isString() )
            {
                QString name = context->argument( 0 ).toString();
                if( context->argument( 1 ).isString() )
                    group = new BookmarkGroup( name, context->argument( 1 ).toString() );
                else if( dynamic_cast<BookmarkGroupPrototype*>( context->argument(1).toQObject() ) )
                    group = new BookmarkGroup( name, dynamic_cast<BookmarkGroupPrototype*>( context->argument(1).toQObject() )->data() );
            }
            break;
    }
    if( !group )
        return context->throwError( QScriptContext::TypeError, QStringLiteral("Invalid arguments!") );
    return engine->newQObject( new BookmarkGroupPrototype( BookmarkGroupPtr( group ) ), QScriptEngine::ScriptOwnership, QScriptEngine::ExcludeSuperClassContents );
}

BookmarkGroupPrototype::BookmarkGroupPrototype( BookmarkGroupPtr group )
:QObject( nullptr )
, m_group( group )
{}

BookmarkList
BookmarkGroupPrototype::childBookmarks() const
{
    return m_group->childBookmarks();
}

BookmarkGroupList
BookmarkGroupPrototype::childGroups() const
{
    return m_group->childGroups();
}

void
BookmarkGroupPrototype::clear()
{
    m_group->clear();
}

void BookmarkGroupPrototype::deleteChildBookmark( AmarokUrlPtr bookmark )
{
    m_group->deleteChild( BookmarkViewItemPtr::staticCast( bookmark ) );
}

void
BookmarkGroupPrototype::deleteChildBookmarkgroup( BookmarkGroupPtr bookmarkGroup )
{
    m_group->deleteChild( BookmarkViewItemPtr::staticCast( bookmarkGroup ) );
}

void
BookmarkGroupPrototype::save()
{
    m_group->save();
    BookmarkModel::instance()->reloadFromDb();
}

// private
int
BookmarkGroupPrototype::childCount() const
{
    return m_group->childCount();
}

QString
BookmarkGroupPrototype::description() const
{
    return m_group->description();
}

int
BookmarkGroupPrototype::id() const
{
    return m_group->id();
}

QString
BookmarkGroupPrototype::name() const
{
    return m_group->name();
}

BookmarkGroupPtr
BookmarkGroupPrototype::parent() const
{
    return m_group->parent();
}

void
BookmarkGroupPrototype::setDescription( const QString &description )
{
    m_group->setDescription( description );
}

void
BookmarkGroupPrototype::setName( const QString &name )
{
    m_group->rename( name );
}

void
BookmarkGroupPrototype::setParent( BookmarkGroupPtr parent )
{
    m_group->reparent( parent );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// BookmarkPrototype
//////////////////////////////////////////////////////////////////////////////////////////////////

// public

// script invokable
bool
BookmarkPrototype::save()
{
    bool success( m_url->saveToDb() );
    if( success )
        BookmarkModel::instance()->reloadFromDb();
    return success;
}

QScriptValue
BookmarkPrototype::bookmarkCtor( QScriptContext *context, QScriptEngine *engine )
{
    AmarokUrlPtr url;
    switch( context->argumentCount() )
    {
        case 0:
            return context->throwError( QScriptContext::SyntaxError, QStringLiteral("Not enough arguments!!") );

        case 1:
            if( context->argument( 0 ).isString() )
                url = new AmarokUrl( context->argument( 0 ).toString() );
            break;

        case 2:
            if( context->argument( 0 ).isString() )
            {
                BookmarkGroupPrototype *proto = dynamic_cast<BookmarkGroupPrototype*>( context->argument(1).toQObject() );
                if( proto )
                    url = new AmarokUrl( context->argument( 0 ).toString(), proto->data() );
            }
            else
            {
                Meta::TrackPtr track = engine->fromScriptValue<Meta::TrackPtr>( context->argument( 0 ) );
                if( track && context->argument( 1 ).toVariant().canConvert( QVariant::LongLong ) )
                    url = new AmarokUrl( PlayUrlGenerator::instance()->createTrackBookmark( track, context->argument(1).toVariant().toLongLong() ) );
            }
            break;
    }
    if( !url )
        return context->throwError( QScriptContext::TypeError, QStringLiteral("Invalid arguments!") );

    return engine->newQObject( new BookmarkPrototype( url ),
                               QScriptEngine::ScriptOwnership,
                               QScriptEngine::ExcludeSuperClassContents );
}

BookmarkPrototype::BookmarkPrototype( AmarokUrlPtr bookmark )
: QObject( nullptr )
, m_url( bookmark )
{}

void
BookmarkPrototype::initFromString( const QString &urlString )
{
    m_url->initFromString( urlString );
}

StringMap
BookmarkPrototype::args() const
{
  return m_url->args();
}

void
BookmarkPrototype::setArg( const QString &name, const QString &value )
{
    m_url->setArg( name, value );
}

QString
BookmarkPrototype::prettyCommand() const
{
    return m_url->prettyCommand();
}

void
BookmarkPrototype::removeFromDb()
{
    m_url->removeFromDb();
}

bool
BookmarkPrototype::run()
{
    return m_url->run();
}

// private

QString
BookmarkPrototype::command() const
{
    return m_url->command();
}

void
BookmarkPrototype::setCommand( const QString &command )
{
    m_url->setCommand( command );
}

QString
BookmarkPrototype::description() const
{
    return m_url->description();
}

void
BookmarkPrototype::setDescription( const QString &description )
{
    m_url->setDescription( description );
}

int
BookmarkPrototype::id() const
{
    return m_url->id();
}

void
BookmarkPrototype::setId( int id )
{
    m_url->setId( id );
}

QString
BookmarkPrototype::customValue() const
{
    return m_url->customValue();
}

void
BookmarkPrototype::setCustomValue( const QString &custom )
{
    m_url->setCustomValue( custom );
}

bool
BookmarkPrototype::isNull() const
{
    return m_url->isNull();
}

QString
BookmarkPrototype::name() const
{
    return m_url->name();
}

void
BookmarkPrototype::setName( const QString &name )
{
    return m_url->setName( name );
}


BookmarkGroupPtr
BookmarkPrototype::parent() const
{
    return m_url->parent();
}

void
BookmarkPrototype::setParent( BookmarkGroupPtr parent )
{
    m_url->reparent( parent );
}

QString
BookmarkPrototype::path() const
{
    return m_url->path();
}

void
BookmarkPrototype::setPath( const QString &path )
{
    m_url->setPath( path );
}

QString
BookmarkPrototype::url() const
{
    return m_url->url();
}
