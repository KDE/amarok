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

#include "core/support/Debug.h"
#include "databaseimporter/amarok14/FastForwardImporter.h"
#include "databaseimporter/itunes/ITunesImporter.h"

#include <KPageWidgetItem>
#include <KVBox>

#include <QButtonGroup>
#include <QLabel>
#include <QPlainTextEdit>
#include <QRadioButton>

DatabaseImporterDialog::DatabaseImporterDialog( QWidget *parent )
    : KAssistantDialog( parent )
    , m_importer( 0 )
    , m_importerConfig( 0 )
{
    setAttribute( Qt::WA_DeleteOnClose );
    setCaption( i18n( "Import Collection" ) );

    KVBox *importerBox = new KVBox( this );
    importerBox->setSpacing( KDialog::spacingHint() );

    QString text = i18n("This tool allows you to import track information and<br>statistical data from another music application.");
    text += i18n("<br><br>Any statistical data in your database will be <i>overwritten</i>" );
    QLabel *label = new QLabel( text, importerBox );
    label->setTextFormat( Qt::RichText );
    label->setAlignment( Qt::AlignHCenter );
    label->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum ); // Don't stretch vertically

    m_buttons = new QButtonGroup( importerBox );
    m_buttons->setExclusive( true );

    QRadioButton *amarok = new QRadioButton( i18n("Amarok 1.4"), importerBox );
    QRadioButton *itunes = new QRadioButton( i18n("iTunes"), importerBox );
    QRadioButton *banshee = new QRadioButton( i18n("Banshee"), importerBox );
    QRadioButton *rhythmbox = new QRadioButton( i18n("Rhythmbox"), importerBox );

    amarok->setChecked( true );
    itunes->setEnabled( true );
    banshee->setEnabled( false );
    rhythmbox->setEnabled( false );

    banshee->setHidden( true );
    rhythmbox->setHidden( true );

    m_buttons->addButton( amarok );
    m_buttons->addButton( itunes );
    m_buttons->addButton( banshee );
    m_buttons->addButton( rhythmbox );

    m_buttonHash.insert( amarok, FastForwardImporter::name() );
    m_buttonHash.insert( itunes, ITunesImporter::name() );
    m_buttonHash.insert( banshee, "" );
    m_buttonHash.insert( rhythmbox, "" );
    
    m_selectImporterPage = addPage( importerBox, i18n("Select Importer") );

    m_configBox = new KVBox( this );
    m_configBox->setSpacing( KDialog::spacingHint() );

    m_configPage = addPage( m_configBox, i18n("Configuration") );

    KVBox *resultBox = new KVBox( this );
    resultBox->setSpacing( KDialog::spacingHint() );
   
    m_results = new QPlainTextEdit( resultBox );
    m_results->setReadOnly( true );
    m_results->setTabChangesFocus( true );

    m_resultsPage = addPage( resultBox, i18n("Migrating") );

    connect( this, SIGNAL( currentPageChanged(KPageWidgetItem*, KPageWidgetItem*) ), SLOT( pageChanged( KPageWidgetItem*, KPageWidgetItem* ) ) );
}

DatabaseImporterDialog::~DatabaseImporterDialog()
{
    delete m_importer;
}

void
DatabaseImporterDialog::pageChanged( KPageWidgetItem *current, KPageWidgetItem *before )
{
    DEBUG_BLOCK

    if( before == m_selectImporterPage && current == m_configPage )
    {
        selectImporter();
        return;
    }

    if( before == m_configPage && current == m_resultsPage )
    {
        if( m_importer && !m_importer->importing() )
            m_importer->startImporting();

        enableButton( KDialog::User1, false );
        return;
    }
}

void
DatabaseImporterDialog::selectImporter()
{
    DEBUG_BLOCK

    QRadioButton *button = dynamic_cast<QRadioButton*>(m_buttons->checkedButton());
    if( !button )
        return;

    QString name = m_buttonHash.value( button, QString() );
    if( name.isEmpty() )
        return;

    delete m_importer;
    m_importer = DatabaseImporterFactory::createImporter( name, this );
    connect( m_importer, SIGNAL( importSucceeded() ), this, SLOT( importSucceeded() ) );
    connect( m_importer, SIGNAL( importFailed() ), this, SLOT( importFailed() ) );
    connect( m_importer, SIGNAL( trackAdded( Meta::TrackPtr ) ), this, SLOT( importedTrack( Meta::TrackPtr ) ) );
    connect( m_importer, SIGNAL( trackDiscarded( QString ) ), this, SLOT( discardedTrack( QString ) ) );
    connect( m_importer, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ),
             this, SLOT( matchedTrack( Meta::TrackPtr, QString ) ) );
    connect( m_importer, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ),
             this, SLOT( ambigousTrack( Meta::TrackList, QString ) ) );
    connect( m_importer, SIGNAL( importError( QString ) ), this, SLOT( importError( QString ) ) );
    connect( m_importer, SIGNAL( showMessage( QString ) ), this, SLOT( showMessage( QString ) ) );

    delete m_importerConfig;
    m_importerConfig = m_importer->configWidget( m_configBox );
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

    enableButton( KDialog::User1, true );
}

void
DatabaseImporterDialog::importFailed()
{
    QString text = i18n( "<b><font color='red'>Failed:</font></b> Unable to import statistics" );
    m_results->appendHtml( text );

    enableButton( KDialog::User1, true );
}

void
DatabaseImporterDialog::showMessage( QString message )
{
    m_results->appendHtml( message );
}

void
DatabaseImporterDialog::importError( QString error )
{
    QString text = i18n( "<b><font color='red'>Error:</font></b> %1", error );
    m_results->appendHtml( text );
}

void
DatabaseImporterDialog::importedTrack( Meta::TrackPtr track )
{
    if( !track ) return;

    QString text;
    if( !track->album() || track->album()->name().isEmpty() )
        text = i18nc( "Track has been imported, format: Artist - Track", 
                      "Imported <b>%1 - %2</b>", track->artist()->name(), track->name() );
    else
        text = i18nc( "Track has been imported, format: Artist - Track (Album)", 
                      "Imported <b>%1 - %2 (%3)</b>", track->artist()->name(), track->name(), track->album()->name() );
    m_results->appendHtml( text );
}

void DatabaseImporterDialog::discardedTrack( QString url )
{
    QString text;
    text = i18nc( "Track has been discarded, format: Url",
                  "Discarded <b><font color='gray'>%1</font></b>", url );
    m_results->appendHtml( text );
}

void DatabaseImporterDialog::matchedTrack( Meta::TrackPtr track, QString oldUrl )
{
    if( !track ) return;

    QString text;
    //TODO: help text; also check wording with imported; unify?
    if( !track->album() || track->album()->name().isEmpty() )
        text = i18nc( "Track has been imported by tags, format: Artist - Track, from Url, to Url",
                      "Imported <b><font color='green'>%1 - %2</font></b><br/>&nbsp;&nbsp;from %3<br/>&nbsp;&nbsp;to %4", track->artist()->name(), track->name(), oldUrl, track->prettyUrl() );
    else
        text = i18nc( "Track has been imported by tags, format: Artist - Track (Album), from Url, to Url",
                      "Imported <b><font color='green'>%1 - %2 (%3)</font></b><br/>&nbsp;&nbsp;from %4<br/>&nbsp;&nbsp;to %5", track->artist()->name(), track->name(), track->album()->name(), oldUrl, track->prettyUrl() );

    m_results->appendHtml( text );
}

void DatabaseImporterDialog::ambigousTrack( Meta::TrackList tracks, QString oldUrl )
{
    Q_UNUSED( tracks );

    QString text;
    // TODO: wording; etc.
    text = i18nc( "Track has been matched ambigously, format: Url",
                  "Multiple ambiguous matches found for <b><font color='red'>%1</font></b>, has been discarded.", oldUrl );
    m_results->appendHtml( text );
}

#include "DatabaseImporterDialog.moc"

