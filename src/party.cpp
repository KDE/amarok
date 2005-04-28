/***************************************************************************
  copyright            : (C) Seb Ruiz <seb100@optusnet.com.au>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "party.h"
#include "smartplaylist.h"
#include "statusbar.h"

#include <qbuttongroup.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qradiobutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>

#include <kactionselector.h>
#include <klocale.h>
#include <kurllabel.h>

/////////////////////////////////////////////////////////////////////////////
///    CLASS Party
////////////////////////////////////////////////////////////////////////////

Party::Party( QString /*defaultName*/, QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Start a Party"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QVBox *partyBox = new QVBox( mainWidget() );
    QHBox *partyBoxT = new QHBox( partyBox );
    m_partyCheck = new QCheckBox( i18n("Enable party mode" ), partyBoxT );

    KURLLabel *helpButton = new KURLLabel( partyBoxT );
    helpButton->setText( i18n("Help") );

    m_partyGroupBox = new QVGroupBox( QString::null, mainWidget() );
    new QLabel( i18n("Type of song to append to playlist:"), m_partyGroupBox );
    QVBox *selectorBox = new QVBox( m_partyGroupBox );
    partyBox->setSpacing( 5 );

    m_buttonGroup = new QButtonGroup( i18n("Append Song Type") );

    m_randomRadio = new QRadioButton( i18n("Random Song"), selectorBox );
    m_buttonGroup->insert( m_randomRadio );

    m_suggestionRadio = new QRadioButton( i18n("Suggested Song"), selectorBox );;
    m_buttonGroup->insert( m_suggestionRadio );

    m_playlistRadio = new QRadioButton( i18n("Random song from one of the following:"), selectorBox );
    m_buttonGroup->insert( m_playlistRadio );

    QVBox *playlistBox = new QVBox( selectorBox );

    m_playlistSelector = new KActionSelector( playlistBox );
    m_playlistSelector->setShowUpDownButtons( false );
    m_playlistSelector->setAvailableLabel( i18n("Available playlists:") );
    m_playlistSelector->setSelectedLabel( i18n("Selected playlists:") );

    m_lbSelected  = m_playlistSelector->selectedListBox();
    m_lbAvailable = m_playlistSelector->availableListBox();

    insertSelectedPlaylists();
    insertAvailablePlaylists();  //requires that insertSelectedPlaylists() has been called first.

    QVBox *optionBox = new QVBox( m_partyGroupBox );
    optionBox->setSpacing( 5 );

    m_cycleTracks = new QCheckBox( i18n("Cycle Tracks"), optionBox );

    QHBox *previousBox = new QHBox( optionBox );
    new QLabel( i18n("Maximum history to show:"), previousBox );
    m_previousIntSpinBox = new KIntSpinBox( previousBox );

    QHBox *upcomingBox = new QHBox( optionBox );
    new QLabel( i18n("Minimum upcoming tracks:"), upcomingBox );
    m_upcomingIntSpinBox = new KIntSpinBox( upcomingBox );

    QHBox *appendBox = new QHBox( optionBox );
    new QLabel( i18n("Number of tracks to append:"), appendBox );
    m_tracksToAddSpinBox = new KIntSpinBox( appendBox );

    previousBox->setEnabled( m_cycleTracks->isEnabled() );
    playlistBox->setEnabled( m_playlistRadio->isEnabled() );

    connect( m_partyCheck,    SIGNAL( toggled(bool) ), m_partyGroupBox, SLOT( setEnabled(bool) ) );
    connect( m_playlistRadio, SIGNAL( toggled(bool) ), playlistBox,     SLOT( setEnabled(bool) ) );
    connect( m_cycleTracks,   SIGNAL( toggled(bool) ), previousBox,     SLOT( setEnabled(bool) ) );
    connect( helpButton,      SIGNAL( leftClickedURL() ), SLOT( showHelp() ) );

    applySettings();
}

QString Party::appendType()
{
    if( m_buttonGroup->selectedId() == 0 )
        return "Random";
    else if( m_buttonGroup->selectedId() == 1 )
        return "Suggestion";
    else
        return "Custom";
}

void Party::applySettings()
{
    m_partyCheck->setChecked( AmarokConfig::partyMode() );

    m_upcomingIntSpinBox->setValue( AmarokConfig::partyUpcomingCount() );
    m_previousIntSpinBox->setValue( AmarokConfig::partyPreviousCount() );
    m_tracksToAddSpinBox->setValue( AmarokConfig::partyAppendCount() );
    m_cycleTracks->setChecked( AmarokConfig::partyCycleTracks() );

    if ( AmarokConfig::partyType() == "Random" ) {
        m_randomRadio->setChecked( TRUE );
        m_suggestionRadio->setChecked( false );
        m_playlistRadio->setChecked( false );
    } else if ( AmarokConfig::partyType() == "Suggestion" ) {
        m_randomRadio->setChecked( false );
        m_suggestionRadio->setChecked( TRUE );
        m_playlistRadio->setChecked( false );
    } else {
        m_randomRadio->setChecked( false );
        m_suggestionRadio->setChecked( false );
        m_playlistRadio->setChecked( TRUE );
    }

    if ( AmarokConfig::partyMode() ) {
        m_partyCheck->setChecked( true );
        m_partyGroupBox->setEnabled( true );
    } else {
        m_partyCheck->setChecked( false );
        m_partyGroupBox->setEnabled( false );
    }
}

void Party::insertAvailablePlaylists()
{
    //Default playlists.
    QStringList defaultPlaylists;
    defaultPlaylists << i18n( "Favorite Tracks" ) << i18n( "Most Played" )  << i18n( "Newest Tracks" )
                     << i18n( "Last Played" )     << i18n( "Never Played" ) << i18n( "Ever Played" )
                     << i18n( "50 Random Tracks" );


    //FIXME: Genres?  Too many to list, need an expander.
    for ( uint i=0; i < defaultPlaylists.count(); i++ )
    {
        // dont add if in selected list.
        // critical that insertSelectedPlaylists() is called first.
        if ( m_lbSelected->findItem( defaultPlaylists[i] ) )
            continue;

        m_lbAvailable->insertItem( defaultPlaylists[i] );
    }

    //Custom playlists.
    QFile file( amaroK::saveLocation() + "smartplaylists" );

    if( file.open( IO_ReadOnly ) )
    {
        QTextStream stream( &file );
        QString line, name, query;

        while( !( line = stream.readLine() ).isNull() )
        {
            if( line.startsWith( "Name=" ) )
                m_lbAvailable->insertItem( line.mid( 5 ) );
        }
    }
}

void Party::insertSelectedPlaylists()
{
    QStringList playlists = QStringList::split( ',' , AmarokConfig::partyCustomList() );

    m_lbSelected->insertStringList( playlists );
}

QString Party::customList()
{
    QString playlists;
    for ( uint i=0; i < m_lbSelected->count(); ++i )
    {
        playlists.append( m_lbSelected->text( i ) );
        if ( i != m_lbSelected->count() - 1 )  playlists.append( ',' );
    }
    return playlists;
}

void Party::showHelp()
{
    amaroK::StatusBar::instance()->longMessage( i18n(
        "<div align=\"center\"><b>Party Mode</b></div><br>"
        "<p>Party mode is a complex playlist handling mechanism - acting "
        "basically on the concept of a 'rotating' playlist.  The playlist can "
        "be modelled as a queuing system, FIFO.  As a track is advanced, "
        "the first track in the playlist is removed, and another appended to "
        "the end.  The type of addition is selected by the user during "
        "configuration.</p>" ) );
}

#include "party.moc"
