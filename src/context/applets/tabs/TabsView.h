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
#include <QStandardItemModel>

class TabsItem;
class TabsTreeView;

class QModelIndex;
namespace Plasma
{
    class ScrollBar;
    class TextBrowser;
}

class TabsView : public QGraphicsProxyWidget
{
    Q_OBJECT

public:
    explicit TabsView( QGraphicsWidget *parent = nullptr );
    ~TabsView();

    void appendTab( TabsItem *tabsItem );
    void clear();
    void clearTabBrowser();

public Q_SLOTS:
    void showTab( TabsItem *tab );

protected:
    void resizeEvent( QGraphicsSceneResizeEvent *event );

private Q_SLOTS:
    void itemClicked( const QModelIndex &index );
    void slotScrollBarRangeChanged( int min, int max );

private:
    Plasma::TextBrowser *m_tabTextBrowser;
    TabsTreeView *m_treeView;
    QGraphicsProxyWidget *m_treeProxy;
    QStandardItemModel *m_model;

    void updateScrollBarVisibility();
    Plasma::ScrollBar *m_scrollBar;
};

#endif // multiple inclusion guard
