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

#ifndef SIMILAR_ARTISTS_BIAS_H
#define SIMILAR_ARTISTS_BIAS_H

#include "CustomBias.h"
#include <QNetworkReply>
#include "EngineObserver.h"

/**
 *  This is a bias which adds the suggested songs to the playlist.
 *
 */

namespace Amarok
{
class Collection;
}

namespace Dynamic
{

// this order of inheritance is a bit screwy, but moc wants the QObject-derived class to be first always
class SimilarArtistsBias : public QObject, public CustomBiasEntry, public EngineObserver
{
    Q_OBJECT
public:
    SimilarArtistsBias();
    ~SimilarArtistsBias();

    // reimplemented from CustomBiasEntry
    virtual QString name();
    virtual QString pluginName();
    virtual QWidget* configWidget ( QWidget* parent );

    virtual bool trackSatisfies ( const Meta::TrackPtr track );
    virtual double numTracksThatSatisfy ( const Meta::TrackList& tracks );

    virtual QDomElement xml( QDomDocument doc ) const;
    
    virtual bool hasCollectionFilterCapability();
    virtual CollectionFilterCapability* collectionFilterCapability();

    // reimplemented from EngineObserver
    virtual void engineNewTrackPlaying();

    void update();


public slots:
    void weightChanged ( double weight )
    {
        m_weight = weight;
    }

private Q_SLOTS:
    void artistQueryDone();
    void updateReady ( QString collectionId, QStringList );
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
    friend class LastFmCollectionFilterCapability; // so it can report the property and weight
};


class LastFmCollectionFilterCapability : public Dynamic::CollectionFilterCapability
{
public:
    LastFmCollectionFilterCapability ( SimilarArtistsBias* bias ) : m_bias ( bias ) {}

    // re-implemented
    virtual const QSet<QByteArray>& propertySet();
    virtual double weight() const
    {
        return m_bias->m_weight;
    }


private:
    SimilarArtistsBias* m_bias;

};

}

#endif
