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

#include "config.h"
#include <KDialog>
#include "core/meta/Meta.h"
#include "musicbrainz/MusicBrainzFinder.h"
#include "musicbrainz/MusicBrainzTags.h"
#include <QItemSelectionModel>

#ifdef HAVE_LIBOFA
    #include "musicbrainz/MusicDNSFinder.h"
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
        explicit MusicBrainzTagger( const Meta::TrackList &tracks,
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
        void progressStep();

    private:
        void init();

        Ui::MusicBrainzTagger *ui;

        Meta::TrackList m_tracks;

        MusicBrainzFinder *mb_finder;
#ifdef HAVE_LIBOFA
        MusicDNSFinder *mdns_finder;
        bool mdns_searchDone;
#endif
        MusicBrainzTagsModel *q_resultsModel;
        MusicBrainzTagsModelDelegate *q_resultsModelDelegate;
};

#endif // MUSICBRAINZTAGGER_H
