/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICBRAINZTAGGER_H
#define MUSICBRAINZTAGGER_H

#include <config-amarok.h>
#include <KDialog>
#include "core/meta/Meta.h"
#include "MusicBrainzFinder.h"
#include "MusicBrainzTagsModel.h"
#include "MusicBrainzTrackListModel.h"
#include <QItemSelectionModel>

#ifdef HAVE_LIBOFA
    #include "MusicDNSFinder.h"
#endif

namespace Ui
{
    class MusicBrainzTagger;
}

class TrackListModel;

class MusicBrainzTagger : public KDialog
{
    Q_OBJECT

    public:
        /**
         * @arg tracks Track list for search
         */
        explicit MusicBrainzTagger( const Meta::TrackList &tracks, bool autostart,
                                    QWidget *parent = 0 );
        virtual ~MusicBrainzTagger();

    signals:
        void sendResult( const QMap < Meta::TrackPtr, QVariantMap > result );

    private slots:
        void saveAndExit();
        void search();
        void searchDone();
#ifdef HAVE_LIBOFA
        void mdnsSearchDone();
#endif

        void trackFound( const Meta::TrackPtr track, const QVariantMap tags );
        void progressStep();

    private:
        void init();

        Ui::MusicBrainzTagger *ui;

        Meta::TrackList m_tracks;
        Meta::TrackList m_failedTracks;

        MusicBrainzFinder *mb_finder;
#ifdef HAVE_LIBOFA
        MusicDNSFinder *mdns_finder;
        bool mdns_used;
        bool mdns_searchDone;
#endif

        MusicBrainzTrackListModel *q_trackListModel;
        MusicBrainzTagsModel *q_resultsModel;
};

#endif // MUSICBRAINZTAGGER_H
