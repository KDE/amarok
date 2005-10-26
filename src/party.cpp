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
#include "debug.h"
#include "party.h"
#include "partydialogbase.h"
#include "playlist.h"
#include "playlistbrowser.h"
#include "statusbar.h"

#include <qcheckbox.h>
#include <qfile.h>

#include <kaction.h>
#include <kapplication.h>
#include <kiconloader.h>       //smallIcon
#include <klocale.h>
#include <knuminput.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <ktoolbar.h>
#include <kurldrag.h>          //dragObject()
#include <kurllabel.h>

/////////////////////////////////////////////////////////////////////////////
///    CLASS Party
////////////////////////////////////////////////////////////////////////////

Party *Party::s_instance = 0;

Party::Party( QWidget *parent, const char *name )
    : QVBox( parent, name )
    , m_ac( new KActionCollection( this ) )
    , m_visible( true )
{
    s_instance = this;

    QFrame *container = new QVBox( this, "container" );
    container->hide();

    //<Toolbar>
    m_repopulate = new KAction( i18n("Repopulate"), "rebuild", 0,
                                       this, SLOT( repopulate() ), m_ac, "Repopulate Upcoming Tracks" );

    m_toolbar = new Browser::ToolBar( container );
    m_toolbar->setIconText( KToolBar::IconTextRight, false ); //we want the buttons to have text on right
    m_repopulate->plug( m_toolbar );

    m_base = new PartyDialogBase( container );
    m_base->m_previousIntSpinBox->setEnabled( m_base->m_cycleTracks->isEnabled() );

    connect( m_base->m_cycleTracks, SIGNAL( toggled(bool) ), m_base->m_previousIntSpinBox, SLOT( setEnabled(bool) ) );

    //Update buttons
    connect( m_base->m_appendCountIntSpinBox, SIGNAL( valueChanged( int ) ), SLOT( applySettings() ) );
    connect( m_base->m_previousIntSpinBox,    SIGNAL( valueChanged( int ) ), SLOT( applySettings() ) );
    connect( m_base->m_upcomingIntSpinBox,    SIGNAL( valueChanged( int ) ), SLOT( applySettings() ) );
    connect( m_base->m_cycleTracks,           SIGNAL( stateChanged( int ) ), SLOT( applySettings() ) );
    connect( m_base->m_markHistory,           SIGNAL( stateChanged( int ) ), SLOT( applySettings() ) );
    connect( m_base->m_appendType,            SIGNAL( activated( int ) ),    SLOT( applySettings() ) );

    QHBox *buttonBox = new QVBox( this );
    QCheckBox   *enableButton = new QCheckBox( i18n("Enable dynamic mode"), buttonBox, "dynamic" );
    KPushButton *configButton = new KPushButton( KGuiItem( i18n("Show Options"), "configure" ), buttonBox );

    configButton->setToggleButton( true );
    connect( configButton, SIGNAL(toggled( bool )), SLOT(showConfig( bool )) );

    connect( amaroK::actionCollection()->action( "dynamic_mode" ), SIGNAL( toggled( bool ) ),
             enableButton, SLOT( setChecked( bool ) ) );

    loadConfig();

    if( !m_base->m_cycleTracks->isChecked() )
        m_base->m_previousIntSpinBox->setEnabled( false );
    /*
     * We don't want to show the info dialog on startup, so we set the dynamic parameter manually and connect
     * the signal afterwards
     */
    enableButton->setChecked( AmarokConfig::dynamicMode() );
    connect( enableButton, SIGNAL(toggled( bool )), SLOT(setDynamicMode( bool )) );

    if( enableButton->isChecked() )
    {
//         Although random mode should be off, we uncheck it, just in case (eg amarokrc tinkering)
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( false );
        amaroK::actionCollection()->action( "random_mode" )->setEnabled( false );
    }
    else
    {
        m_repopulate->setEnabled( false );
        m_base->setEnabled( false );
    }

    KConfig *config = amaroK::config( "PlaylistBrowser" );
    if( config->readBoolEntry( "Show Dynamic Config", false ) )
        configButton->toggle();
}

Party::~Party()
{
    KConfig *config = amaroK::config( "PlaylistBrowser" );
    bool isShown = static_cast<QWidget*>(child("container"))->isShown();
    config->writeEntry( "Show Dynamic Config", isShown );
}

void
Party::loadConfig( PartyEntry *config )
{
    if( !config )
    {
        blockSignals(true); // else valueChanged() etc get connected

        m_base->m_upcomingIntSpinBox->setValue( AmarokConfig::dynamicUpcomingCount() );
        m_base->m_previousIntSpinBox->setValue( AmarokConfig::dynamicPreviousCount() );
        m_base->m_appendCountIntSpinBox->setValue( AmarokConfig::dynamicAppendCount() );
        m_base->m_cycleTracks->setChecked( AmarokConfig::dynamicCycleTracks() );
        m_base->m_markHistory->setChecked( AmarokConfig::dynamicMarkHistory() );
        if ( AmarokConfig::dynamicType() == "Random" )
            m_base->m_appendType->setCurrentItem( RANDOM );

        else if ( AmarokConfig::dynamicType() == "Suggestion" )
            m_base->m_appendType->setCurrentItem( SUGGESTION );

        else // Custom
            m_base->m_appendType->setCurrentItem( CUSTOM );

        blockSignals(false);
    }
    else
    {
        blockSignals(true);

        m_base->m_upcomingIntSpinBox->setValue( config->upcoming() );
        m_base->m_previousIntSpinBox->setValue( config->previous() );
        m_base->m_appendCountIntSpinBox->setValue( config->appendCount() );
        m_base->m_cycleTracks->setChecked( config->isCycled() );
        m_base->m_markHistory->setChecked( config->isMarked() );
        m_base->m_appendType->setCurrentItem( config->appendType() );

        AmarokConfig::setDynamicCustomList( config->items() );

        blockSignals(false);

        applySettings();
    }
}

void
Party::applySettings() //SLOT
{
    //TODO this should be in app.cpp or the dialog's class implementation, here is not the right place
    if( CollectionDB::instance()->isEmpty() )
        return;

    QString type;
    if( appendType() == RANDOM )
        type = "Random";
    else if( appendType() == SUGGESTION )
        type = "Suggestion";
    else if( appendType() == CUSTOM )
        type = "Custom";

    if( type != AmarokConfig::dynamicType() )
    {
        AmarokConfig::setDynamicType( type );
        PlaylistBrowser::instance()->loadDynamicItems();
    }

    if ( AmarokConfig::dynamicPreviousCount() != previousCount() )
    {
        Playlist::instance()->adjustPartyPrevious( previousCount() );
        AmarokConfig::setDynamicPreviousCount( previousCount() );
    }

    if ( AmarokConfig::dynamicUpcomingCount() != upcomingCount() )
    {
        Playlist::instance()->adjustPartyUpcoming( upcomingCount(), type );
        AmarokConfig::setDynamicUpcomingCount( upcomingCount() );
    }

    if ( AmarokConfig::dynamicMarkHistory() != markHistory() )
    {
        Playlist::instance()->alterHistoryItems( !markHistory() ); // markHistory() means NOT enabled
        AmarokConfig::setDynamicMarkHistory( markHistory() );
    }

    AmarokConfig::setDynamicCycleTracks( cycleTracks() );
    AmarokConfig::setDynamicAppendCount( appendCount() );

//     amaroK::actionCollection()->action( "prev" )->setEnabled( !AmarokConfig::dynamicMode() );
    amaroK::actionCollection()->action( "random_mode" )->setEnabled( !AmarokConfig::dynamicMode() );
    amaroK::actionCollection()->action( "playlist_shuffle" )->setEnabled( !AmarokConfig::dynamicMode() );
}

void
Party::blockSignals( const bool b )
{
    m_base->m_upcomingIntSpinBox->blockSignals( b );
    m_base->m_previousIntSpinBox->blockSignals( b );
    m_base->m_appendCountIntSpinBox->blockSignals( b );
    m_base->m_cycleTracks->blockSignals( b );
    m_base->m_markHistory->blockSignals( b );
    m_base->m_appendType->blockSignals( b );

    QVBox::blockSignals( b );  // respect inheritance
}

void
Party::setDynamicMode( bool enable, bool showDialog ) //SLOT
{
    if( enable )
    {
        KConfig *config = amaroK::config( "Notification Messages" );
        showDialog &= config->readBoolEntry( "showDynamicInfo", true );

        if( showDialog )
        {
            QString text = i18n( "<p align=\"center\"><b>Dynamic Mode</b></p>"
                           "Dynamic mode is a powerful method to manipulate your playlist. amaroK can automatically remove played "
                           "items and insert new ones to suit your taste!<br>"
                           "<br>"
                           "If you select <i>Playlist Shuffle</i>, make sure you choose some playlists or smart playlists by right-clicking "
                           "on the items in the playlist browser" );

            int info = KMessageBox::messageBox( this, KMessageBox::Information, text, i18n("Dynamic Mode Introduction"),
                                                i18n("Continue"), i18n("Cancel"), "showDynamicInfo" );

            if( info != KMessageBox::Ok )
                return;
        }

        // uncheck before disabling
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( false );
        amaroK::actionCollection()->action( "random_mode" )->setEnabled( false );
        amaroK::actionCollection()->action( "playlist_shuffle" )->setEnabled( false );
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( true );
    }
    else
    {
        Playlist::instance()->alterHistoryItems( true, true ); //enable all items

        // Random mode was being enabled without notification on leaving dynamic mode.  Remember to re-enable first!
        amaroK::actionCollection()->action( "random_mode" )->setEnabled( true );
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( false );
        amaroK::actionCollection()->action( "playlist_shuffle" )->setEnabled( true );
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( false );
    }
    m_repopulate->setEnabled( enable );
    m_base->setEnabled( enable );
}

void
Party::showConfig( bool show ) //SLOT
{
    static_cast<QWidget*>(child("container"))->setShown( show );
}

int     Party::previousCount() { return m_base->m_previousIntSpinBox->value(); }
int     Party::upcomingCount() { return m_base->m_upcomingIntSpinBox->value(); }
int     Party::appendCount()   { return m_base->m_appendCountIntSpinBox->value(); }
int     Party::appendType()    { return m_base->m_appendType->currentItem(); }
bool    Party::cycleTracks()   { return m_base->m_cycleTracks->isChecked(); }
bool    Party::markHistory()   { return m_base->m_markHistory->isChecked(); }


#include "party.moc"
