/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef STATSYNCING_CHOOSEPROVIDERSPAGE_H
#define STATSYNCING_CHOOSEPROVIDERSPAGE_H

#include "ui_ChooseProvidersPage.h"

namespace StatSyncing
{
    class ProvidersModel;

    class ChooseProvidersPage : public QWidget, private Ui::ChooseProvidersPage
    {
        Q_OBJECT

        public:
            explicit ChooseProvidersPage( QWidget *parent = 0, Qt::WindowFlags f = 0 );
            virtual ~ChooseProvidersPage();

            void setFields( const QList<qint64> &fields, qint64 checkedFields );
            qint64 checkedFields() const;

            /**
             * Sets the model of providers to choose from. ChooseProvidersPage does _not_
             * take ownership of the model or the selection model.
             */
            void setProvidersModel( ProvidersModel *model, QItemSelectionModel *selectionModel );

        public Q_SLOTS:
            void disableControls();
            void setProgressBarText( const QString &text );
            void setProgressBarMaximum( int maximum );
            void progressBarIncrementProgress();

        Q_SIGNALS:
            void checkedFieldsChanged();

            /**
             * Emitted when user clicks the Next button.
             */
            void accepted();

            /**
             * Emitted when user pushes the Cancel button.
             */
            void rejected();

        private Q_SLOTS:
            void updateMatchedLabel();
            void updateEnabledFields();
            void openConfiguration();

        private:
            ProvidersModel *m_providersModel;
    };
} // namespace StatSyncing

#endif // STATSYNCING_CHOOSEPROVIDERSPAGE_H
