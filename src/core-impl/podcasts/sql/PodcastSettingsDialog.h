/****************************************************************************************
 * Copyright (c) 2006-2008 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#ifndef AMAROK_PODCASTSETTINGSDIALOG_H
#define AMAROK_PODCASTSETTINGSDIALOG_H

#include "SqlPodcastMeta.h"

#include <KPageDialog>

namespace Ui {
    class PodcastSettingsBase;
}

class PodcastSettingsDialog : public KPageDialog
{
    Q_OBJECT

    public:
        explicit PodcastSettingsDialog( const Podcasts::SqlPodcastChannelPtr &channel, QWidget* parent=nullptr );

        bool configure();

    protected:
        bool hasChanged();

    protected Q_SLOTS:
        void checkModified();
        void slotApply();
        void slotFeedUrlClicked( const QString &url );
        void launchFilenameLayoutConfigDialog();

    private:
        void init();
        QString requesterSaveLocation();

        Ui::PodcastSettingsBase *m_ps;

        Podcasts::SqlPodcastChannelPtr m_channel;
};

#endif /*AMAROK_PODCASTSETTINGSDIALOG_H*/
