/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_DATABASEIMPORTERDIALOG_H
#define AMAROK_DATABASEIMPORTERDIALOG_H

#include <KAssistantDialog>    //baseclass

#include "core/meta/forward_declarations.h"

#include <QHash>
#include <QPushButton>

class QPlainTextEdit;
class KPageWidgetItem;
class BoxWidget;
class SqlBatchImporter;
class SqlBatchImporterConfig;

class DatabaseImporterDialog : public KAssistantDialog
{
    Q_OBJECT

    public:
        explicit DatabaseImporterDialog( QWidget *parent = nullptr );
        ~DatabaseImporterDialog() override;

    private Q_SLOTS:
        void importFailed();
        void importSucceeded();
        void importError( const QString &error );
        void importedTrack( Meta::TrackPtr track );
        void discardedTrack( const QString &url );
        void matchedTrack(Meta::TrackPtr track, const QString &oldUrl );
        void ambigousTrack( const Meta::TrackList &tracks, const QString &oldUrl );
        void pageChanged( KPageWidgetItem *current, KPageWidgetItem *before );
        void showMessage( const QString &message );

    private:
        SqlBatchImporter *m_importer;
        SqlBatchImporterConfig *m_importerConfig;

        BoxWidget        *m_configBox;
        KPageWidgetItem  *m_selectImporterPage;
        KPageWidgetItem  *m_configPage;
        KPageWidgetItem  *m_resultsPage;
        QPlainTextEdit   *m_results;
};

#endif

