/****************************************************************************************
 * Copyright (c) 2008, 2009 Nikolaj Hald Nielsen <nhn@kde.org>                          *
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

#ifndef BOOKMARKMANAGERWIDGET_H
#define BOOKMARKMANAGERWIDGET_H

#include "amarok_export.h"
#include "amarokurls/BookmarkTreeView.h"
#include "widgets/BoxWidget.h"
#include "widgets/LineEdit.h"

#include <QSortFilterProxyModel>
#include <QToolBar>


/**
A widget for managing amarok:// bookmark urls

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT BookmarkManagerWidget : public BoxWidget
{
    Q_OBJECT
public:
    explicit BookmarkManagerWidget( QWidget *parent = nullptr );
    ~BookmarkManagerWidget() override;

    BookmarkTreeView * treeView();

private:

    QToolBar * m_toolBar;
    Amarok::LineEdit * m_searchEdit;
    BookmarkTreeView * m_bookmarkView;

    int m_currentBookmarkId;
    QString m_lastFilter;

    QSortFilterProxyModel * m_proxyModel;

};

#endif
