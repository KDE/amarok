/*
 *   Copyright (C) 2007 by Matias Valdenegro T. <mvaldenegro@informatica.utem.cl>
 *   Copyright (C) 2007 by Leo Franchi <lfranchi@gmail.com> 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#ifndef AMAROK_VBOX_LAYOUT
#define AMAROK_VBOX_LAYOUT

#include <QtCore/QRectF>
#include <QtCore/QSizeF>

#include "amarok_export.h"

#include <plasma/widgets/vboxlayout.h>

// note: this is the same as the plasma vboxlayout class except for we modify
// setGeometry method so it acts like a collapsible VBoxLayout, that is, it packs
// even expanding items from the top down according to their size hint

namespace Context
{

/**
 * Vertical Box Layout
 */
class AMAROK_EXPORT VBoxLayout : public Plasma::VBoxLayout
{
    public:
        VBoxLayout(Plasma::LayoutItem *parent = 0);
        virtual ~VBoxLayout();

		void setGeometry(const QRectF& geometry);

        void shrinkToMinimumSize();

	private:
		class Private;
		Private *const d;
};

}

#endif /* __V_BOX_LAYOUT__ */
