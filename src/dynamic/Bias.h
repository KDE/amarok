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

#include "shared/amarok_export.h"
#include "TrackSet.h"

#include <QObject>
#include <QWidget>
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

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

        All biases are shared to prevent problems when they are removed while the
        BiasSolver is running at the same time.
     */
    class AMAROK_EXPORT AbstractBias : public QObject, public QSharedData
    {
        Q_OBJECT

        public:
            // role used for the model
            enum Roles
            {
                WidgetRole = 0xf00d
            };

            AbstractBias();
            virtual ~AbstractBias();

            /** Writes the contents of this object to an xml stream.
                Only the content is writen and no enclosing tags.
                This is done to make it mirror the constructor which does not read those
                tags either.
            */
            virtual void toXml( QXmlStreamWriter *writer ) const;

            /** Returns the name of this bias.
                The name is used for reading and writing to xml.
            */
            static QString sName();

            /** The same as sName just not static */
            virtual QString name() const;

            /** Create a widget appropriate for editing the bias.
            */
            virtual QWidget* widget( QWidget* parent = 0 );

            /** Returns the tracks that would fit at the indicated position.
                The function can also return an "outstanding" Track set and return
                later with a resultReady signal.
                In such a case "matchingTracks" cannot be called again until
                the result was received or invalidate called.
                @param position The position inside the playlist that we search a track for.
                                The position can be larger than the number of playlist entries
                                if we search a track for the end of the playlist.
                @param playlist The current playlist context for the track.
                @param contextCount The number of songs that are already fixed. Those songs
                                should not take part in the calculation of an energy value.
                @param universe A TrackCollectionPtr to be used for the resulting TrackSet
            */
            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist, int contextCount,
                                             const TrackCollectionPtr universe ) const = 0;

            /** Returns true if indicated track fits that position.
                The function might block until a result is ready.
                @param position The position inside the playlist that we search a track for.
                                The position can be larger than the number of playlist entries
                                if we search a track for the end of the playlist.
                @param playlist The current playlist context for the track.
                @param contextCount The number of songs that are already fixed. Those songs
                                should not take part in the calculation of an energy value.
                @param universe A TrackCollectionPtr to be used for the resulting TrackSet
            */
            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist, int contextCount ) const = 0;


            /** Returns an energy value for the given playlist.
                The function might block until a result is ready.
                The energy value should be in the range 0-1.
                0 is the perfect value.
                The default implementation here uses trackMatches to calculate the energy
                @param playlist The current playlist context for the track.
                @param contextCount The number of songs that are already fixed. Those songs
                                should not take part in the calculation of an energy value.
            */
            virtual double energy( const Meta::TrackList& playlist, int contextCount ) const;

        signals:
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

        public slots:
            /** This slot is called when the bias should discard cached results.
                This will be done in case a new playlist is requested for an updated
                collection.
            */
            virtual void invalidate();

            /** Call this function when this bias should be replaced by a new one. */
            virtual void replace( Dynamic::BiasPtr newBias );

        protected:
            /** Recursively set's the row number of the bias returning the next number */
    };

    /** A bias that returns all the tracks in the universe as possible tracks */
    class AMAROK_EXPORT RandomBias : public AbstractBias
    {
        Q_OBJECT

        public:
            RandomBias();
            RandomBias( QXmlStreamReader *reader );
            virtual ~RandomBias();

            void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist, int contextCount,
                                             const TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;

            virtual double energy( const Meta::TrackList& playlist, int contextCount ) const;

        private:
            Q_DISABLE_COPY(RandomBias)
    };

    /** A bias that returns tracks that are not in the playlist already */
    class AMAROK_EXPORT UniqueBias : public AbstractBias
    {
        Q_OBJECT

        public:
            UniqueBias();
            UniqueBias( QXmlStreamReader *reader );
            virtual ~UniqueBias();

            void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist, int contextCount,
                                             const TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;

        private:
            Q_DISABLE_COPY(UniqueBias)
    };

    class AMAROK_EXPORT AndBias : public AbstractBias
    {
        Q_OBJECT

        public:
            AndBias();
            AndBias( QXmlStreamReader *reader );
            virtual ~AndBias();

            void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist, int contextCount,
                                             const TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;

            /** Appends a bias to this bias.
                This object will take ownership of the bias and free it when destroyed.
            */
            virtual void appendBias( Dynamic::BiasPtr bias );
            virtual void moveBias( int from, int to );

            BiasList biases() const
            { return m_biases; }

        signals:
            void biasAppended( Dynamic::BiasPtr bias );
            void biasRemoved( int pos );
            void biasMoved( int from, int to );

        public slots:
            virtual void invalidate();

        protected slots:
            virtual void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );
            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        protected:
            bool m_duringConstruction; // protect against accidentially freeing this bias by creating a BiasPtr
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
            OrBias( QXmlStreamReader *reader );

            static QString sName();
            virtual QString name() const;

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist, int contextCount,
                                             const TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;

        protected slots:
            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        private:
            Q_DISABLE_COPY(OrBias)
    };

    /** Actually this bias works more like a NOR bias. */
    class AMAROK_EXPORT NotBias : public OrBias
    {
        Q_OBJECT

        public:
            NotBias();
            NotBias( QXmlStreamReader *reader );

            static QString sName();
            virtual QString name() const;

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist, int contextCount,
                                             const TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;

            virtual double energy( const Meta::TrackList& playlist, int contextCount ) const;

        protected slots:
            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        private:
            Q_DISABLE_COPY(NotBias)
    };

}

Q_DECLARE_METATYPE( Dynamic::BiasPtr )

#endif

