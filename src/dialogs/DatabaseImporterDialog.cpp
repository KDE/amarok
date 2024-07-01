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

#include "DatabaseImporterDialog.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "databaseimporter/SqlBatchImporter.h"
#include "databaseimporter/SqlBatchImporterConfig.h"

#include <KPageWidgetItem>
#include <KLocalizedString>

#include <QBoxLayout>
#include <QDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStyle>

DatabaseImporterDialog::DatabaseImporterDialog( QWidget *parent )
    : KAssistantDialog( parent )
    , m_importer( nullptr )
    , m_importerConfig( nullptr )
{
    setAttribute( Qt::WA_DeleteOnClose );
    QWidget::setWindowTitle( i18n( "Import Collection" ) );

    BoxWidget *importerBox = new BoxWidget( true, this );
    importerBox->layout()->setSpacing( style()->layoutSpacing( QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Vertical ) );

    m_configBox = new BoxWidget( true, this );
    m_configBox->layout()->setSpacing( style()->layoutSpacing( QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Vertical ) );

    m_configPage = addPage( m_configBox, i18n("Import configuration") );

    m_importer = new SqlBatchImporter( this );
    connect( m_importer, &SqlBatchImporter::importSucceeded, this, &DatabaseImporterDialog::importSucceeded );
    connect( m_importer, &SqlBatchImporter::importFailed, this, &DatabaseImporterDialog::importFailed );
    connect( m_importer, &SqlBatchImporter::trackAdded, this, &DatabaseImporterDialog::importedTrack );
    connect( m_importer, &SqlBatchImporter::trackDiscarded, this, &DatabaseImporterDialog::discardedTrack );
    connect( m_importer, &SqlBatchImporter::trackMatchFound, this, &DatabaseImporterDialog::matchedTrack );
    connect( m_importer, &SqlBatchImporter::trackMatchMultiple, this, &DatabaseImporterDialog::ambigousTrack );
    connect( m_importer, &SqlBatchImporter::importError, this, &DatabaseImporterDialog::importError );
    connect( m_importer, &SqlBatchImporter::showMessage, this, &DatabaseImporterDialog::showMessage );
    m_importerConfig = m_importer->configWidget( m_configBox );

    BoxWidget *resultBox = new BoxWidget( true, this );
    resultBox->layout()->setSpacing( style()->layoutSpacing( QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Vertical ) );

    m_results = new QPlainTextEdit( resultBox );
    m_results->setReadOnly( true );
    m_results->setTabChangesFocus( true );

    m_resultsPage = addPage( resultBox, i18n("Migrating") );

    connect( this, &DatabaseImporterDialog::currentPageChanged, this, &DatabaseImporterDialog::pageChanged );
}

DatabaseImporterDialog::~DatabaseImporterDialog()
{
    delete m_importer;
}

void
DatabaseImporterDialog::pageChanged( KPageWidgetItem *current, KPageWidgetItem *before )
{
    DEBUG_BLOCK

    if( before == m_configPage && current == m_resultsPage )
    {
        if( m_importer && !m_importer->importing() )
            m_importer->startImporting();

        QPushButton* user1Button = new QPushButton();
        user1Button->setEnabled( false );
        return;
    }
}

void
DatabaseImporterDialog::importSucceeded()
{
    // Special case the 0 import track count as it is really a failure
    QString text;
    if( !m_importer->importedCount() )
        text = i18n( "<b><font color='red'>Failed:</font></b> No tracks were imported" );
    else
        text = i18np( "<b><font color='green'>Success:</font></b> Imported %1 track",
                      "<b><font color='green'>Success:</font></b> Imported %1 tracks", m_importer->importedCount() );

    m_results->appendHtml( text );

    QPushButton* user1Button = new QPushButton();
    user1Button->setEnabled( true );
}

void
DatabaseImporterDialog::importFailed()
{
    QString text = i18n( "<b><font color='red'>Failed:</font></b> Unable to import statistics" );
    m_results->appendHtml( text );

    QPushButton* user1Button = new QPushButton();
    user1Button->setEnabled( true );

}

void
DatabaseImporterDialog::showMessage( const QString &message )
{
    m_results->appendHtml( message );
}

void
DatabaseImporterDialog::importError( const QString &error )
{
    QString text = i18n( "<b><font color='red'>Error:</font></b> %1", error );
    m_results->appendHtml( text );
}

void
DatabaseImporterDialog::importedTrack( Meta::TrackPtr track )
{
    if( !track ) return;

    QString text;
    Meta::ArtistPtr artist = track->artist();
    Meta::AlbumPtr album = track->album();

    if( !artist || artist->name().isEmpty() )
        text = i18nc( "Track has been imported, format: Track",
                      "Imported <b>%1</b>", track->name() );
    else if( !album || album->name().isEmpty() )
        text = i18nc( "Track has been imported, format: Artist - Track",
                      "Imported <b>%1 - %2</b>", artist->name(), track->name() );
    else
        text = i18nc( "Track has been imported, format: Artist - Track (Album)",
                      "Imported <b>%1 - %2 (%3)</b>", artist->name(), track->name(), album->name() );

    m_results->appendHtml( text );
}

void DatabaseImporterDialog::discardedTrack( const QString &url )
{
    QString text;
    text = i18nc( "Track has been discarded, format: Url",
                  "Discarded <b><font color='gray'>%1</font></b>", url );
    m_results->appendHtml( text );
}

void DatabaseImporterDialog::matchedTrack( Meta::TrackPtr track, const QString &oldUrl )
{
    if( !track ) return;

    QString text;
    Meta::ArtistPtr artist = track->artist();
    Meta::AlbumPtr album = track->album();

    //TODO: help text; also check wording with imported; unify?
    if( !artist || artist->name().isEmpty() )
        text = i18nc( "Track has been imported by tags, format: Track, from Url, to Url",
                      "Imported <b><font color='green'>%1</font></b><br/>&nbsp;&nbsp;from %2<br/>&nbsp;&nbsp;to %3", track->name(), oldUrl, track->prettyUrl() );
    else if( !album || album->name().isEmpty() )
        text = i18nc( "Track has been imported by tags, format: Artist - Track, from Url, to Url",
                      "Imported <b><font color='green'>%1 - %2</font></b><br/>&nbsp;&nbsp;from %3<br/>&nbsp;&nbsp;to %4", artist->name(), track->name(), oldUrl, track->prettyUrl() );
    else
        text = i18nc( "Track has been imported by tags, format: Artist - Track (Album), from Url, to Url",
                      "Imported <b><font color='green'>%1 - %2 (%3)</font></b><br/>&nbsp;&nbsp;from %4<br/>&nbsp;&nbsp;to %5", artist->name(), track->name(), album->name(), oldUrl, track->prettyUrl() );

    m_results->appendHtml( text );
}

void DatabaseImporterDialog::ambigousTrack( const Meta::TrackList &tracks, const QString &oldUrl )
{
    Q_UNUSED( tracks );

    QString text;
    // TODO: wording; etc.
    text = i18nc( "Track has been matched ambiguously, format: Url",
                  "Multiple ambiguous matches found for <b><font color='red'>%1</font></b>, has been discarded.", oldUrl );
    m_results->appendHtml( text );
}


