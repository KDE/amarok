/****************************************************************************************
 * Copyright (c) 2004-2008 Trolltech ASA <copyright@trolltech.com>                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or any    *
 * later version publicly approved by Trolltech ASA (or its successor, if any) and the  *
 * KDE Free Qt Foundation.                                                              *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                                      *
 * In addition, Trolltech gives you certain additional rights as described in the       *
 * Trolltech GPL Exception version 1.2 which can be found at                            *
 * http://www.trolltech.com/products/qt/gplexception/                                   *
 ****************************************************************************************/

#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H


#include "amarok_export.h"

#include <QLayout>
#include <QRect>
#include <QWidgetItem>



 class AMAROK_EXPORT FlowLayout : public QLayout
{
    public:
        explicit FlowLayout(QWidget *parent, int margin = 0, int spacing = -1);
        FlowLayout(int spacing = -1);
        ~FlowLayout();

        void addItem(QLayoutItem *item);
        Qt::Orientations expandingDirections() const;
        bool hasHeightForWidth() const;
        int heightForWidth(int) const;
        int count() const;
        QLayoutItem *itemAt(int index) const;
        QSize minimumSize() const;
        void setGeometry(const QRect &rect);
        QSize sizeHint() const;
        QLayoutItem *takeAt(int index);

    private:
        int doLayout(const QRect &rect, bool testOnly) const;

        QList<QLayoutItem *> itemList;
};

#endif
