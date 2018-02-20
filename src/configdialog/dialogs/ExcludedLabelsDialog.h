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

#ifndef EXCLUDEDLABELSDIALOG_H
#define EXCLUDEDLABELSDIALOG_H

#include "ui_ExcludedLabelsDialog.h"
#include "core/meta/forward_declarations.h"

#include <QDialog>

namespace StatSyncing {
    class Config;
}
class QLineEdit;
class QGridLayout;
class QListWidget;

class ExcludedLabelsDialog : public QDialog, private Ui_ExcludedLabelsDialog
{
    Q_OBJECT

    public:
        explicit ExcludedLabelsDialog( StatSyncing::Config *config, QWidget *parent = Q_NULLPTR,
                                       Qt::WindowFlags flags = Qt::WindowFlags() );

    private Q_SLOTS:
        void slowNewResultReady( const Meta::LabelList &labels );
        void slotAddExcludedLabel();
        void slotSaveToConfig();

    private:
        void addLabel( const QString &label, bool selected = false );
        void addLabels( const QSet<QString> &labels, bool selected = false );

        StatSyncing::Config *m_statSyncingConfig;
};

#endif // EXCLUDEDLABELSDIALOG_H
