/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef MAGNATUNEARTISTINFOBOX_H
#define MAGNATUNEARTISTINFOBOX_H

#include "amarok.h"
#include "magnatunetypes.h"
#include "statusbar.h"

#include <khtml_part.h>
#include <kio/jobclasses.h>
#include <kio/job.h>

/**
A class for displaying html info about a Magnatune artist

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneArtistInfoBox : public KHTMLPart
{
Q_OBJECT

public:
    MagnatuneArtistInfoBox(QWidget *parentWidget, const char *widgetname);

    ~MagnatuneArtistInfoBox();

    bool displayArtistInfo(KURL url);

    bool displayAlbumInfo(MagnatuneAlbum * album);

protected:

   KIO::TransferJob * m_infoDownloadJob;

   QString extractArtistInfo(QString artistPage);


protected slots:

   void infoDownloadComplete( KIO::Job* downLoadJob);

};

#endif
