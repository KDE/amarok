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

#include "amarok_export.h"
#include "dynamic/Bias.h"

#include <QDialog>

class QVBoxLayout;
class QLabel;
class KComboBox;

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

        public Q_SLOTS:
            void accept();
            void reject();

        protected Q_SLOTS:
            /** Updates the list of biases in the bias type selection list */
            void factoriesChanged();
            /** Called when a new bias type has been selected */
            void selectionChanged( int index );
            void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );

        protected:

            QVBoxLayout* m_mainLayout;
            QVBoxLayout* m_biasLayout;

            KComboBox* m_biasSelection;
            QLabel *m_descriptionLabel;
            QWidget *m_biasWidget;

            Dynamic::BiasPtr m_origBias;

            /** A copy of the bias given when constructing this object.
             *
             *  We edit only in the copy so that we can discard it if needed.
             */
            Dynamic::BiasPtr m_bias;
    };

}

#endif
