/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DYNAMICCATEGORY_H
#define DYNAMICCATEGORY_H

#include "browsers/BrowserCategory.h"

class QPushButton;
class QToolButton;
class QSpinBox;

namespace PlaylistBrowserNS {

    class DynamicView;

    /**
    */
    class DynamicCategory : public BrowserCategory
    {
        Q_OBJECT
        public:
            explicit DynamicCategory( QWidget* parent );
            ~DynamicCategory() override;

            bool allowDuplicates() const;

        private Q_SLOTS:
            void navigatorChanged();
            void selectionChanged();
            void playlistCleared();
            void setUpcomingTracks( int );
            void setPreviousTracks( int );

            void setAllowDuplicates( bool value );

        private:
            QToolButton *m_onOffButton;
            QToolButton *m_duplicateButton;
            QToolButton *m_addButton;
            QToolButton *m_editButton;
            QToolButton *m_deleteButton;
            QPushButton *m_repopulateButton;

            DynamicView *m_tree;

            QSpinBox *m_previous, *m_upcoming;
    };

}

#endif
