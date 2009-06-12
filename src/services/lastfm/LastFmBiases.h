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
#include "EngineObserver.h"

/**
 *  These classes implement a few biases for the dynamic playlist code. 
 *
 */

namespace Amarok
{
    class Collection;
}

namespace Dynamic
{

// this order of inheritance is a bit screwy, but moc wants the QObject-derived class to be first always
class LastFmBias : public QObject, public Dynamic::CollectionFilterBias, public CustomBiasEntry, public EngineObserver
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

        void update();

        virtual const QSet<QByteArray>& propertySet();

        virtual bool filterFromCollection() { return true; }

        /**
         * This is a bit screwy due to the levels of abstraction that we have.
         * CustomBias makes no demands on it's sub-biases, but this bias in particular
         * is a CollectionFilterBias, which means it needs to know the value of its slider.
         * However, this slider is owned and operated by CustomBiasEntry (tm), and we don't  have
         * access to it. However, when CustomBiasEntry notifies CustomBias, we hop onboard and also
         * change our local weight variable to be the same. This way when the BiasSolver asks us for
         * the value of the slider we can truthfully answer.
         *
         * And there was much rejoicing.
         */
        virtual double weight() const { return m_weight; }

    public slots:
        void weightChanged( double weight ) { m_weight = weight; }
        
    private Q_SLOTS:
        void artistQueryDone();
        void updateReady( QString collectionId, QStringList );
        void updateFinished();
        void collectionUpdated();

    private:
        QString m_currentArtist;
        QNetworkReply* m_artistQuery;
        QueryMaker* m_qm; // stored so it can be refreshed
                          // if the collection changes
        Amarok::Collection* m_collection; // null => all queryable collections
        bool m_needsUpdating;
        QMutex m_mutex;

        double m_weight;

        QMap< QString, QSet< QByteArray > > m_savedArtists; // populated as queries come in
                                                            // we do some caching here so multiple
                                                            // queries of the same artist are cheap
        
};

}

#endif
