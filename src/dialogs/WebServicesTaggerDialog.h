/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#ifndef WEB_SERVICES_TAGGER_DIALOG_H
#define WEB_SERVICES_TAGGER_DIALOG_H

#include <config.h>

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"
#include "tagguessing/Finder.h"

#include <KDialog>

namespace Ui
{
    class WebServicesTagger;
}

#ifdef HAVE_LIBOFA
class MusicDNSFinder;
#endif

class QSortFilterProxyModel;
namespace TagGuessing {
    class TagsModel;
    class Finder;
    class TagsModelDelegate;
}

class AMAROK_EXPORT WebServicesTaggerDialog : public KDialog
{
    Q_OBJECT

    public:
        /**
         * @arg tracks Track list for search
         */
        explicit WebServicesTaggerDialog( const Meta::TrackList &tracks,
                                    QWidget *parent = 0 );
        virtual ~WebServicesTaggerDialog();

    signals:
        void sendResult( const QMap<Meta::TrackPtr, QVariantMap> result );

    private slots:
        void search();
        void progressStep();
        void searchDone();
#ifdef HAVE_LIBOFA
        void mdnsSearchDone();
#endif
        void saveAndExit();

    private:
        void init();

        Ui::WebServicesTagger *ui;

        Meta::TrackList m_tracks;

        TagGuessing::Finder *m_tagFinder;
#ifdef HAVE_LIBOFA
        MusicDNSFinder *mdns_finder;
        bool mdns_searchDone;
#endif
        TagGuessing::TagsModel *m_resultsModel;
        TagGuessing::TagsModelDelegate *m_resultsModelDelegate;
        QSortFilterProxyModel *m_resultsProxyModel;
};

#endif // WEB_SERVICES_TAGGER_DIALOG_H
