/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <seb100@optusnet.com.au>       *
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
#include "partydialogbase.h"
#include "smartplaylist.h"
#include "statusbar.h"

#include <qfile.h>

#include <kapplication.h>
#include <klocale.h>
#include <kurllabel.h>

/////////////////////////////////////////////////////////////////////////////
///    CLASS Party
////////////////////////////////////////////////////////////////////////////

Party::Party( QString /*defaultName*/, QWidget *parent, const char *name )
    : KDialogBase( parent, name, false, 0, Ok|Cancel|Help )
{
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n("Configure Party Mode") ) );

    m_base = new PartyDialogBase(this);
    setMainWidget(m_base);

    m_lbSelected  = m_base->m_playlistSelector->selectedListBox();
    m_lbAvailable = m_base->m_playlistSelector->availableListBox();

    insertSelectedPlaylists();
    insertAvailablePlaylists();  //requires that insertSelectedPlaylists() has been called first.

    m_base->m_previousIntSpinBox->setEnabled( m_base->m_cycleTracks->isEnabled() );
    m_base->m_playlistSelector->setEnabled( m_base->m_playlistRadio->isEnabled() );

    // FIXME Showing a StatusBar longMessage doesn't suit very well for help texts
    connect( this, SIGNAL( helpClicked() ), SLOT( showHelp() ) );

    connect( m_base->m_playlistRadio, SIGNAL( toggled(bool) ), m_base->m_playlistSelector,     SLOT( setEnabled(bool) ) );
    connect( m_base->m_cycleTracks,   SIGNAL( toggled(bool) ), m_base->m_previousIntSpinBox,     SLOT( setEnabled(bool) ) );

    //update buttons
    connect( m_base->m_playlistSelector, SIGNAL( removed(QListBoxItem *) ), SLOT( updateButtons() ) );
    connect( m_base->m_playlistSelector, SIGNAL( added(QListBoxItem *) ),   SLOT( updateButtons() ) );
    connect( m_base->m_playlistRadio,    SIGNAL( toggled(bool) ),           SLOT( updateButtons() ) );

    applySettings();
}

QString Party::appendType()
{
    if( m_base->m_buttonGroup->selectedId() == 0 )
        return "Random";
    else if( m_base->m_buttonGroup->selectedId() == 1 )
        return "Suggestion";
    else
        return "Custom";
}

void Party::applySettings()
{
    m_base->m_partyCheck->setChecked( AmarokConfig::partyMode() );

    m_base->m_upcomingIntSpinBox->setValue( AmarokConfig::partyUpcomingCount() );
    m_base->m_previousIntSpinBox->setValue( AmarokConfig::partyPreviousCount() );
    m_base->m_appendCountIntSpinBox->setValue( AmarokConfig::partyAppendCount() );
    m_base->m_cycleTracks->setChecked( AmarokConfig::partyCycleTracks() );
    m_base->m_markHistory->setChecked( AmarokConfig::partyMarkHistory() );

    if ( AmarokConfig::partyType() == "Random" ) {
        m_base->m_randomRadio->setChecked( TRUE );
        m_base->m_suggestionRadio->setChecked( false );
        m_base->m_playlistRadio->setChecked( false );
    } else if ( AmarokConfig::partyType() == "Suggestion" ) {
        m_base->m_randomRadio->setChecked( false );
        m_base->m_suggestionRadio->setChecked( TRUE );
        m_base->m_playlistRadio->setChecked( false );
    } else {
        m_base->m_randomRadio->setChecked( false );
        m_base->m_suggestionRadio->setChecked( false );
        m_base->m_playlistRadio->setChecked( TRUE );
    }

    m_base->m_partyCheck->setChecked( AmarokConfig::partyMode() );
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

void Party::showHelp() //SLOT
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

void Party::updateButtons() //SLOT
{
    //disbale OK button if no item is in the selected listbox
    if( m_base->m_playlistRadio->isChecked() )
        enableButtonOK( m_lbSelected->firstItem() );
    else
        enableButtonOK( true );
}

bool    Party::isChecked()     { return m_base->m_partyCheck->isChecked(); }
int     Party::previousCount() { return m_base->m_previousIntSpinBox->value(); }
int     Party::upcomingCount() { return m_base->m_upcomingIntSpinBox->value(); }
int     Party::appendCount()   { return m_base->m_appendCountIntSpinBox->value(); }
bool    Party::cycleTracks()   { return m_base->m_cycleTracks->isChecked(); }
bool    Party::markHistory()   { return m_base->m_markHistory->isChecked(); }

#include "party.moc"
