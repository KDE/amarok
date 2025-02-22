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

#ifndef GPODDERPODCASTTREEITEM_H_
#define GPODDERPODCASTTREEITEM_H_

#include "GpodderTreeItem.h"
#include <mygpo-qt6/Podcast.h>

class GpodderPodcastTreeItem: public GpodderTreeItem
{
    Q_OBJECT
public:
    explicit GpodderPodcastTreeItem( mygpo::PodcastPtr podcast, GpodderTreeItem *parent = nullptr );
    ~GpodderPodcastTreeItem() override;
    QVariant displayData() const override;

    mygpo::PodcastPtr podcast() const;
private:
    mygpo::PodcastPtr m_podcast;
};

#endif /* GPODDERPODCASTTREEITEM_H_ */
