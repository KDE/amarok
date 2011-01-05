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

#include "Bias.h"
#include "BiasFactory.h"

namespace Dynamic
{

    /** The part bias will ensure that tracks are fulfilling all the sub-biases according to it's weights.
        The bias has an implicit random sub-bias
    */
    class PartBias : public AndBias
    {
        Q_OBJECT

        public:
            PartBias();
            PartBias( QXmlStreamReader *reader );

            virtual void toXml( QXmlStreamWriter *writer ) const;

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

            /** Returns the weights of the bias itself and all the sub-biases. */
            virtual QList<qreal> weights() const;

            /** Appends a bias to this bias.
                This object will take ownership of the bias and free it when destroyed.
            */
            virtual void appendBias( Dynamic::BiasPtr bias );
            virtual void moveBias( int from, int to );

        public slots:
            /** The overall weight has changed */
            void changeBiasWeight( int biasNum, qreal value );

            // virtual void resultReceived( const Dynamic::TrackSet &tracks );

        signals:
            /** The overall weight has changed */
            void weightsChanged();

        protected slots:
            virtual void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );
        protected:
            QList<qreal> m_weights;

        private:
            Q_DISABLE_COPY(PartBias)
    };


    class AMAROK_EXPORT PartBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
            virtual BiasPtr createBias( QXmlStreamReader *reader );
    };

}


#endif

