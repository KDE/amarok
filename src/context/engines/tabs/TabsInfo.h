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
#ifndef AMAROK_TABS_INFO
#define AMAROK_TABS_INFO

#include <QString>
#include <KUrl>

//!  Struct TabsInfo contain all the infos for a tabs
class TabsInfo {

public:

    TabsInfo()
    {
        title = QString();
        tabs = QString();
        source = QString();
        url.clear();
    }

    ~TabsInfo()
    {
    }

    enum TabType { GUITAR, BASS, DRUM, PIANO };

    QString title;    // Name of the specific tab
    QString tabs;     // Data for the tab
    QString source;   // origin from the tab
    TabType tabType;  // TabType for the tab
    KUrl url;         // Url of the specific tab
};

#endif
