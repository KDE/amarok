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

#ifndef PLASMA_TREEVIEW_H
#define PLASMA_TREEVIEW_H

#include <QtGui/QGraphicsProxyWidget>

#include <plasma/plasma_export.h>

class QTreeView;
class QAbstractItemModel;

namespace Plasma
{

class TreeViewPrivate;

/**
 * @class TreeView plasma/widgets/treeview.h <Plasma/Widgets/TreeView>
 *
 * @short Provides a plasma-themed QTreeView.
 */
class PLASMA_EXPORT TreeView : public QGraphicsProxyWidget
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel)
    Q_PROPERTY(QGraphicsWidget *parentWidget READ parentWidget)
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
    Q_PROPERTY(QTreeView *nativeWidget READ nativeWidget)

public:
    explicit TreeView(QGraphicsWidget *parent = 0);
    ~TreeView();

    /**
     * Sets a model for this weather view
     *
     * @arg model the model to display
     */
    void setModel(QAbstractItemModel *model);

    /**
     * @return the model shown by this view
     */
    QAbstractItemModel *model();

    /**
     * Sets the stylesheet used to control the visual display of this TreeView
     *
     * @arg stylesheet a CSS string
     */
    void setStyleSheet(const QString &stylesheet);

    /**
     * @return the stylesheet currently used with this widget
     */
    QString styleSheet();

    /**
     * @return the native widget wrapped by this TreeView
     */
    QTreeView *nativeWidget() const;

private:
    TreeViewPrivate *const d;
};

}
#endif // multiple inclusion guard
