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

#ifndef ECHO_NEST_BIAS_H
#define ECHO_NEST_BIAS_H

#include "CustomBiasEntry.h"
#include "CustomBiasEntryFactory.h"
#include <QNetworkReply>
#include "core/engine/EngineObserver.h"

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

class KComboBox;
class KJob;

namespace Dynamic
{
    
    class EchoNestBiasFactory : public CustomBiasEntryFactory
    {
        public:
            EchoNestBiasFactory();
            ~EchoNestBiasFactory();
            
            virtual QString name() const;
            virtual QString pluginName() const;
            virtual CustomBiasEntry* newCustomBiasEntry();
            virtual CustomBiasEntry* newCustomBiasEntry( QDomElement e);
    };
    
    // this order of inheritance is a bit screwy, but moc wants the QObject-derived class to be first always
    class EchoNestBias : public CustomBiasEntry, public EngineObserver
    {
        Q_OBJECT
        public:
            EchoNestBias();
            ~EchoNestBias();
            
            // reimplemented from CustomBiasEntry
            virtual QString pluginName() const;
            virtual QWidget* configWidget ( QWidget* parent );
            
            virtual bool trackSatisfies ( const Meta::TrackPtr track );
            virtual double numTracksThatSatisfy ( const Meta::TrackList& tracks );
            
            virtual QDomElement xml( QDomDocument doc ) const;
            
            virtual bool hasCollectionFilterCapability();
            virtual CollectionFilterCapability* collectionFilterCapability( double weight );
            
            // reimplemented from EngineObserver
            virtual void engineNewTrackPlaying();
            
            void update();
            
        private Q_SLOTS:
            void artistNameQueryDone( KJob* );
            void artistSuggestedQueryDone( KJob* );
            void updateReady ( QString collectionId, QStringList );
            void updateFinished();
            void collectionUpdated();
            void selectionChanged( int );
            
        private:
            KUrl createUrl( QString method, QMultiMap< QString, QString > params );
            
            QString m_currentArtist;
            QStringList m_currentPlaylist;
            QMap< QString, QString > m_artistIds;
            
            QMap< KIO::StoredTransferJob*, QString> m_artistNameQueries;
            KIO::StoredTransferJob* m_artistSuggestedQuery;
            QueryMaker* m_qm; // stored so it can be refreshed
            // if the collection changes
            Amarok::Collection* m_collection; // null => all queryable collections
            bool m_needsUpdating;
            QMutex m_mutex;
            
            KComboBox* m_fieldSelection;
            bool m_currentOnly;
            
            QMap< QString, QSet< QByteArray > > m_savedArtists; // populated as queries come in
            // we do some caching here so multiple
            // queries of the same artist are cheap
            friend class EchoNestBiasCollectionFilterCapability; // so it can report the property and weight
    };
    
    
    class EchoNestBiasCollectionFilterCapability : public Dynamic::CollectionFilterCapability
    {
        public:
            EchoNestBiasCollectionFilterCapability ( EchoNestBias* bias, double weight ) : m_bias ( bias ), m_weight( weight ) {}
            
            // re-implemented
            virtual const QSet<QByteArray>& propertySet();
            virtual double weight() const;
            
            
        private:
            EchoNestBias* m_bias;
            double m_weight;
            
    };
    
}

#endif
