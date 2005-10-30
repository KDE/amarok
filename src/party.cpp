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
{
    s_instance = this;

    m_repopulate = new KAction( i18n("Repopulate"), "rebuild", 0,
                                       this, SLOT( repopulate() ), m_ac, "Repopulate Upcoming Tracks" );

    QHBox *buttonBox = new QVBox( this );
    QCheckBox   *enableButton = new QCheckBox( i18n("Enable dynamic mode"), buttonBox, "dynamic" );
    connect( amaroK::actionCollection()->action( "dynamic_mode" ), SIGNAL( toggled( bool ) ),
             enableButton, SLOT( setChecked( bool ) ) );

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
    }
    else
        m_repopulate->setEnabled( false );
}

Party::~Party()
{ }

void
Party::loadConfig( PartyEntry *config )
{
    //if(!config)
//        return;
    m_upcomingCount = config->upcoming();
    m_previousCount = config->previous();
    m_appendCount = config->appendCount();
    m_cycleTracks = config->isCycled();
    m_markHistory= config->isMarked();
    m_appendType = config->appendType();

    AmarokConfig::setDynamicCustomList( config->items() );
    applySettings();
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
    amaroK::actionCollection()->action( "playlist_shuffle" )->setEnabled( !AmarokConfig::dynamicMode() );
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
        amaroK::actionCollection()->action( "playlist_shuffle" )->setEnabled( false );
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( true );
    }
    else
    {
        Playlist::instance()->alterHistoryItems( true, true ); //enable all items

        // Random mode was being enabled without notification on leaving dynamic mode.  Remember to re-enable first!
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "dynamic_mode" ))->setChecked( false );
    }
    m_repopulate->setEnabled( enable );
}

#include "party.moc"
