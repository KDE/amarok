/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_PARTBIAS_H
#define AMAROK_PARTBIAS_H

#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"

#include <QList>
#include <QVector>
class QGridLayout;
class QSlider;
class QWidget;

namespace Dynamic
{
    class PartBias;

    /** The widget for the PartBias */
    class PartBiasWidget : public QWidget
    {
        Q_OBJECT

        public:
            explicit PartBiasWidget( Dynamic::PartBias* bias, QWidget* parent = nullptr );

        protected Q_SLOTS:
            void biasAppended( Dynamic::BiasPtr bias );
            void biasRemoved( int pos );
            void biasMoved( int from, int to );

            void sliderValueChanged( int val );
            void biasWeightsChanged();

        protected:
            /** True if we just handle a signal. Used to protect against recursion */
            bool m_inSignal;

            Dynamic::PartBias* m_bias;

            QGridLayout* m_layout;

            QList<QSlider*> m_sliders;
            QList<QWidget*> m_widgets;
    };

    /** The part bias will ensure that tracks are fulfilling all the sub-biases according to it's weights.
        The bias has an implicit random sub-bias
    */
    class PartBias : public AndBias
    {
        Q_OBJECT

        public:
            /** Create a new part bias.
            */
            PartBias();

            void fromXml( QXmlStreamReader *reader ) override;
            void toXml( QXmlStreamWriter *writer ) const override;

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;
            void paintOperator( QPainter* painter, const QRect &rect, Dynamic::AbstractBias* bias ) override;

            TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr universe ) const override;

            bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const override;

            /** Returns the weights of the bias itself and all the sub-biases. */
            virtual QList<qreal> weights() const;

            /** Appends a bias to this bias.
                This object will take ownership of the bias and free it when destroyed.
            */
            void appendBias( Dynamic::BiasPtr bias ) override;
            void moveBias( int from, int to ) override;

        public Q_SLOTS:
            void resultReceived( const Dynamic::TrackSet &tracks ) override;

            /** The overall weight has changed */
            void changeBiasWeight( int biasNum, qreal value );

        Q_SIGNALS:
            /** The overall weight has changed */
            void weightsChanged();

        protected Q_SLOTS:
            void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias ) override;

        private:
            /** Using the data from m_matchingTracks it tries to compute a valid m_tracks */
            void updateResults() const; // only changes mutables

            QList<qreal> m_weights;
            mutable QVector<Dynamic::TrackSet> m_matchingTracks;

            // buffered by matchingTracks
            mutable Meta::TrackList m_playlist;
            mutable int m_contextCount;
            mutable int m_finalCount;
            mutable Dynamic::TrackCollectionPtr m_universe;

            Q_DISABLE_COPY(PartBias)
    };


    class AMAROK_EXPORT PartBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            QString i18nName() const override;
            QString name() const override;
            QString i18nDescription() const override;
            BiasPtr createBias() override;
    };

}


#endif

