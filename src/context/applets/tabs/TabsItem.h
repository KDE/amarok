/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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

#ifndef AMAROK_TABS_ITEM_H
#define AMAROK_TABS_ITEM_H

#include "context/engines/tabs/TabsInfo.h"

#include <QStandardItem>

class TabsItem : public QStandardItem
{
    public:
        TabsItem();
        ~TabsItem() { }

        /**
         * Sets the tab-data for this specific tab-item
         *
         * @arg TabsInfo pointer to associate with
         */
        void setTab( TabsInfo * );

        /**
         * defines the size of the tab-icons to display
         */
        void setIconSize( const int iconSize );
        void setTabIcon( TabsInfo::TabType tabtype );

        const QString getTabData();
        const QString getTabTitle();
        const QString getTabUrl();
        const QString getTabSource();

    private:
        int      m_iconSize;
        TabsInfo *m_tabsInfo;
};

#endif // multiple inclusion guard
