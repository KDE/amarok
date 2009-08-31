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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_DATABASEIMPORTERDIALOG_H
#define AMAROK_DATABASEIMPORTERDIALOG_H

#include <KAssistantDialog>    //baseclass

#include "databaseimporter/DatabaseImporter.h"
#include "meta/Meta.h"

#include <QHash>

class QAbstractButton;
class QButtonGroup;
class QRadioButton;
class QPlainTextEdit;
class KPageWidgetItem;
class KVBox;

class DatabaseImporterDialog : public KAssistantDialog
{
    Q_OBJECT

    public:
        DatabaseImporterDialog( QWidget *parent = 0 );
        ~DatabaseImporterDialog();
        
    private slots:
        void importFailed();
        void importSucceeded();
        void importError( QString error );
        void importedTrack( Meta::TrackPtr track );
        void discardedTrack( QString url );
        void matchedTrack( Meta::TrackPtr track, QString oldUrl );
        void ambigousTrack( Meta::TrackList tracks, QString oldUrl );
        void pageChanged( KPageWidgetItem *current, KPageWidgetItem *before );
        void showMessage( QString message );
    
    private:
        void selectImporter();

        QHash<QRadioButton*,QString> m_buttonHash;
        QButtonGroup     *m_buttons;
        DatabaseImporter *m_importer;
        DatabaseImporterConfig *m_importerConfig;

        KVBox            *m_configBox;
        KPageWidgetItem  *m_selectImporterPage;
        KPageWidgetItem  *m_configPage;
        KPageWidgetItem  *m_resultsPage;
        QPlainTextEdit   *m_results;
};

#endif

