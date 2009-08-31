/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef LAYOUTEDITWIDGET_H
#define LAYOUTEDITWIDGET_H

#include "LayoutItemConfig.h"

#include "widgets/TokenWithLayout.h"

#include <KVBox>

class KHBox;
class QCheckBox;
class QSpinBox;
class TokenDropTarget;



namespace Playlist {

/**
 * A widget to define the layout of a single type of playlist item ( head, body or single )
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */
class LayoutEditWidget : public KVBox
{
    Q_OBJECT

    public:
        /**
         * Constructor.
         * @param parent The parent widget.
         */
        LayoutEditWidget( QWidget *parent );

        /**
         * Destructor.
         */
        ~LayoutEditWidget();

        /**
         * Setup the edit widget to represent an existing LayoutItemConfig.
         * @param config The config to read.
         */
        void readLayout( Playlist::LayoutItemConfig config );

        /**
         * Create and return a LayoutItemConfig corrosponding to the current state of the editor
         * @return LayoutItemConfig matching the contents of the editor.
         */
        Playlist::LayoutItemConfig config();

        /**
         * Clear the editor.
         */
        void clear();

    signals:

        /**
         * Signal emited when the token drop target recieves input focus.
         * @param The widget that recieved the focus.
         */
        void focussed( QWidget* );
        void changed();

    private:
        QCheckBox *m_showCoverCheckBox;
        TokenDropTarget *m_dragstack;
        TokenWithLayoutFactory *m_tokenFactory;
};

}

#endif
