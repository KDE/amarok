/****************************************************************************************
 * Copyright (c) 2011 Sandeep Raghuraman <sandy.8925@gmail.com>                         *
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

#ifndef AMAROK_PODCASTFILENAMELAYOUTCONFIGDIALOG_H
#define AMAROK_PODCASTFILENAMELAYOUTCONFIGDIALOG_H

#include "SqlPodcastMeta.h"

#include <KPageDialog>

namespace Ui
{
class PodcastFilenameLayoutConfigWidget;
}

class PodcastFilenameLayoutConfigDialog : public KPageDialog
{

    Q_OBJECT
public:
    explicit PodcastFilenameLayoutConfigDialog( Podcasts::SqlPodcastChannelPtr channel, QWidget *parent = 0 );
    //shows the filename configuration dialog
    bool configure();

protected Q_SLOTS:
    void slotApply();

private:
    void init();
    Podcasts::SqlPodcastChannelPtr m_channel;
    Ui::PodcastFilenameLayoutConfigWidget *m_pflc;    
    int m_choice;
};

#endif
