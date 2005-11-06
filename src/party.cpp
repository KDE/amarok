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
#include "playlistselection.h"
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

Party*
Party::instance()
{
    if( s_instance )
        return s_instance;
    return new Party( PlaylistWindow::self(), "PartySettings" );
}

Party::Party( QWidget *parent, const char *name )
    : QObject(parent, name)
    , m_currentParty(0)
    , m_ac( new KActionCollection( this ) )
{
    s_instance = this;

    m_repopulate = new KAction( i18n("Repopulate"), "rebuild", 0,
                                this, SLOT( repopulate() ), m_ac, "Repopulate Upcoming Tracks" );

    if( AmarokConfig::dynamicMode() )
    {
        //Although random mode should be off, we uncheck it, just in case (eg amarokrc tinkering)
        static_cast<KToggleAction*>(amaroK::actionCollection()->action( "random_mode" ))->setChecked( false );
    }
}

Party::~Party()
{ }

void
Party::loadConfig( PartyEntry *config )
{
    m_currentParty = config;

    AmarokConfig::setDynamicCustomList( config->items() );

    emit titleChanged( config->title() );

    applySettings();
}

void
Party::editActiveParty()
{
    if( m_currentParty == 0 )
        return;
    ConfigDynamic::editDynamicPlaylist(PlaylistWindow::self(), m_currentParty);
}

#define partyInfo(function, default) { \
    if (m_currentParty) return m_currentParty->function(); \
    else return default; }
    //do something sane if m_currentParty has been deleted

int  Party::previousCount() { partyInfo( previous, 5); }
int  Party::upcomingCount() { partyInfo( upcoming, 20); }
int  Party::appendCount()   { partyInfo( appendCount, 1); }
int  Party::appendType()    { partyInfo( appendType, 0); }
bool Party::cycleTracks()   { partyInfo( isCycled, true); }
bool Party::markHistory()   { partyInfo( isMarked, true); }
QString Party::title()      { partyInfo( title, "Invalid"); } //no i18n since its just a fallback

#undef partyInfo

void Party::setDynamicItems(const QPtrList<QListViewItem>& newList)
{
    if(!m_currentParty)
       { warning() << "Party has a 0 for m_currentParty." << endl;  return; }

    QStringList strListEntries;
    QListViewItem* entry;
    QPtrListIterator<QListViewItem> it( newList );

    while ((entry = it.current()) != 0)
    {
        ++it;
        strListEntries << entry->text(0);
    }

    m_currentParty->setItems(strListEntries);
    PlaylistBrowser::instance()->saveDynamics();
}

void
Party::repopulate() //SLOT
{
    Playlist::instance()->repopulate();
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

            int info = KMessageBox::messageBox( static_cast<QWidget*>(parent()), KMessageBox::Information, text, i18n("Dynamic Mode Introduction"),
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
