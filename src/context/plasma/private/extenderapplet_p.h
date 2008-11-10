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

#ifndef EXTENDERAPPLET_H
#define EXTENDERAPPLET_H

#include "applet.h"

/**
 * This class is used as a 'host' for detached extender items. When an extender item is dropped
 * somewhere, this applet is added at the location where the item is dropped, and the item is added
 * to it's extender.
 */
class ExtenderApplet : public Plasma::Applet
{
    Q_OBJECT
    public:
        ExtenderApplet(QObject *parent, const QVariantList &args);
        ~ExtenderApplet();

        void init();

    public Q_SLOTS:
        void itemDetached(Plasma::ExtenderItem *);
};

#endif
