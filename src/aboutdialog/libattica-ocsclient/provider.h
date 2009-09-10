/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

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

#ifndef ATTICA_PROVIDER_H
#define ATTICA_PROVIDER_H

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QString>

#include "atticaclient_export.h"
#include "category.h"

class KUrl;
class QDate;

namespace Attica {

class ActivityListJob;
class CategoryListJob;
class ContentJob;
class ContentListJob;
class EventJob;
class EventListJob;
class FolderListJob;
class KnowledgeBaseJob;
class KnowledgeBaseListJob;
class Message;
class MessageListJob;
class PersonJob;
class PersonListJob;
class PostJob;
class ProviderInitJob;

/**
  Open Collaboration Services API.
*/
class ATTICA_EXPORT Provider
{
  public:
    Provider();
    Provider(const Provider& other);
    Provider(const QString& id, const KUrl& baseUrl, const QString& name);
    Provider& operator=(const Provider& other);
    ~Provider();
    
    QString name() const;
    QString id() const;

    enum SortMode {
        Newest,
        Alphabetical,
        Rating,
        Downloads
    };

    static ProviderInitJob* byId(const QString& id);

    // Person part of OCS

    PersonJob* requestPerson(const QString& id) const;
    PersonJob* requestPersonSelf();
    PersonListJob* requestPersonSearchByName(const QString& name);
    PersonListJob* requestPersonSearchByLocation(qreal latitude, qreal longitude, qreal distance, int page = 0, int pageSize = 100);
    PostJob* postLocation(qreal latitude, qreal longitude, const QString& city = QString(), const QString& country = QString());

    // Friend part of OCS

    PersonListJob* requestFriend(const QString& id, int page = 0, int pageSize = 100);
    PostJob* postInvitation(const QString& to, const QString& message);

    // Message part of OCS

    FolderListJob* requestFolders();
    MessageListJob* requestMessages(const QString& folderId);
    PostJob* postMessage(const Message& message);

    // Activity part of OCS

    ActivityListJob* requestActivity();
    PostJob* postActivity(const QString& message);

    // Content part of OCS

    CategoryListJob* requestCategories();
    ContentListJob* requestContent(const Category::List& categories, const QString& search, SortMode mode);
    ContentJob* requestContent(const QString& id);

    // KnowledgeBase part of OCS

    KnowledgeBaseJob* requestKnowledgeBase(const QString& id);
    KnowledgeBaseListJob* requestKnowledgeBase(int content, const QString& search, SortMode, int page, int pageSize);

    // Event part of OCS

    EventJob* requestEvent(const QString& id);
    EventListJob* requestEvent(const QString& country, const QString& search, const QDate& startAt, SortMode mode, int page, int pageSize);

  protected:
    KUrl createUrl(const QString& path) const;
  
    PersonJob* doRequestPerson(const KUrl& url) const;
    PersonListJob* doRequestPersonList(const KUrl& url);
    ActivityListJob* doRequestActivityList(const KUrl& url);
    FolderListJob* doRequestFolderList(const KUrl& url);
    MessageListJob* doRequestMessageList(const KUrl& url);

  private:
    class Private;
    QExplicitlySharedDataPointer<Private> d;
};

}

#endif
