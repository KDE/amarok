/*
 *   Copyright 2008 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "treeview.h"

#include <QTreeView>
#include <QHeaderView>
#include <QScrollBar>

#include <KIconLoader>

#include "private/style.h"

namespace Plasma
{

class TreeViewPrivate
{
public:
    TreeViewPrivate()
    {
    }

    ~TreeViewPrivate()
    {
    }
};

TreeView::TreeView(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new TreeViewPrivate)
{
    QTreeView *native = new QTreeView;
    setWidget(native);
    native->setAttribute(Qt::WA_NoSystemBackground);
    native->setFrameStyle(QFrame::NoFrame);

    Plasma::Style *style = new Plasma::Style();
    native->verticalScrollBar()->setStyle(style);
    native->horizontalScrollBar()->setStyle(style);
}

TreeView::~TreeView()
{
    delete d;
}

void TreeView::setModel(QAbstractItemModel *model)
{
    nativeWidget()->setModel(model);
}

QAbstractItemModel *TreeView::model()
{
    return nativeWidget()->model();
}

void TreeView::setStyleSheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString TreeView::styleSheet()
{
    return widget()->styleSheet();
}

QTreeView *TreeView::nativeWidget() const
{
    return static_cast<QTreeView*>(widget());
}

}

#include <treeview.moc>

