/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#include <config.h>
#include "core/meta/forward_declarations.h"

#include <QDialog>

namespace Ui
{
    class MusicBrainzTagger;
}

class MusicBrainzFinder;
class MusicBrainzTagsModel;
class MusicBrainzTagsModelDelegate;

class QSortFilterProxyModel;

class MusicBrainzTagger : public QDialog
{
    Q_OBJECT

    public:
        /**
         * @arg tracks Track list for search
         */
        explicit MusicBrainzTagger( const Meta::TrackList &tracks,
                                    QWidget *parent = nullptr );
        ~MusicBrainzTagger() override;

    Q_SIGNALS:
        void sendResult( const QMap<Meta::TrackPtr, QVariantMap> &result );

    private Q_SLOTS:
        void search();
        void progressStep();
        void searchDone();
        void saveAndExit();

    private:
        void init();

        Ui::MusicBrainzTagger *ui;

        Meta::TrackList m_tracks;

        MusicBrainzFinder *mb_finder;
        MusicBrainzTagsModel *m_resultsModel;
        MusicBrainzTagsModelDelegate *m_resultsModelDelegate;
        QSortFilterProxyModel *m_resultsProxyModel;
};

#endif // MUSICBRAINZTAGGER_H
