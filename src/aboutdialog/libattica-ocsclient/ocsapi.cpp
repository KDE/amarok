/*
    This file is part of KDE.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "ocsapi.h"

#include <KDebug>

using namespace Attica;

OcsApi::OcsApi()
{
}

PersonJob *OcsApi::requestPerson( const QString &id )
{
  KUrl url = createUrl( "person/data/" + id );
  return doRequestPerson( url );
}

PersonJob *OcsApi::requestPersonSelf()
{
  KUrl url = createUrl( "person/self" );
  return doRequestPerson( url );
}

PersonListJob *OcsApi::requestPersonSearchByName( const QString &name )
{
  KUrl url = createUrl( "person/data");
  url.addQueryItem("name", name);
  return doRequestPersonList( url );
}

PersonListJob *OcsApi::requestPersonSearchByLocation( qreal latitude, qreal longitude, qreal distance, const int page, const int pageSize)
{
  KUrl url = createUrl( "person/data" );
  url.addQueryItem("latitude", QString::number(latitude));
  url.addQueryItem("longitude", QString::number(longitude));
  url.addQueryItem("distance", QString::number(distance));
  url.addQueryItem("page", QString::number(page));
  url.addQueryItem("pagesize", QString::number(pageSize));
  
  qDebug() << "Location-based search:" << latitude << longitude << distance;
  qDebug() << "URL:" << url;
  return doRequestPersonList( url );
}

PersonListJob *OcsApi::requestFriend( const QString &id, const int page, const int pageSize )
{
  KUrl url = createUrl( "friend/data/" + id );
  url.addQueryItem("page", QString::number(page));
  url.addQueryItem("pagesize", QString::number(pageSize));
  kDebug() << "URL:" << url;
  return doRequestPersonList( url );
}

ActivityListJob *OcsApi::requestActivity()
{
  KUrl url = createUrl( "activity" );
  return doRequestActivityList( url );
}

PostJob *OcsApi::postActivity( const QString &message )
{
  PostJob *job = new PostJob();

  KUrl url = createUrl( "activity" );
  job->setUrl( url );
  job->setData( "message", message );
  
  job->start();
  return job;
}

PostJob *OcsApi::postInvitation( const QString &to, const QString &message )
{
  PostJob *job = new PostJob();

  KUrl url = createUrl( "friend/outbox/" + to );
  job->setUrl( url );
  job->setData( "message", message );

  job->start();
  return job;  
}

PostJob *OcsApi::postLocation( qreal latitude, qreal longitude, const QString &city, const QString &country )
{
  PostJob *job = new PostJob();
  
  KUrl url = createUrl( "person/self" );
  
  job->setUrl( url ); 

  job->setData( "latitude", QString("%1").arg(latitude) );
  job->setData( "longitude", QString("%1").arg(longitude) );
  job->setData( "city", city );
  job->setData( "country", country );
  
  job->start();
  return job;
}


FolderListJob *OcsApi::requestFolders()
{
  return doRequestFolderList( createUrl( "message" ) );
}

MessageListJob *OcsApi::requestMessages( const QString &folderId )
{
  return doRequestMessageList( createUrl( "message/" + folderId ) );
}

PostJob *OcsApi::postMessage( const Message &message )
{
  PostJob *job = new PostJob();
  
  KUrl url = createUrl( "message/2" );
  job->setUrl( url );
  job->setData( "message", message.body() );
  job->setData( "subject", message.subject() );
  job->setData( "to", message.to() );
  
  job->start();
  return job;
}

CategoryListJob *OcsApi::requestCategories()
{
  CategoryListJob *job = new CategoryListJob();
  
  KUrl url = createUrl( "content/categories" );
  job->setUrl( url );
  
  job->start();
  return job;
}

ContentListJob *OcsApi::requestContent( const Category::List &categories,
  const QString &search, SortMode sortMode )
{
  ContentListJob *job = new ContentListJob();
  
  KUrl url = createUrl( "content/data" );

  QStringList categoryIds;
  foreach( const Category &category, categories ) {
    categoryIds.append( category.id() );
  }
  url.addQueryItem( "categories", categoryIds.join( "x" ) );
  
  url.addQueryItem( "search", search );
  QString sortModeString;
  switch ( sortMode ) {
    case Newest:
      sortModeString = "new";
      break;
    case Alphabetical:
      sortModeString = "alpha";
      break;
    case Rating:
      sortModeString = "high";
      break;
    case Downloads:
      sortModeString = "down";
      break;
  }
  if ( !sortModeString.isEmpty() ) {
    url.addQueryItem( "sortmode", sortModeString );
  }

  job->setUrl( url );
  
  job->start();
  return job;
}

ContentJob *OcsApi::requestContent( const QString &id )
{
  ContentJob *job = new ContentJob();
  
  KUrl url = createUrl( "content/data/" + id );
  job->setUrl( url );
  
  job->start();
  return job;
}

KnowledgeBaseJob *OcsApi::requestKnowledgeBase( const QString &id )
{
  KnowledgeBaseJob *job = new KnowledgeBaseJob();

  KUrl url = createUrl( "knowledgebase/data/" + id );
  job->setUrl( url );

  job->start();
  return job;
}

KnowledgeBaseListJob *OcsApi::requestKnowledgeBase( const int content, const QString &search, SortMode sortMode, const int page, const int pageSize )
{
  KnowledgeBaseListJob *job = new KnowledgeBaseListJob();

  KUrl url = createUrl( "knowledgebase/data" );

  if (content) {
      url.addQueryItem("content", QString::number(content));
  }

  url.addQueryItem( "search", search );
  QString sortModeString;
  switch ( sortMode ) {
    case Newest:
      sortModeString = "new";
      break;
    case Alphabetical:
      sortModeString = "alpha";
      break;
    case Rating:
      sortModeString = "high";
      break;
    //FIXME: knowledge base doesn't have downloads
    case Downloads:
      sortModeString = "new";
      break;
  }
  if ( !sortModeString.isEmpty() ) {
    url.addQueryItem( "sortmode", sortModeString );
  }

  url.addQueryItem( "page", QString::number(page) );
  url.addQueryItem( "pagesize", QString::number(pageSize) );

  job->setUrl( url );

  job->start();
  return job;
}

KUrl OcsApi::createUrl( const QString &path )
{
  KUrl url( "https://api.opendesktop.org/v1/" );
  url.addPath( path );
  return url;
}

PersonJob *OcsApi::doRequestPerson( const KUrl &url )
{
  PersonJob *job = new PersonJob();

  job->setUrl( url );

  job->start();
  return job;
}

PersonListJob *OcsApi::doRequestPersonList( const KUrl &url )
{
  PersonListJob *job = new PersonListJob();

  job->setUrl( url );

  job->start();
  return job;
}

ActivityListJob *OcsApi::doRequestActivityList( const KUrl &url )
{
  ActivityListJob *job = new ActivityListJob();

  job->setUrl( url );

  job->start();
  return job;
}

FolderListJob *OcsApi::doRequestFolderList( const KUrl &url )
{
  FolderListJob *job = new FolderListJob();
  
  job->setUrl( url );
  job->start();
  return job;
}

MessageListJob *OcsApi::doRequestMessageList( const KUrl &url )
{
  MessageListJob *job = new MessageListJob();
  
  job->setUrl( url );
  job->start();
  return job;
}
