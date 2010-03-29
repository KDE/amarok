/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_BIAS_H
#define AMAROK_BIAS_H

#include "core/meta/Meta.h"
#include "core-implementations/collections/support/XmlQueryReader.h"

#include <QDomElement>
#include <QMutex>
#include <QObject>
#include <QSet>

namespace Collections {
    class Collection;
    class QueryMaker;
    class XmlQueryWriter;
}

namespace PlaylistBrowserNS
{
    class BiasWidget;
}


namespace Dynamic
{

    class CollectionFilterCapability;
    
    /**
     * A bias is essentially just a function that evaluates the suitability of a
     * playlist in some arbitrary way.
     */
    class Bias
    {
        public:
            static Bias* fromXml( QDomElement );

            Bias();
            virtual ~Bias() {}

            QString description() const;
            void setDescription( const QString& );

            /**
             * Create a widget appropriate for editing the bias.
             */
            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            /**
             * Returns xml suitable to be loaded with fromXml.
             */
            virtual QDomElement xml() const = 0;

            /**
             * Should the bias be considered or ignored by the playlist?
             */
            void setActive( bool );
            bool active();


            /**
             * Returns a value in the range [-1,1]. (The sign is not considered,
             * but it may be useful to return negative numbers for
             * implementing reevaluate.) Playlist generation is being
             * treated as a minimization problem, so 0 means the bias is completely
             * satisfied, (+/-)1 that it is not satisfied at all. The tracks that
             * precede the playlist are passed as 'context'.
             */
            virtual double energy( const Meta::TrackList& playlist, const Meta::TrackList& context ) = 0;


            /**
             * When a track is swapped in the playlist, avoid completely reevaluating
             * the energy function if possible.
             */
            virtual double reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist,
                    Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context );

            /**
             * This returns whether or not this bias operates on the collection in such a way
             * that it wants to filter the tracks to be used to generate the initial playlists.
             * An example of this is the GlobalBias---which modifies the creation of playlists to
             * optimized.
             *
             * If if this is false, and there are no biases that return true for this turned on, the
             * BiasSolver selects a completely random playlist from the collection, then tries to
             * optimize it by calling energy() on mutations.
             *
             * Classes that return true here should also return a valid CollectionFilterCapability.
             *
             */
            virtual bool hasCollectionFilterCapability() { return false; }

            /** 
             * Returns a QSet< QByteArray > of track uids that match this bias. Used when building the
             * initial playlists, this must be implemented if your bias returns true for filterFromCollection.
             */
            virtual CollectionFilterCapability* collectionFilterCapability() { return 0; }
            
        protected:
            bool m_active;
            QString m_description;
    };



    /**
     * A bias that depends on the state of the collection.
     */
    class CollectionDependantBias : public QObject, public Bias
    {
        Q_OBJECT

        public:
            CollectionDependantBias();
            CollectionDependantBias( Collections::Collection* );

            /**
             * This gets called when the collection changes. It's expected to
             * emit a biasUpdated signal when finished.
             */
            virtual void update() = 0;
            bool needsUpdating();

        signals:
            void biasUpdated( CollectionDependantBias* );

        public slots:
            void collectionUpdated();

        protected:
            Collections::Collection* m_collection; // null => all queryable collections
            bool m_needsUpdating;
            QMutex m_mutex;
    };

    /**
     * This is a capability that biases have if they operate on and expect to filter the collection. It stores
     * the currently matching tracks in a QSet of uids, and shares them with the BiasSolver
     * when asked in order to generate initial starting playlists.
     */
    class CollectionFilterCapability
    {
        public:
            CollectionFilterCapability() {}
            virtual ~CollectionFilterCapability() {}


            /**
             * This is the list of tracks from the collection that fit the Bias.
             * The QSet is a set of bytearray UIDs from the collection itself. 
             */
            virtual const QSet< QByteArray> & propertySet() = 0;
            
            /**
             * All collection filter biases must also share a weight to
             * be read, as it is used by the solver when generating the
             * initial playlist.
             *
             */
            virtual double weight() const = 0;

    };
            
    /**
     * This a bias in which the order and size of the playlist are not
     * considered. Instead we want a given proportion (weight) of the tracks to
     * have a certain property (or belong to a certain set).
     */
    class GlobalBias : public CollectionDependantBias
    {
        Q_OBJECT
                
        public:
            GlobalBias( double weight, XmlQueryReader::Filter );
            GlobalBias( Collections::Collection* coll, double weight, XmlQueryReader::Filter query );
            
            ~GlobalBias();

            void setQuery( XmlQueryReader::Filter );

            QDomElement xml() const;

            PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );
            const XmlQueryReader::Filter& filter() const;

            double energy( const Meta::TrackList& playlist, const Meta::TrackList& context );
            double reevaluate( double oldEnergy, const Meta::TrackList& oldPlaylist,
                    Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context );

            virtual const QSet<QByteArray>* propertySet() { return &m_property; }
            bool trackSatisfies( Meta::TrackPtr );
            void update();

            virtual double weight() const;
            void setWeight( double );

            // reimplemented
            virtual bool hasCollectionFilterCapability();
            virtual CollectionFilterCapability* collectionFilterCapability();
            
        private slots:
            void updateReady( QString collectionId, QStringList );
            void updateFinished();

        private:
            double m_weight; ///< range: [0,1]
            QSet<QByteArray> m_property;
            Collections::XmlQueryWriter* m_qm;
            XmlQueryReader::Filter m_filter;

            // Disable copy constructor and assignment
            GlobalBias( const GlobalBias& );
            GlobalBias& operator= ( const GlobalBias& );

            friend class GlobalBiasFilterCapability; // friend so it we can share our privates
    };

    /**
     * This is the implementation for GlobalBias of the CollectionFilterCapability.
     */
    class GlobalBiasFilterCapability : public CollectionFilterCapability
    {
        public:
            GlobalBiasFilterCapability( GlobalBias* bias ) : m_bias( bias ) {}

            virtual const QSet<QByteArray>& propertySet() { return m_bias->m_property; }
            virtual double weight() const { return m_bias->weight(); };

        private:
            GlobalBias* m_bias;
    };
    
    /**
     * A bias that works with numerical fields and attempts to fit the playlist to
     * a normal distribution.
     */
    class NormalBias : public Bias
    {
        public:
            NormalBias();

            QDomElement xml() const;
            PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            double energy( const Meta::TrackList& playlist, const Meta::TrackList& context );

            /**
             * The mean of the distribution, the 'ideal' value.
             */
            void setValue( double );
            double value() const;

            /**
             * The QueryMaker field that will be considered. Only numerical
             * fields (e.g. year, score, etc) will work.
             */
            void setField( qint64 );
            qint64 field() const;

            /**
             * A number in [0.0,1.0] that controls the variance of the
             * distribution in a sort of contrived but user friendly way. 1.0 is
             * maximum strictness, 0.0 means minimum strictness.
             */
            void setScale( double );
            double scale();

            virtual bool filterFromCollection() { return false; }

        private:
            double sigmaFromScale( double scale );
            double relevantField( Meta::TrackPtr t );
            void setDefaultMu();

            double m_scale;

            double m_mu;    //!< mean
            double m_sigma; //!< standard deviation

            qint64 m_field;
    };
}

Q_DECLARE_METATYPE( Dynamic::Bias* )

#endif

