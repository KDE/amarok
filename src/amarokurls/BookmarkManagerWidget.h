/****************************************************************************************
 * Copyright (c) 2008, 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>              *
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
#include "BookmarkTreeView.h"
#include "LineEdit.h"

#include <KVBox>

#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QToolBar>


/**
A widget for managing amarok:// bookmark urls

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT BookmarkManagerWidget : public KVBox
{
    Q_OBJECT
public:
    BookmarkManagerWidget( QWidget * parent = 0);
    ~BookmarkManagerWidget();

    BookmarkTreeView * treeView();
protected slots:
    
     /**
     * Notify widget that the text in the search edit has changed.
     * @param filter The new text in the search widget.
     */
    void slotFilterChanged( const QString &filter );
    
private:

    QToolBar * m_toolBar;
    Amarok::LineEdit * m_searchEdit; 
    BookmarkTreeView * m_bookmarkView;

    int m_currentBookmarkId;
    QString m_lastFilter;

    QSortFilterProxyModel * m_proxyModel;

};

#endif
