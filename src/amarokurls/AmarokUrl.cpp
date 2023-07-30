/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "AmarokUrl.h"

#include "AmarokUrlHandler.h"
#include "BookmarkGroup.h"
#include "core-impl/storage/StorageManager.h"
#include "core/support/Debug.h"
#include <core/storage/SqlStorage.h>

#include <QUrl>
#include <QUrlQuery>

AmarokUrl::AmarokUrl()
    : m_id( -1 )
    , m_parent( nullptr )
{}

AmarokUrl::AmarokUrl( const QString & urlString, const BookmarkGroupPtr &parent )
    : m_id( -1 )
    , m_parent( parent )
{
    initFromString( urlString );
}

AmarokUrl::AmarokUrl( const QStringList & resultRow, const BookmarkGroupPtr &parent )
    : m_parent( parent )
{
    m_id = resultRow[0].toInt();
    m_name = resultRow[2];
    const QString urlString = resultRow[3];
    m_description = resultRow[4];
    m_customValue = resultRow[5];

    initFromString( urlString );
}

AmarokUrl::~AmarokUrl()
{}

void AmarokUrl::initFromString( const QString & urlString )
{
    //first, strip amarok://
    QString strippedUrlString = urlString;
    strippedUrlString = strippedUrlString.replace( QLatin1String("amarok://"), QLatin1String("") );

    //separate path from arguments
    QStringList parts = strippedUrlString.split( QLatin1Char('?') );

    QString commandAndPath = parts.at( 0 );

    QString argumentsString;
    if ( parts.size() == 2 )
        argumentsString = parts.at( 1 );

    if ( !argumentsString.isEmpty() )
    {
        parts = argumentsString.split( '&' );
        
        foreach( const QString &argument, parts )
        {
            
            QStringList argParts = argument.split( '=' );
            debug() << "argument: " << argument << " unescaped: " << unescape( argParts.at( 1 ) );
            setArg( argParts.at( 0 ), unescape( argParts.at( 1 ) ) );
        }
    }

    //get the command

    parts = commandAndPath.split( QLatin1Char('/') );
    m_command = parts.takeFirst();

    m_path = parts.join( QLatin1Char('/') );

    m_path = unescape( m_path );
}

void AmarokUrl::setCommand( const QString & command )
{
    m_command = command;
}

QString AmarokUrl::command() const
{
        return m_command;
}

QString
AmarokUrl::prettyCommand() const
{
    return The::amarokUrlHandler()->prettyCommand( command() );
}

QMap<QString, QString> AmarokUrl::args() const
{
    return m_arguments;
}

void AmarokUrl::setArg( const QString &name, const QString &value )
{
    m_arguments.insert( name, value );
}

bool AmarokUrl::run()
{
    DEBUG_BLOCK
    return The::amarokUrlHandler()->run( *this );
}

QString AmarokUrl::url() const
{
    QUrl url;
    url.setScheme( QStringLiteral("amarok") );
    url.setHost( m_command );
    url.setPath( '/' + m_path ); // the path must begin by /
    QUrlQuery query;

    foreach( const QString &argName, m_arguments.keys() )
        query.addQueryItem( argName, m_arguments[argName] );

    url.setQuery( query );
    return url.toEncoded();
}

bool AmarokUrl::saveToDb()
{
    DEBUG_BLOCK

    if ( isNull() )
        return false;

    const int parentId = m_parent ? m_parent->id() : -1;

    auto sql =  StorageManager::instance()->sqlStorage();

    if( m_id != -1 )
    {
        //update existing
        debug() << "Updating bookmark";
        QString query = QStringLiteral("UPDATE bookmarks SET parent_id=%1, name='%2', url='%3', description='%4', custom='%5' WHERE id=%6;");
        query = query.arg( QString::number( parentId ), sql->escape( m_name ), sql->escape( url() ),
                           sql->escape( m_description ), sql->escape( m_customValue ), QString::number( m_id ) );
        StorageManager::instance()->sqlStorage()->query( query );
    }
    else
    {
        //insert new
        debug() << "Creating new bookmark in the db";
        QString query = QStringLiteral("INSERT INTO bookmarks ( parent_id, name, url, description, custom ) VALUES ( %1, '%2', '%3', '%4', '%5' );");
        query = query.arg( QString::number( parentId ), sql->escape( m_name ), sql->escape( url() ),
                           sql->escape( m_description ), sql->escape( m_customValue ) );
        m_id = StorageManager::instance()->sqlStorage()->insert( query, nullptr );
    }

    return true;
}

void AmarokUrl::setName( const QString & name )
{
    m_name = name;
}

QString AmarokUrl::name() const
{
    return m_name;
}

void AmarokUrl::setDescription( const QString & description )
{
    m_description = description;
}

QString AmarokUrl::description() const
{
    return m_description;
}

void AmarokUrl::removeFromDb()
{
    QString query = QStringLiteral("DELETE FROM bookmarks WHERE id=%1");
    query = query.arg( QString::number( m_id ) );
    StorageManager::instance()->sqlStorage()->query( query );
}

void AmarokUrl::rename( const QString &name )
{
    m_name = name;
    if ( m_id != -1 )
        saveToDb();
}

void AmarokUrl::reparent( const BookmarkGroupPtr &parent )
{
    m_parent = parent;
    saveToDb();
}

void AmarokUrl::setCustomValue( const QString & custom )
{
    m_customValue = custom;
}

QString AmarokUrl::customValue() const
{
    return m_customValue;
}

QString AmarokUrl::escape( const QString & in )
{
    return QUrl::toPercentEncoding( in.toUtf8() );
}

QString AmarokUrl::unescape( const QString & in )
{
    return QUrl::fromPercentEncoding( in.toUtf8() );
}

bool AmarokUrl::isNull() const
{
    return m_command.isEmpty();
}

QString AmarokUrl::path() const
{
    return m_path;
}

void AmarokUrl::setPath( const QString &path )
{
    m_path = path;
}



