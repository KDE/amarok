/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_DYNAMICBIASWIDGETS_H
#define AMAROK_DYNAMICBIASWIDGETS_H

#include "Bias.h"
#include "widgets/MetaQueryWidget.h"

#include <QWidget>

class QFormLayout;
class QLabel;
class QSlider;
class QToolButton;
class KComboBox;

namespace Amarok
{
    class Slider;
}


namespace PlaylistBrowserNS
{
    /** This widget encapsulates the real bias widgets and exchanges them if needed.
        Also this widget will display a "list like" background.
    */
    class BiasBoxWidget : public QWidget
    {
        Q_OBJECT

        public:
            BiasBoxWidget( Dynamic::BiasPtr bias, QWidget* parent = 0 );
            virtual ~BiasBoxWidget() {}

            void setRemovable( bool );

        protected slots:
            void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );

        protected:
            // void resizeEvent( QResizeEvent* );
            void paintEvent( QPaintEvent* );

            QHBoxLayout* m_mainLayout;
            QWidget *m_biasWidget;
            bool m_removable;
    };

    class BiasWidget : public QWidget
    {
        Q_OBJECT

        public:
            BiasWidget( Dynamic::BiasPtr bias, QWidget* parent = 0 );

            void setRemovable( bool value );

        private slots:
            void factoriesChanged();
            void selectionChanged( int index );
            void biasRemoved();

        protected: //protected data members make Mike cry :'(
            Dynamic::BiasPtr m_bias;

            /** This is where sub-classes can add their stuff */
            QFormLayout* m_layout;

        private:
            KComboBox* m_biasSelection;
            QToolButton* m_removeButton;
    };

    /** A bias that has sub biases like the AndBias */
    class LevelBiasWidget : public BiasWidget
    {
        Q_OBJECT

        public:
            LevelBiasWidget( Dynamic::AndBias* bias, bool haveWeights,
                             QWidget* parent = 0 );

        signals:
            void biasWeightChanged( int biasNum, qreal value );

        protected slots:
            void appendBias();
            void biasAppended( Dynamic::BiasPtr bias );
            void biasRemoved( int pos );
            void biasMoved( int from, int to );

            void sliderValueChanged( int val );
            void biasWeightsChanged();

        protected:
            /** calls setRemovable() for all sub-widgets */
            void correctRemovability();

            bool m_haveWeights;

            /** True if we just handle a signal. Used to protect agains recursion */
            bool m_inSignal;

            Dynamic::AndBias* m_abias;

            QSlider *m_weightSelection;
            QList<BiasBoxWidget*> m_widgets;
            QList<QSlider*> m_sliders;

            QToolButton* m_addButton;
    };


    class TagMatchBiasWidget : public BiasWidget
    {
        Q_OBJECT

        public:
            TagMatchBiasWidget( Dynamic::TagMatchBias* bias, QWidget* parent = 0 );

        private slots:
            void syncControlsToBias();
            void syncBiasToControls();

        private:
            Amarok::Slider*  m_scaleSelection;
            QLabel*          m_scaleLabel;
            MetaQueryWidget* m_queryWidget;

            Dynamic::TagMatchBias* m_tbias;
    };

}

#endif
