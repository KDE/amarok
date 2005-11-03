/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_STATISTICS_H
#define AMAROK_STATISTICS_H

#include "playlistwindow.h"
#include "statisticsbase.h"

#include <kdialogbase.h>    //baseclass

#include <qlabel.h>
#include <qvbox.h>


class Statistics : public KDialogBase
{
        Q_OBJECT

    public:
        Statistics( QWidget *parent = 0, const char *name = 0 );
        ~Statistics();

        static Statistics *instance() { return s_instance; }

    private slots:
        void loadDetails( int index );
        void resultCountChanged( int value );

    private:
        enum View { TRACK, ARTIST, ALBUM, GENRE };

        void loadSummary();
        void loadChooser();

        void buildAlbumInfo();
        void buildArtistInfo();
        void buildGenreInfo();
        void buildTrackInfo();

        StatisticsBase *m_gui;
        int m_resultCount;
        int m_viewMode;

        static Statistics *s_instance;
};

#endif /* AMAROK_STATISTICS_H */
