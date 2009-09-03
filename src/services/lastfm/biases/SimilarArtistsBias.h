/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef SIMILAR_ARTISTS_BIAS_H
#define SIMILAR_ARTISTS_BIAS_H

#include "CustomBiasEntry.h"
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

class SimilarArtistsBiasFactory : public CustomBiasFactory
{
    public:
        SimilarArtistsBiasFactory();
        ~SimilarArtistsBiasFactory();
        
        virtual QString name() const;
        virtual QString pluginName() const;
        virtual CustomBiasEntry* newCustomBias( double weight );
        virtual CustomBiasEntry* newCustomBias( QDomElement e, double weight );
};

// this order of inheritance is a bit screwy, but moc wants the QObject-derived class to be first always
class SimilarArtistsBias : public CustomBiasEntry, public EngineObserver
{
    Q_OBJECT
public:
    SimilarArtistsBias( double weight );
    ~SimilarArtistsBias();

    // reimplemented from CustomBiasEntry
    virtual QString pluginName() const;
    virtual QWidget* configWidget ( QWidget* parent );

    virtual bool trackSatisfies ( const Meta::TrackPtr track );
    virtual double numTracksThatSatisfy ( const Meta::TrackList& tracks );

    virtual QDomElement xml( QDomDocument doc ) const;
    
    virtual bool hasCollectionFilterCapability();
    virtual CollectionFilterCapability* collectionFilterCapability();

    // reimplemented from EngineObserver
    virtual void engineNewTrackPlaying();

    void update();

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

    QMap< QString, QSet< QByteArray > > m_savedArtists; // populated as queries come in
    // we do some caching here so multiple
    // queries of the same artist are cheap
    friend class SimilarArtistsBiasCollectionFilterCapability; // so it can report the property and weight
};


class SimilarArtistsBiasCollectionFilterCapability : public Dynamic::CollectionFilterCapability
{
public:
    SimilarArtistsBiasCollectionFilterCapability ( SimilarArtistsBias* bias ) : m_bias ( bias ) {}

    // re-implemented
    virtual const QSet<QByteArray>& propertySet();
    virtual double weight() const;


private:
    SimilarArtistsBias* m_bias;

};

}

#endif
