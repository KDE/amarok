/**************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>          *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef LASTFM_BIASES_H
#define LASTFM_BIASES_H

#include "CustomBias.h"
#include <QNetworkReply>
#include <EngineObserver.h>

/**
 *  These classes implement a few biases for the dynamic playlist code. 
 *
 */

namespace Dynamic
{

class LastFmBias : public CustomBiasEntry, public EngineObserver
{
    Q_OBJECT
    public:
        LastFmBias();
//        ~LastFmBias();

        virtual QString name();
        virtual QWidget* configWidget();

        virtual bool trackSatisfies( const Meta::TrackPtr track );
        virtual double numTracksThatSatisfy( const Meta::TrackList& tracks );

        virtual void engineNewTrackPlaying();
        
    private Q_SLOTS:
        void artistQueryDone();

    private:
        QString m_currentArtist;
        QNetworkReply* m_artistQuery;
        
        QMap< QString, QStringList > m_savedArtists; // populated as queries come in
        
};

}

#endif
