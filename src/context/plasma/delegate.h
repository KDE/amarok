/*
    Copyright 2007 Robert Knight <robertknight@gmail.com>
    Copyright 2008 Marco Martin <notmart@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef PLASMA_DELEGATE_H
#define PLASMA_DELEGATE_H

// Qt
#include <QtGui/QAbstractItemDelegate>

// Plasma
#include <plasma/plasma_export.h>

namespace Plasma
{

class DelegatePrivate;

/**
 * @class Delegate plasma/delegate.h <Plasma/Delegate>
 *
 * Item delegate for rendering items in Plasma menus implemented with item views.
 *
 * The delegate makes use of its own data roles that are:
 * SubTitleRole: the text of the subtitle
 * SubTitleMandatoryRole: if the subtitle is to always be displayed
 *   (as default the subtitle is displayed only on mouse over)
 * ColumnTypeRole: if the column is a main column (with title and subtitle)
 * or a secondary action column (only a little icon that appears on mouse
 * over is displayed)
 */
class PLASMA_EXPORT Delegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:

    enum SpecificRoles {
        SubTitleRole = Qt::UserRole + 1,
        SubTitleMandatoryRole = Qt::UserRole + 2,
        ColumnTypeRole = Qt::UserRole + 3
    };

    enum ColumnType {
        MainColumn = 1,
        SecondaryActionColumn = 2
    };

    Delegate(QObject *parent = 0);
    ~Delegate();

    /**
     * Maps an arbitrary role to a role belonging to SpecificRoles.
     * Using this function you can use any model with this delegate.
     *
     * @param role a role belonging to SpecificRoles
     * @param actual an arbitrary role of the model we are using
     */
    void setRoleMapping(SpecificRoles role, int actual);

    int roleMapping(SpecificRoles role) const;

    //Reimplemented
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

protected:
    /**
     * Returns the empty area after the title.
     * The height is the height of the subtitle.
     * It can be used by subclasses that wants to paint additional data after
     * calling the paint function of the superclass.
     *
     * @param option options for the title text
     * @param index model index that we want to compute the free area
     */
    QRect rectAfterTitle(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    /**
     * Returns the empty area after the subtitle.
     * The height is the height of the subtitle.
     * It can be used by subclasses, that wants to paint additional data.
     *
     * @param option options for the subtitle text
     * @param index model index that we want to compute the free area
     */
    QRect rectAfterSubTitle(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    /**
     * Returns the empty area after both the title and the subtitle.
     * The height is the height of the item.
     * It can be used by subclasses that wants to paint additional data
     *
     * @param option options for the title and subtitle text
     * @param index model index that we want to compute the free area
     */
    QRect emptyRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    DelegatePrivate *const d;
};

}

#endif // PLASMA_DELEGATE_H
