//
// C++ Interface: magnatuneartistinfobox
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MAGNATUNEARTISTINFOBOX_H
#define MAGNATUNEARTISTINFOBOX_H

#include "magnatunetypes.h"

#include "amarok.h"
#include "statusbar.h"
#include <khtml_part.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

/**
A class for displaying html info about a Magnatune artist

@author Mark Kretschmann
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
