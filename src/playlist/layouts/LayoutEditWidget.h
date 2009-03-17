/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef LAYOUTEDITWIDGET_H
#define LAYOUTEDITWIDGET_H

#include "LayoutItemConfig.h"

#include "widgets/TokenWithLayout.h"

#include <KVBox>

class KHBox;
class QCheckBox;
class QSpinBox;
class TokenDropTarget;

/**
    A widget to define the layout of a single type of playlist item ( head, body or single )
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/

namespace Playlist {

class LayoutEditWidget : public KVBox
{
    Q_OBJECT

    public:
        LayoutEditWidget( QWidget *parent );
        ~LayoutEditWidget();

        void readLayout( Playlist::LayoutItemConfig config );
        Playlist::LayoutItemConfig config();

    signals:
        void focussed ( QWidget* );

    private:
        QCheckBox *m_showCoverCheckBox;
        TokenDropTarget *m_dragstack;
        TokenWithLayoutFactory *m_tokenFactory;
};

}

#endif
