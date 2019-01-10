/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
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

#ifndef GPODDERPODCASTREQUESTHANDLER_H_
#define GPODDERPODCASTREQUESTHANDLER_H_

#include "GpodderServiceModel.h"
#include <mygpo-qt5/PodcastList.h>

#include <QModelIndex>

class GpodderPodcastRequestHandler : public QObject
{
    Q_OBJECT
public:
    GpodderPodcastRequestHandler( mygpo::PodcastListPtr podcasts, const QModelIndex &parentItem, GpodderServiceModel *model );
    virtual ~GpodderPodcastRequestHandler();

public Q_SLOTS:
    void finished();
    void requestError( QNetworkReply::NetworkError );
    void parseError();

private:
    mygpo::PodcastListPtr m_podcasts;
    QModelIndex m_parentItem;
    GpodderServiceModel *m_model;
};

#endif /* GPODDERPODCASTREQUESTHANDLER_H_ */
