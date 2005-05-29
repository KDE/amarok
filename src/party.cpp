/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "actionclasses.h"    //see toolbar construction
#include "amarok.h"
#include "amarokconfig.h"
#include "browserToolBar.h"
#include "collectiondb.h"
#include "party.h"
#include "partydialogbase.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "statusbar.h"

#include <qfile.h>

#include <kaction.h>
#include <kapplication.h>
#include <klocale.h>
#include <ktoolbar.h>
#include <kurllabel.h>

/////////////////////////////////////////////////////////////////////////////
///    CLASS Party
////////////////////////////////////////////////////////////////////////////

Party *Party::s_instance = 0;

Party::Party( QWidget *parent, const char *name )
    : QVBox( parent, name )
    , m_visible( false )
{
    s_instance = this;

    KToolBar *toolbar = new Browser::ToolBar( this );

    toolbar->setIconText( KToolBar::IconTextRight, false ); //text on right
    toolbar->insertButton( "edit_add", 0, true, i18n("Add Selected") );
    toolbar->insertButton( "edit_remove", 1, true, i18n("Remove") );
    toolbar->insertLineSeparator();
    toolbar->insertButton( "filesave", 2, true, i18n("Apply") );
    toolbar->insertLineSeparator();
    toolbar->insertButton( "up", 3, true, i18n("Show") );

    connect( (QObject*)toolbar->getButton( 0 ), SIGNAL(clicked( int )), SLOT( addPlaylists() ) );
    connect( (QObject*)toolbar->getButton( 1 ), SIGNAL(clicked( int )), SLOT( subPlaylists() ) );
    connect( (QObject*)toolbar->getButton( 2 ), SIGNAL(clicked( int )), SLOT( applySettings() ) );
    connect( (QObject*)toolbar->getButton( 3 ), SIGNAL(clicked( int )), SLOT( toggleVisibility() ) );

    m_base = new PartyDialogBase(this);

    m_playlists = m_base->m_playlistSelector;
    m_playlists->setSorting( 0 );
    m_playlists->addColumn( i18n("Playlists") );
    m_playlists->addColumn( i18n("Type") );
    m_playlists->setSelectionModeExt( KListView::Extended ); // Allow many files to be selected
    m_playlists->setColumnWidthMode( 0, QListView::Maximum );
    m_playlists->setColumnWidthMode( 1, QListView::Maximum );

    insertPlaylists();

    m_base->m_previousIntSpinBox->setEnabled( m_base->m_cycleTracks->isEnabled() );
    m_base->m_playlistSelector->setEnabled( m_base->m_appendType->currentItem() == 2 );

    connect( m_base->m_appendType,  SIGNAL( activated(int) ), SLOT( setAppendMode(int) ) );
    connect( m_base->m_cycleTracks, SIGNAL( toggled(bool) ), m_base->m_previousIntSpinBox, SLOT( setEnabled(bool) ) );

    connect( amaroK::actionCollection()->action( "party_mode" ), SIGNAL( toggled( bool ) ), SLOT( statusChanged( bool ) ) );

    restoreSettings();
}

void
Party::restoreSettings()
{
    m_base->m_partyCheck->setChecked( AmarokConfig::partyMode() );

    m_base->m_upcomingIntSpinBox->setValue( AmarokConfig::partyUpcomingCount() );
    m_base->m_previousIntSpinBox->setValue( AmarokConfig::partyPreviousCount() );
    m_base->m_appendCountIntSpinBox->setValue( AmarokConfig::partyAppendCount() );
    m_base->m_cycleTracks->setChecked( AmarokConfig::partyCycleTracks() );
    m_base->m_markHistory->setChecked( AmarokConfig::partyMarkHistory() );

    if ( AmarokConfig::partyType() == "Random" )
        m_base->m_appendType->setCurrentItem( RANDOM );

    else if ( AmarokConfig::partyType() == "Suggestion" )
        m_base->m_appendType->setCurrentItem( SUGGESTION );

    else // Custom
    {
        m_base->m_appendType->setCurrentItem( CUSTOM );
        m_playlists->setEnabled( true );
    }

    m_base->m_partyCheck->setChecked( AmarokConfig::partyMode() );
}

void
Party::loadConfig( PartyEntry *config )
{
    m_base->m_partyCheck->setChecked( true );

    m_base->m_upcomingIntSpinBox->setValue( config->upcoming() );
    m_base->m_previousIntSpinBox->setValue( config->previous() );
    m_base->m_appendCountIntSpinBox->setValue( config->appendCount() );
    m_base->m_cycleTracks->setChecked( config->isCycled() );
    m_base->m_markHistory->setChecked( config->isMarked() );
    m_base->m_appendType->setCurrentItem( config->appendType() );

    QStringList items = config->items();
    m_playlists->clear();
    QListViewItem *last=0;
    for( uint i=0; i < items.count(); i = i+2 )
        last = new QListViewItem( m_playlists, last, items[i], items[i+1] );

    applySettings();
}

void
Party::insertPlaylists()
{
    QStringList playlists = QStringList::split( ',' , AmarokConfig::partyCustomList() );
    QListViewItem *last=0;

    for( uint i=0; i < playlists.count(); i = i+2 )
        last = new QListViewItem( m_playlists, last, playlists[i], playlists[i+1] );
}

QString Party::customList()
{
    QString playlists;

    for( QListViewItem *it = m_playlists->firstChild(); it ; it = it->nextSibling() )
    {
        playlists.append( it->text(0) );
        playlists.append( ',' );
        playlists.append( it->text(1) );
        if ( it != m_playlists->lastItem() )
            playlists.append( ',' );
    }
    return playlists;
}

void
Party::addPlaylists() //SLOT
{
    QStringList selected = PlaylistBrowser::instance()->selectedList();

    QListViewItem *last = 0;
    for( uint i=0; i < selected.count(); i = i+2 )
        last = new KListViewItem( m_playlists, 0, selected[i], selected[i+1] );
}

void
Party::subPlaylists() //SLOT
{
    //assemble a list of what needs removing
    //calling removeItem() iteratively is more efficient if they are in _reverse_ order, hence the prepend()
    QPtrList<QListViewItem> list;
    QListViewItemIterator it( m_playlists, QListViewItemIterator::Selected);

    for( ; *it; list.prepend( *it ), ++it );

    if( list.isEmpty() ) return;

    //remove the items
    for( QListViewItem *item = list.first(); item; item = list.next() )
        delete item;
}

void
Party::setAppendMode( int id ) //SLOT
{
    bool enable = false;
    if( id == 2 ) enable = true;

    m_playlists->setEnabled( enable );
}

void
Party::applySettings() //SLOT
{
    //TODO this should be in app.cpp or the dialog's class implementation, here is not the right place

    if( CollectionDB::instance()->isEmpty() )
        return;

    amaroK::StatusBar::instance()->shortMessage( i18n("Party Configuration Saved.") );

    bool partyEnabled = isChecked();
    if ( partyEnabled != AmarokConfig::partyMode() )
    {
        static_cast<amaroK::PartyAction*>( amaroK::actionCollection()->action("party_mode") )->setChecked( partyEnabled );
        if ( !partyEnabled )
        {
            Playlist::instance()->alterHistoryItems( true, true ); //enable all items
            amaroK::actionCollection()->action( "prev" )->setEnabled( !AmarokConfig::partyMode() );
            return;
        }
    }
    QString type;
    if( appendType() == RANDOM )
        type = "Random";
    else if( appendType() == SUGGESTION )
        type = "Suggestion";
    else if( appendType() == CUSTOM )
        type = "Custom";

    AmarokConfig::setPartyType( type );

    if ( AmarokConfig::partyType() == "Custom" )
        AmarokConfig::setPartyCustomList( customList() );

    if ( AmarokConfig::partyPreviousCount() != previousCount() )
    {
        Playlist::instance()->adjustPartyPrevious( previousCount() );
        AmarokConfig::setPartyPreviousCount( previousCount() );
    }

    if ( AmarokConfig::partyUpcomingCount() != upcomingCount() )
    {
        AmarokConfig::setPartyUpcomingCount( upcomingCount() );
        Playlist::instance()->adjustPartyUpcoming( upcomingCount(), type );
    }

    AmarokConfig::setPartyCycleTracks( cycleTracks() );
    AmarokConfig::setPartyAppendCount( appendCount() );
    AmarokConfig::setPartyMarkHistory( markHistory() );

    amaroK::actionCollection()->action( "prev" )->setEnabled( !AmarokConfig::partyMode() );
    amaroK::actionCollection()->action( "random_mode" )->setEnabled( false );
}

void
Party::statusChanged( bool enable ) // SLOT
{
    if( !enable )
        Playlist::instance()->alterHistoryItems( true, true ); //enable all items

    m_base->m_partyCheck->setChecked( enable );
    applySettings();
}

void
Party::toggleVisibility()
{
    if( m_visible )
    {
        m_base->setHidden( true );
        m_visible = false;
    }
    else
    {
        m_base->setShown( true );
        m_visible = true;
    }
}

bool    Party::isChecked()     { return m_base->m_partyCheck->isChecked(); }
int     Party::previousCount() { return m_base->m_previousIntSpinBox->value(); }
int     Party::upcomingCount() { return m_base->m_upcomingIntSpinBox->value(); }
int     Party::appendCount()   { return m_base->m_appendCountIntSpinBox->value(); }
int     Party::appendType()    { return m_base->m_appendType->currentItem(); }
bool    Party::cycleTracks()   { return m_base->m_cycleTracks->isChecked(); }
bool    Party::markHistory()   { return m_base->m_markHistory->isChecked(); }

#include "party.moc"
