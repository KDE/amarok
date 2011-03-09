/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2010,2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "shared/amarok_export.h"
#include "Bias.h"
#include "widgets/MetaQueryWidget.h"

#include <QDialog>

class QGridLayout;
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
    class PartBias;
    class TagMatchBias;
}

namespace PlaylistBrowserNS
{
    class BiasWidget;

    /** A dialog that contains the widget from a bias and allows to edit it.
    */
    class BiasDialog : public QDialog
    {
        Q_OBJECT

        public:
            BiasDialog( Dynamic::BiasPtr bias, QWidget* parent = 0 );
            virtual ~BiasDialog();

        protected slots:
            void factoriesChanged();
            void selectionChanged( int index );
            void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );

        protected:

            QVBoxLayout* m_mainLayout;
            QVBoxLayout* m_biasLayout;

            KComboBox* m_biasSelection;
            QLabel *m_descriptionLabel;
            QWidget *m_biasWidget;

            Dynamic::BiasPtr m_bias;
    };


    /** The widget for the PartBias */
    class PartBiasWidget : public QWidget
    {
        Q_OBJECT

        public:
            PartBiasWidget( Dynamic::PartBias* bias, QWidget* parent = 0 );

        protected slots:
            void appendBias();
            void biasAppended( Dynamic::BiasPtr bias );
            void biasRemoved( int pos );
            void biasMoved( int from, int to );

            void sliderValueChanged( int val );
            void biasWeightsChanged();

        protected:
            /** True if we just handle a signal. Used to protect agains recursion */
            bool m_inSignal;

            Dynamic::PartBias* m_bias;

            QGridLayout* m_layout;

            QList<QSlider*> m_sliders;
            QList<QWidget*> m_widgets;
    };


    class TagMatchBiasWidget : public QWidget
    {
        Q_OBJECT

        public:
            TagMatchBiasWidget( Dynamic::TagMatchBias* bias, QWidget* parent = 0 );

        private slots:
            void syncControlsToBias();
            void syncBiasToControls();

        private:
            MetaQueryWidget* m_queryWidget;

            Dynamic::TagMatchBias* m_bias;
    };

}

#endif
