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

/////////////////////////////////////////////////////////////////////////////
///    CLASS Party
////////////////////////////////////////////////////////////////////////////

#include "amarok.h"
#include "amarokconfig.h"
#include "party.h"
#include "smartplaylist.h"

#include <qlabel.h>
#include <qvbox.h>

#include <klocale.h>

Party::Party( QString defaultName, QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Start a Party"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QVBox *partyBox = new QVBox( mainWidget() );
    m_partyCheck = new QCheckBox( i18n("Enable party mode" ), partyBox );
    m_partyCheck->setChecked( AmarokConfig::partyMode() );

    QVGroupBox *partyGroupBox = new QVGroupBox( QString::null, mainWidget() );
    new QLabel( i18n("Type of song to append to playlist:"), partyGroupBox );
    QVBox *selectorBox = new QVBox( partyGroupBox );
    partyBox->setSpacing( 5 );

    m_buttonGroup = new QButtonGroup( i18n("Append Song Type") );

    m_randomRadio = new QRadioButton( i18n("Random Song"), selectorBox );
    m_buttonGroup->insert( m_randomRadio );

    m_suggestionRadio = new QRadioButton( i18n("Suggested Song"), selectorBox );;
    m_buttonGroup->insert( m_suggestionRadio );

    QVBox *tempDisable = new QVBox( selectorBox );
    m_playlistRadio = new QRadioButton( i18n("Random song from one of the following:"), tempDisable/*selectorBox */);
    m_playlistRadio->setChecked( false );
//     m_buttonGroup->insert( m_playlistRadio );

    if ( AmarokConfig::partyType() == "Random" )
        m_randomRadio->setChecked( true );
    else if ( AmarokConfig::partyType() == "Suggestion" )
        m_suggestionRadio->setChecked( true );
    else
        m_playlistRadio->setChecked( true );

    QVBox *playlistBox = new QVBox( tempDisable/*selectorBox*/ );

    m_playlistSelector = new KActionSelector( playlistBox );
    m_playlistSelector->setShowUpDownButtons( false );
    m_playlistSelector->setAvailableLabel( i18n("Available playlists:") );
    m_playlistSelector->setSelectedLabel( i18n("Selected playlists:") );

    m_lbSelected  = m_playlistSelector->selectedListBox();
    m_lbAvailable = m_playlistSelector->availableListBox();

    insertSelectedPlaylists();
    insertAvailablePlaylists();  //requires that insertSelectedPlaylists() has been called first.

    QHBox *upcomingBox = new QHBox( partyGroupBox );
    new QLabel( i18n("Number of upcoming songs to show:"), upcomingBox );
    m_upcomingIntSpinBox = new KIntSpinBox( upcomingBox );
    m_upcomingIntSpinBox->setValue( AmarokConfig::partyUpcomingCount() );

    QHBox *previousBox = new QHBox( partyGroupBox );
    new QLabel( i18n("Number of previous songs to show:"), previousBox );
    m_previousIntSpinBox = new KIntSpinBox( previousBox );
    m_previousIntSpinBox->setValue( AmarokConfig::partyPreviousCount() );

    if ( !m_playlistRadio->isEnabled() )
        playlistBox->setEnabled( false );

    tempDisable->setEnabled( false );

    connect( m_partyCheck, SIGNAL( toggled(bool) ), partyGroupBox,  SLOT( setEnabled(bool) ) );
    connect( m_playlistRadio, SIGNAL( toggled(bool) ), playlistBox, SLOT( setEnabled(bool) ) );

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

//NOTE: Maybe refactor to have a complex create smart playlist type chooser.
void Party::insertAvailablePlaylists()
{
    //Default playlists.
    QStringList defaultPlaylists;
    defaultPlaylists << i18n( "Favorite Tracks" ) << i18n( "Most Played" )  << i18n( "Newest Tracks" )
                     << i18n( "Last Played" )     << i18n( "Never Played" ) << i18n( "Ever Played" )
                     << i18n( "Random" );


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


#include "party.moc"
