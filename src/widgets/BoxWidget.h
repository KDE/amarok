/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef BOXWIDGET_H
#define BOXWIDGET_H

#include "amarok_export.h"

#include <QFrame>

class QBoxLayout;

/**
 * A Simple QFrame subclass that automatically adds new children to its layout
 */
class AMAROK_EXPORT BoxWidget : public QFrame
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param vertical defines if this BoxWidget has a vertical Layout.
     * Has a horizontal if false.
     * @param parent the parent widget.
     */
    explicit BoxWidget( bool vertical = true, QWidget *parent = Q_NULLPTR );
    virtual ~BoxWidget() {}

    QBoxLayout* layout() const;

protected:
    virtual void childEvent(QChildEvent* event) override;
};

#endif // BOXWIDGET_H
