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
class QEvent;

namespace Amarok
{
    class Slider;
}

namespace Dynamic
{
    class TagMatchBias;
}

namespace PlaylistBrowserNS
{
    class BiasWidget;

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

            BiasWidget* biasWidget()
            {
                return qobject_cast<BiasWidget*>(m_biasWidget);
            }

        protected slots:
            void biasRemoved();
            void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );

        protected:
            /** Returns a row number for this bias.
                Used to display the alternate background for the widgets and
                it's not really the row number but instead something that looks
                good. */
            int rowNumber() const;

            // void resizeEvent( QResizeEvent* );
            void paintEvent( QPaintEvent *event );
            void enterEvent ( QEvent *event );
            void leaveEvent ( QEvent *event );

            QHBoxLayout* m_mainLayout;
            Dynamic::BiasPtr m_bias;
            QWidget *m_biasWidget;

        private:
            QToolButton* m_removeButton;
            bool m_hover;
    };

    class BiasWidget : public QWidget
    {
        Q_OBJECT

        public:
            BiasWidget( Dynamic::BiasPtr bias, QWidget* parent = 0 );

            /** Set's a widget that is handled as part of the bias widget.
                Example: The weight slider in case the bias is inside a
                PartBias.
                The widget will belong to this object afterwards.
            */
            void setCustomWidget( const QString &label, QWidget* widget );

        protected slots:
            void factoriesChanged();
            void selectionChanged( int index );

        protected:
            Dynamic::BiasPtr m_bias;
            QWidget* m_customWidget;
            KComboBox* m_biasSelection;

            /** This is where sub-classes are added */
            QFormLayout* m_layout;
    };

    /** A bias that has sub biases like the AndBias or PartBias */
    class LevelBiasWidget : public BiasWidget
    {
        Q_OBJECT

        public:
            LevelBiasWidget( Dynamic::AndBias* bias, bool haveWeights,
                             QWidget* parent = 0 );

            QList<BiasBoxWidget*> widgets() const
            { return m_widgets; }

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
