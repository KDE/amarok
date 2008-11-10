/*
 * Copyright 2008 by Rob Scheepmaker <r.scheepmaker@student.utwente.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "extenderapplet_p.h"

#include "../extender.h"
#include "../extenderitem.h"

#include <QGraphicsLinearLayout>

ExtenderApplet::ExtenderApplet(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

ExtenderApplet::~ExtenderApplet()
{
}

void ExtenderApplet::init()
{
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(this);
    layout->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    qreal left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    extender()->setExtenderAppearance(Plasma::Extender::NoBorders);
    extender()->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    connect(extender(), SIGNAL(itemDetached(Plasma::ExtenderItem*)),
            this, SLOT(itemDetached(Plasma::ExtenderItem*)));

    layout->addItem(extender());
    updateGeometry();
}

void ExtenderApplet::itemDetached(Plasma::ExtenderItem *)
{
    if (!extender()->attachedItems().count()) {
        destroy();
    }
}

#include "extenderapplet_p.moc"

