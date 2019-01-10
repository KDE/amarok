/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011 Ralf Engels <ralf-engels@gmx.de>                            *
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

#include "amarok_export.h"
#include "dynamic/TrackSet.h"

#include <QMetaType>
#include <QObject>
#include <QRect>
#include <QWidget>
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

class QPainter;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace Collections {
    class Collection;
    class QueryMaker;
}

namespace Dynamic
{
    class AbstractBias;

    typedef QExplicitlySharedDataPointer<Dynamic::AbstractBias> BiasPtr;
    typedef QList<Dynamic::BiasPtr> BiasList;

    /** A bias is essentially just a function that evaluates the suitability of a
        playlist in some arbitrary way.
        It's function is to return new tracks for a playlist. For this it can
        only rely on previous tracks in the playlist (because of the way the
        playlists are generated).


        All biases are shared data to prevent problems when they are removed while the
        BiasSolver is running at the same time.
     */
    class AMAROK_EXPORT AbstractBias : public QObject, public QSharedData
    {
        Q_OBJECT

        public:
            AbstractBias();
            virtual ~AbstractBias();

            /** Reads the contents of this object from an xml stream */
            virtual void fromXml( QXmlStreamReader *reader );

            /** Writes the contents of this object to an xml stream.
                Only the content is written and no enclosing tags.
                This is done to make it mirror the constructor which does not read those
                tags either.
            */
            virtual void toXml( QXmlStreamWriter *writer ) const;

            /** Creates a new bias (with all sub-biases) that is a copy of this.
             *
             *  This function does a deep copy by writing and reading back from xml.
             *  Should work for all biases.
             */
            BiasPtr clone() const;

            /** Returns the name of this bias.
                The name is used for reading and writing to xml.
            */
            static QString sName();

            /** The same as sName just not static */
            virtual QString name() const;

            /** The textual representation of this bias (localized) */
            virtual QString toString() const = 0;

            /** Create a widget appropriate for editing the bias.
            */
            virtual QWidget* widget( QWidget* parent = nullptr );

            /** Paints an operator (like "and" or a progress bar") in front of a bias item */
            virtual void paintOperator( QPainter* painter, const QRect &rect, Dynamic::AbstractBias* bias );

            /** Returns the tracks that would fit at the end of the given playlist.
                The function can also return an "outstanding" Track set and return
                later with a resultReady signal.
                In such a case "matchingTracks" cannot be called again until
                the result was received or invalidate called.
                @param playlist The current playlist context for the track.
                @param contextCount The number of songs that are already fixed.
                @param finalCount The number of tracks that the playlist should finally contain (including the contextCount)
                @param universe A TrackCollectionPtr to be used for the resulting TrackSet
            */
            virtual TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr &universe ) const = 0;

            /** Returns true if indicated track fits that position.
                The function might block until a result is ready.
                @param position The position inside the playlist that we search a track for.
                                The position can be larger than the number of playlist entries
                                if we search a track for the end of the playlist.
                @param playlist The current playlist context for the track.
                @param contextCount The number of songs that are already fixed.
            */
            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist, int contextCount ) const = 0;

        Q_SIGNALS:
            /** This signal is emitted when the bias is changed.
                e.g. an internal variable is set so that the bias will return different tracks.
                this can also happen if the bias depends on the current track and this
                track changed.
            */
            void changed( Dynamic::BiasPtr thisBias );

            /** This signal is emitted when this bias should be replaced by a new one. */
            void replaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );

            /** Emitted when the result to a previously called matchingTracks is ready */
            void resultReady( const Dynamic::TrackSet &set );

        public Q_SLOTS:
            /** This slot is called when the bias should discard cached results.
                This will be done in case a new playlist is requested for an updated
                collection.
            */
            virtual void invalidate();

            /** Call this function when this bias should be replaced by a new one.
                @param newBias The bias that replaces this bias. If you give
                an empty BiasPtr as argument the bias will be removed.
            */
            virtual void replace( const Dynamic::BiasPtr &newBias );
    };

    /** A bias that returns all the tracks in the universe as possible tracks */
    class AMAROK_EXPORT RandomBias : public AbstractBias
    {
        Q_OBJECT

        public:
            RandomBias();
            virtual ~RandomBias();

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;

            TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr &universe ) const override;

            bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const override;

        private:
            Q_DISABLE_COPY(RandomBias)
    };


    /** An and bias
        The And Bias currently serves as a base class for all biases
        with children.
    */
    class AMAROK_EXPORT AndBias : public AbstractBias
    {
        Q_OBJECT

        public:
            /** Create a new And bias.
            */
            AndBias();
            ~AndBias() override;

            void fromXml( QXmlStreamReader *reader ) override;
            void toXml( QXmlStreamWriter *writer ) const override;

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;
            void paintOperator( QPainter* painter, const QRect &rect, Dynamic::AbstractBias* bias ) override;

            TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr &universe ) const override;

            bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const override;

            /** Appends a bias to this bias.
                This object will take ownership of the bias and free it when destroyed.
            */
            virtual void appendBias( const Dynamic::BiasPtr &bias );
            virtual void moveBias( int from, int to );

            BiasList biases() const
            { return m_biases; }

        Q_SIGNALS:
            void biasAppended( Dynamic::BiasPtr bias );
            void biasRemoved( int pos );
            void biasMoved( int from, int to );

        public Q_SLOTS:
            void invalidate() override;

        protected Q_SLOTS:
            virtual void resultReceived( const Dynamic::TrackSet &tracks );
            virtual void biasReplaced( const Dynamic::BiasPtr &oldBias, const Dynamic::BiasPtr &newBias );
            virtual void biasChanged( const Dynamic::BiasPtr &bias );

        protected:
            BiasList m_biases;

            mutable TrackSet m_tracks;
            mutable int m_outstandingMatches;

        private:
            Q_DISABLE_COPY(AndBias)
    };

    class AMAROK_EXPORT OrBias : public AndBias
    {
        Q_OBJECT

        public:
            OrBias();

            static QString sName();
            QString name() const override;
            QString toString() const override;

            void paintOperator( QPainter* painter, const QRect &rect, Dynamic::AbstractBias* bias ) override;

            TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr &universe ) const override;

            bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const override;

        protected Q_SLOTS:
            void resultReceived( const Dynamic::TrackSet &tracks ) override;

        private:
            Q_DISABLE_COPY(OrBias)
    };

}

Q_DECLARE_METATYPE( Dynamic::BiasPtr )

#endif
