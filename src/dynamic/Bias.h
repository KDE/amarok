/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "widgets/MetaQueryWidget.h"
#include "TrackSet.h"

#include <QObject>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace Collections {
    class Collection;
    class QueryMaker;
}

namespace PlaylistBrowserNS
{
    class BiasWidget;
}

namespace Dynamic
{
    class AbstractBias;

    typedef QList<Dynamic::AbstractBias*> BiasList;

    /** A bias is essentially just a function that evaluates the suitability of a
        playlist in some arbitrary way.
     */
    class AbstractBias : public QObject
    {
        Q_OBJECT

        public:
            AbstractBias( QObject *parent = 0 );
            virtual ~AbstractBias();

            /** Writes the contents of this object to an xml stream.
                Only the content is writen and no enclosing tags.
                This is done to make it mirror the constructor which does not read those
                tags either.
            */
            virtual void toXml( QXmlStreamWriter *writer ) const = 0;

            /** Returns the name of this bias.
                The name is used for reading and writing to xml.
            */
            static QString name();
            virtual QString description() const = 0;

            /** Create a widget appropriate for editing the bias.
            */
            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 ) = 0;

            /** Returns the tracks that would fit at the indicated position.
                The function can also return an "outstanding" Track set and return
                later with a resultReady signal.
                In such a case "matchingTracks" cannot be called again until
                the result was received or invalidate called.
            */
            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             TrackCollectionPtr universe ) const = 0;

            /** Returns all sub-biases of this bias */
            virtual BiasList biases() const;

            /** Returns the maximum number of allowed sub-biases.
                That can be either 0, 1 or more.
            */
            virtual int getMaxBiases() const;

        signals:
            /** This signal is emitted when the bias is changed.
                e.g. an internal variable is set so that the bias will return different tracks.
                this can also happen if the bias depends on the current track and this
                track changed.
            */
            void changed( Dynamic::AbstractBias* );

            /** Emitted when the result to a previously called matchingTracks is ready */
            void resultReady( const Dynamic::TrackSet &set );

        public slots:
            /** This slot is called when the bias should discard cached results.
                This will be done in case a new playlist is requested for an updated
                collection.
            */
            virtual void invalidate();

    };

    /** A bias that returns all the tracks in the universe as possible tracks */
    class RandomBias : public AbstractBias
    {
        Q_OBJECT

        public:
            RandomBias( QObject *parent = 0 );
            RandomBias( QXmlStreamReader *reader, QObject *parent = 0 );
            virtual ~RandomBias();

            void toXml( QXmlStreamWriter *writer ) const;

            static QString name();
            QString description() const;

            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             TrackCollectionPtr universe ) const;

            virtual BiasList biases() const;

        private:
            Q_DISABLE_COPY(RandomBias)
    };

    class AndBias : public AbstractBias
    {
        Q_OBJECT

        public:
            AndBias( QObject *parent = 0 );
            AndBias( QXmlStreamReader *reader, QObject *parent = 0 );
            virtual ~AndBias();

            void toXml( QXmlStreamWriter *writer ) const;

            static QString name();
            QString description() const;

            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             TrackCollectionPtr universe ) const;

            virtual BiasList biases() const;
            virtual int getMaxBiases() const;

            /** Appends a bias to this bias.
                This object will take ownership of the bias and free it when destroyed.
            */
            void appendBias( AbstractBias* bias );
            void removeBiasAt( int i );
            void moveBias( int from, int to );

        public slots:
            virtual void invalidate();

        protected slots:
            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        protected:
            QList<AbstractBias*> m_biases;

            mutable TrackSet m_tracks;
            mutable int m_outstandingMatches;

        private:
            Q_DISABLE_COPY(AndBias)
    };

    class OrBias : public AndBias
    {
        Q_OBJECT

        public:
            OrBias( QObject *parent = 0 );
            OrBias( QXmlStreamReader *reader, QObject *parent = 0 );

            static QString name();
            QString description() const;

            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            /** Returns the tracks that would fit at the indicated position */
            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             TrackCollectionPtr universe ) const;

        protected slots:
            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        private:
            Q_DISABLE_COPY(OrBias)
    };

    /** Actually this bias works more like a Nand bias. */
    class NotBias : public AndBias
    {
        Q_OBJECT

        public:
            NotBias( QObject *parent = 0 );
            NotBias( QXmlStreamReader *reader, QObject *parent = 0 );

            static QString name();
            QString description() const;

            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            /** Returns the tracks that would fit at the indicated position */
            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             TrackCollectionPtr universe ) const;

        protected slots:
            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        private:
            Q_DISABLE_COPY(NotBias)
    };

    class TagMatchBias : public AbstractBias
    {
        Q_OBJECT

        public:
            TagMatchBias( QObject *parent = 0 );
            TagMatchBias( QXmlStreamReader *reader, QObject *parent = 0 );

            void toXml( QXmlStreamWriter *writer ) const;

            static QString name();
            QString description() const;

            virtual PlaylistBrowserNS::BiasWidget* widget( QWidget* parent = 0 );

            /** Returns the tracks that would fit at the indicated position */
            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             TrackCollectionPtr universe ) const;

            MetaQueryWidget::Filter filter() const;
            void setFilter( const MetaQueryWidget::Filter &filter);

        public slots:
            virtual void invalidate();

        protected slots:
            /** Called when we get new uids from the query maker */
            void updateReady( QString collectionId, QStringList uids );

            /** Called when the querymaker is finished */
            void updateFinished();

        protected:
            void newQuery() const;

            static QString nameForCondition( MetaQueryWidget::FilterCondition cond );
            static MetaQueryWidget::FilterCondition conditionForName( const QString &name );

            MetaQueryWidget::Filter m_filter;

            mutable QScopedPointer<Collections::QueryMaker> m_qm;

            /** The result from the current query manager are buffered in the m_uids set. */
            bool m_tracksValid;
            mutable TrackSet m_tracks;

        private:
            Q_DISABLE_COPY(TagMatchBias)
    };

    /*
    Other biases that we might want to do:
    PreventDuplicateTagBias
    QuizPlayBias (last chacter of song title is first character of new song title)
    AlbumPlayBias (new track is the next track in current album)
    */

}

Q_DECLARE_METATYPE( Dynamic::AbstractBias* )

#endif

