/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
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

#ifndef GPODDERTREEITEM_H_
#define GPODDERTREEITEM_H_

#include <mygpo-qt5/ApiRequest.h>
#include <mygpo-qt5/TagList.h>

#include <QList>
#include <QModelIndex>
#include <QVariant>


class GpodderTreeItem : public QObject
{
    Q_OBJECT
public:
    explicit GpodderTreeItem( GpodderTreeItem *parent = nullptr, const QString &name = QString() );
    ~GpodderTreeItem() override;

    void appendChild( GpodderTreeItem *child );

    GpodderTreeItem *child( int row );
    int childCount() const;
    void setHasChildren( bool hasChildren );
    bool hasChildren() const;

    GpodderTreeItem *parent() const;
    bool isRoot() const;


    virtual QVariant displayData() const;

    virtual void appendTags( mygpo::TagListPtr tags );
    virtual void appendPodcasts( mygpo::PodcastListPtr podcasts );
private:
    QList<GpodderTreeItem *> m_childItems;
    GpodderTreeItem *m_parentItem;
    QString m_name;
    bool m_hasChildren;
};

#endif /* GPODDERTREEITEM_H_ */
