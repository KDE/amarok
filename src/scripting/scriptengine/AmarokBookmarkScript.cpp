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

#include <QJSEngine>

Q_DECLARE_METATYPE( StringMap )

using namespace AmarokScript;

AmarokBookmarkScript::AmarokBookmarkScript( QJSEngine *engine )
    : QObject( engine )
    , m_engine( engine )
{
    QJSValue scriptObject = m_engine->newQObject( this );
    m_engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("Bookmark"), scriptObject );

    qRegisterMetaType<BookmarkGroupPtr>();
    QMetaType::registerConverter<BookmarkGroupPtr,QJSValue>( [=] (BookmarkGroupPtr bGroup) { return toScriptValue<BookmarkGroupPtr, BookmarkGroupPrototype>( m_engine, bGroup ); } );
    QMetaType::registerConverter<QJSValue,BookmarkGroupPtr>( [] (QJSValue jsValue) {
            BookmarkGroupPtr bGroup;
            fromScriptValue<BookmarkGroupPtr, BookmarkGroupPrototype>( jsValue, bGroup );
            return bGroup;
        } );

    qRegisterMetaType<AmarokUrlPtr>();
    QMetaType::registerConverter<AmarokUrlPtr, QJSValue>( [=] (AmarokUrlPtr url) { return toScriptValue<AmarokUrlPtr, BookmarkPrototype>( m_engine, url ); } );
    QMetaType::registerConverter<QJSValue, AmarokUrlPtr>( [] (QJSValue jsValue) {
        AmarokUrlPtr url;
        fromScriptValue<AmarokUrlPtr, BookmarkPrototype>( jsValue, url );
        return url;
    } );

    QJSValue bookmarkGroupCtor = scriptObject.property("bookmarkGroupCtorWrapper");
    m_engine->globalObject().setProperty( QStringLiteral("BookmarkGroup"), bookmarkGroupCtor );

    QJSValue bookmarkCtor = scriptObject.property("bookmarkCtorWrapper");
    m_engine->globalObject().setProperty( QStringLiteral("Bookmark"), bookmarkCtor );

    qRegisterMetaType<BookmarkGroupList>();
    QMetaType::registerConverter<BookmarkGroupList, QJSValue>( [=] (BookmarkGroupList bgList) { return toScriptArray<BookmarkGroupList>( m_engine, bgList ); } );
    QMetaType::registerConverter<QJSValue, BookmarkGroupList>( [] (QJSValue jsValue) {
        BookmarkGroupList bgList;
        fromScriptArray<BookmarkGroupList>( jsValue, bgList );
        return bgList;
    } );

    qRegisterMetaType<BookmarkList>();
    QMetaType::registerConverter<BookmarkList,QJSValue>( [=] (BookmarkList bList) { return toScriptArray<BookmarkList>( m_engine, bList); } );
    QMetaType::registerConverter<QJSValue,BookmarkList>( [] (QJSValue jsValue) {
        BookmarkList bList;
        fromScriptArray<BookmarkList>( jsValue, bList );
        return bList;
    } );
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

QJSValue
AmarokBookmarkScript::bookmarkCtorWrapper( QJSValueList arguments )
{
    return BookmarkPrototype::bookmarkCtor( arguments, m_engine );
}

QJSValue
AmarokBookmarkScript::bookmarkGroupCtorWrapper( QJSValueList arguments )
{
    return BookmarkGroupPrototype::bookmarkGroupCtor( arguments, m_engine );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// BookmarkGroupPrototype
//////////////////////////////////////////////////////////////////////////////////////////////////

QJSValue
BookmarkGroupPrototype::bookmarkGroupCtor( QJSValueList arguments, QJSEngine *engine )
{
    BookmarkGroup *group = 0;
    switch( arguments.size() )
    {
        case 0:
            engine->throwError( QJSValue::SyntaxError, QStringLiteral("Not enough arguments!!") );
            return engine->newErrorObject( QJSValue::SyntaxError, QStringLiteral("Not enough arguments!!") );

        case 1:
            if( arguments.at( 0 ).isString() )
                group = new BookmarkGroup( arguments.at( 0 ).toString() );
            break;

        case 2:
            if( arguments.at( 0 ).isString() )
            {
                QString name = arguments.at( 0 ).toString();
                if( arguments.at( 1 ).isString() )
                    group = new BookmarkGroup( name, arguments.at( 1 ).toString() );
                else if( dynamic_cast<BookmarkGroupPrototype*>( arguments.at(1).toQObject() ) )
                    group = new BookmarkGroup( name, dynamic_cast<BookmarkGroupPrototype*>( arguments.at(1).toQObject() )->data() );
            }
            break;
    }
    if( !group ) {
        engine->throwError( QJSValue::TypeError, QStringLiteral("Invalid arguments!") );
        return engine->newErrorObject( QJSValue::TypeError,  QStringLiteral("Invalid arguments!") );
    }

    return engine->newQObject( new BookmarkGroupPrototype( BookmarkGroupPtr( group ) ) );
}

BookmarkGroupPrototype::BookmarkGroupPrototype( const BookmarkGroupPtr &group )
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

void BookmarkGroupPrototype::deleteChildBookmark( const AmarokUrlPtr &bookmark )
{
    m_group->deleteChild( BookmarkViewItemPtr::staticCast( bookmark ) );
}

void
BookmarkGroupPrototype::deleteChildBookmarkgroup( const BookmarkGroupPtr &bookmarkGroup )
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
BookmarkGroupPrototype::setParent( const BookmarkGroupPtr &parent )
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

QJSValue
BookmarkPrototype::bookmarkCtor( QJSValueList arguments, QJSEngine *engine )
{
    AmarokUrlPtr url;
    switch( arguments.size() )
    {
        case 0:
            engine->throwError( QJSValue::SyntaxError, QStringLiteral("Not enough arguments!!") );
            return engine->newErrorObject( QJSValue::SyntaxError, QStringLiteral("Not enough arguments!!") );

        case 1:
            if( arguments.at( 0 ).isString() )
                url = new AmarokUrl( arguments.at( 0 ).toString() );
            break;

        case 2:
            if( arguments.at( 0 ).isString() )
            {
                BookmarkGroupPrototype *proto = dynamic_cast<BookmarkGroupPrototype*>( arguments.at(1).toQObject() );
                if( proto )
                    url = new AmarokUrl( arguments.at( 0 ).toString(), proto->data() );
            }
            else
            {
                Meta::TrackPtr track = engine->fromScriptValue<Meta::TrackPtr>( arguments.at( 0 ) );
                if( track && arguments.at( 1 ).toVariant().canConvert( QVariant::LongLong ) )
                    url = new AmarokUrl( PlayUrlGenerator::instance()->createTrackBookmark( track, arguments.at(1).toVariant().toLongLong() ) );
            }
            break;
    }
    if( !url ) {
        engine->throwError( QJSValue::TypeError, QStringLiteral("Invalid arguments!") );
        return engine->newErrorObject( QJSValue::TypeError,  QStringLiteral("Invalid arguments!") );
    }

    return engine->newQObject( new BookmarkPrototype( url ) );
}

BookmarkPrototype::BookmarkPrototype( const AmarokUrlPtr &bookmark )
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
    m_url->setName( name );
}


BookmarkGroupPtr
BookmarkPrototype::parent() const
{
    return m_url->parent();
}

void
BookmarkPrototype::setParent( const BookmarkGroupPtr &parent )
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
