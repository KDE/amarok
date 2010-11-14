/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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

#ifndef AMAROK_TabsVIEW_H
#define AMAROK_TabsVIEW_H

#include <QGraphicsProxyWidget>
#include <Plasma/TextBrowser>

#include "core/meta/Meta.h"

class TabsItem;
class TabsView;
class TabsTreeView;

class QTreeView;
class QAbstractItemModel;
class QModelIndex;

class TabsView : public QGraphicsProxyWidget
{
    Q_OBJECT

public:
    explicit TabsView( QGraphicsWidget *parent = 0 );
    ~TabsView() { };

    /**
     * Sets a model for this view
     *
     * @arg the model to display
     */
    void setModel( QAbstractItemModel *model );

    /**
     * @return the model shown by this view
     */
    QAbstractItemModel *model();

    /**
     * clears the content of the tab textbrowser
     */
    void clearTabBrowser();

    /**
     * @return the native widget wrapped by this TabsView
     */
    QTreeView* tabsListView() const;

public slots:
    /**
    * \param tab :
    */
    void showTab( TabsItem *tab );

protected:
    void resizeEvent( QGraphicsSceneResizeEvent *event );

private slots:
    void itemClicked( const QModelIndex &index );

private:
    Plasma::TextBrowser *m_tabTextBrowser;
    TabsTreeView *m_treeView;
    QGraphicsProxyWidget *m_treeProxy;
};

#endif // multiple inclusion guard
