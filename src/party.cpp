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
    , m_visible( true )
{
    s_instance = this;

    //<Toolbar>
    m_ac = new KActionCollection( this );

    m_addButton  = new KAction( i18n("Add Selected"), "edit_add", 0, this, SLOT( addPlaylists() ), m_ac, "Add Selected" );
    m_subButton  = new KAction( i18n("Remove"), "edit_remove", 0, this, SLOT( subPlaylists() ), m_ac, "Remove" );

    m_applyButton = new KAction( i18n("Apply"), "apply", 0, this, SLOT( applySettings() ), m_ac, "Apply Settings" );

    m_toolbar = new Browser::ToolBar( this );
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the open button to have text on right

    m_addButton->plug( m_toolbar );
    m_subButton->plug( m_toolbar );
    m_toolbar->insertLineSeparator();
    m_applyButton->plug( m_toolbar );


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

    //Update buttons
    connect( m_base->m_appendCountIntSpinBox, SIGNAL( valueChanged( int ) ), SLOT( updateApplyButton() ) );
    connect( m_base->m_previousIntSpinBox,    SIGNAL( valueChanged( int ) ), SLOT( updateApplyButton() ) );
    connect( m_base->m_upcomingIntSpinBox,    SIGNAL( valueChanged( int ) ), SLOT( updateApplyButton() ) );
    connect( m_base->m_cycleTracks,   SIGNAL( stateChanged( int ) ), SLOT( updateApplyButton() ) );
    connect( m_base->m_markHistory,   SIGNAL( stateChanged( int ) ), SLOT( updateApplyButton() ) );
    connect( m_base->m_appendType,    SIGNAL( activated( int ) ),    SLOT( updateApplyButton() ) );

    connect( m_playlists, SIGNAL( selectionChanged() ), SLOT( updateRemoveButton() ) );

    restoreSettings();
}

void
Party::restoreSettings()
{
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

    m_applyButton->setEnabled( false );
    m_addButton->setEnabled( false );
    m_subButton->setEnabled( false );
}

void
Party::loadConfig( PartyEntry *config )
{
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

/// We must make sure that we set m_selected accordingly prior to calling this function.
void
Party::subPlaylists() //SLOT
{
    if( m_selected.isEmpty() ) return;

    //remove the items
    for( QListViewItem *item = m_selected.first(); item; item = m_selected.next() )
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

    bool partyEnabled = PlaylistBrowser::instance()->dynamicEnabled();
    if ( partyEnabled != AmarokConfig::partyMode() )
    {
        static_cast<amaroK::DynamicAction*>( amaroK::actionCollection()->action("dynamic_mode") )->setChecked( partyEnabled );
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
    amaroK::actionCollection()->action( "random_mode" )->setEnabled( !AmarokConfig::partyMode() );
}

void
Party::statusChanged( bool enable ) // SLOT
{
    if( !enable )
        Playlist::instance()->alterHistoryItems( true, true ); //enable all items

    applySettings();
}

void
Party::updateApplyButton() //SLOT
{
    if( cycleTracks() != AmarokConfig::partyCycleTracks() ||
        markHistory() != AmarokConfig::partyMarkHistory() ||
        previousCount() != AmarokConfig::partyPreviousCount() ||
        upcomingCount() != AmarokConfig::partyUpcomingCount() )
    {
        m_applyButton->setEnabled( true );
        return;
    }

    QString type = AmarokConfig::partyType();
    int typeValue = CUSTOM;

    if( type == "Random" )          typeValue = RANDOM;
    else if( type == "Suggestion" ) typeValue = SUGGESTION;

    if( typeValue != appendType() )
        m_applyButton->setEnabled( true );
    else
        m_applyButton->setEnabled( false );

}

void
Party::updateAddButton() //SLOT
{
    if( !PlaylistBrowser::instance()->selectedList().isEmpty() )
        m_addButton->setEnabled( true );
    else
        m_addButton->setEnabled( false );
}

void
Party::updateRemoveButton() //SLOT
{
    //assemble a list of what needs removing
    //calling removeItem() iteratively is more efficient if they are in _reverse_ order, hence the prepend()
    m_selected.clear();
    QListViewItemIterator it( m_playlists, QListViewItemIterator::Selected);

    for( ; *it; m_selected.prepend( *it ), ++it );

    if( m_selected.isEmpty() )
        m_subButton->setEnabled( false );
    else
        m_subButton->setEnabled( true );
}

int     Party::previousCount() { return m_base->m_previousIntSpinBox->value(); }
int     Party::upcomingCount() { return m_base->m_upcomingIntSpinBox->value(); }
int     Party::appendCount()   { return m_base->m_appendCountIntSpinBox->value(); }
int     Party::appendType()    { return m_base->m_appendType->currentItem(); }
bool    Party::cycleTracks()   { return m_base->m_cycleTracks->isChecked(); }
bool    Party::markHistory()   { return m_base->m_markHistory->isChecked(); }

#include "party.moc"
