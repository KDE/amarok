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

#ifndef ECHO_NEST_BIAS_H
#define ECHO_NEST_BIAS_H

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
namespace KIO {
    class StoredTransferJob;
}
class KJob;

namespace Dynamic
{
    
    class EchoNestBiasFactory : public CustomBiasFactory
    {
        public:
            EchoNestBiasFactory();
            ~EchoNestBiasFactory();
            
            virtual QString name() const;
            virtual QString pluginName() const;
            virtual CustomBiasEntry* newCustomBias( double weight );
            virtual CustomBiasEntry* newCustomBias( QDomElement e, double weight );
    };
    
    // this order of inheritance is a bit screwy, but moc wants the QObject-derived class to be first always
    class EchoNestBias : public CustomBiasEntry, public EngineObserver
    {
        Q_OBJECT
        public:
            EchoNestBias( double weight );
            ~EchoNestBias();
            
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
            void artistNameQueryDone( KJob* );
            void artistSuggestedQueryDone( KJob* );
            void updateReady ( QString collectionId, QStringList );
            void updateFinished();
            void collectionUpdated();
            
        private:
            KUrl createUrl( QString method, QMap< QString, QString > params );
            
            QString m_currentArtist;
            QString m_artistId;
            
            KIO::StoredTransferJob* m_artistNameQuery;
            KIO::StoredTransferJob* m_artistSuggestedQuery;
            QueryMaker* m_qm; // stored so it can be refreshed
            // if the collection changes
            Amarok::Collection* m_collection; // null => all queryable collections
            bool m_needsUpdating;
            QMutex m_mutex;
            
            QMap< QString, QSet< QByteArray > > m_savedArtists; // populated as queries come in
            // we do some caching here so multiple
            // queries of the same artist are cheap
            friend class EchoNestBiasCollectionFilterCapability; // so it can report the property and weight
    };
    
    
    class EchoNestBiasCollectionFilterCapability : public Dynamic::CollectionFilterCapability
    {
        public:
            EchoNestBiasCollectionFilterCapability ( EchoNestBias* bias ) : m_bias ( bias ) {}
            
            // re-implemented
            virtual const QSet<QByteArray>& propertySet();
            virtual double weight() const;
            
            
        private:
            EchoNestBias* m_bias;
            
    };
    
}

#endif
