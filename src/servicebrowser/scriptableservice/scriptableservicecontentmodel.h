/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 


#ifndef AMAROKSCRIPTABLESERVICECONTENTMODEL_H
#define AMAROKSCRIPTABLESERVICECONTENTMODEL_H

#include "scriptableservicecontentitem.h"
#include "../servicemodelbase.h"
#include <QMap>
#include <QModelIndex>
#include <QVariant>


class ScriptableServiceContentModel : public ServiceModelBase
{
    Q_OBJECT

private:

    mutable QMap<int, ScriptableServiceContentItem *> m_contentItemMap;  // the script refers to each node using an id number
    mutable int m_contentIndex;

    ScriptableServiceContentItem *m_rootContentItem;
    QString m_header;
    mutable bool m_populatingDynamicItem;
    mutable int m_indexBeingUpdated;
    mutable int m_updateCount;

    void triggerUpdateScript(const QString &script, const QString &argument, int nodeId) const;


public:

   

    ScriptableServiceContentModel(QObject *parent, const QString &header);
    ~ScriptableServiceContentModel();
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;

    bool canFetchMore( const QModelIndex & parent ) const;
    void fetchMore ( const QModelIndex & parent );

    int insertItem( const QString &name, const QString &url, const QString &infoHtml, int parentId );
    int insertDynamicItem( const QString &name, const QString &callbackScript, const QString &callbackArgument, const QString &infoHtml, int parentId );

    void requestHtmlInfo ( const QModelIndex & item ) const;

    void resetModel();

signals:

    void infoChanged( QString infoHtml ) const;


 };

 #endif
