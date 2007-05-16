/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "configdialognew.h"
#include "debug.h"

#include "AppearanceConfig.h"
#include "CollectionConfig.h"
#include "EngineConfig.h"
#include "GeneralConfig.h"
#include "LastfmConfig.h"
#include "MediadeviceConfig.h"
#include "OsdConfig.h"
#include "PlaybackConfig.h"

#include <KIconLoader>
#include <KLocale>
#include <KStandardDirs>


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

Amarok2ConfigDialog::Amarok2ConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
   : KConfigDialog( parent, name, config )
{
    setAttribute( Qt::WA_DeleteOnClose );

    ConfigDialogBase* appearance  = new AppearanceConfig( this );
    ConfigDialogBase* collection  = new CollectionConfig( this );
    ConfigDialogBase* engine      = new EngineConfig( this );
    ConfigDialogBase* general     = new GeneralConfig( this );
    ConfigDialogBase* lastfm      = new LastfmConfig( this );
    ConfigDialogBase* mediadevice = new MediadeviceConfig( this );
    ConfigDialogBase* osd         = new OsdConfig( this );
    ConfigDialogBase* playback    = new PlaybackConfig( this );

    addPage( general,     i18n( "General" ), Amarok::icon( "settings_general" ), i18n( "Configure General Options" ) );
    addPage( appearance,  i18n( "Appearance" ), Amarok::icon( "settings_view" ), i18n( "Configure Amarok's Appearance" ) );
    addPage( playback,    i18n( "Playback" ), Amarok::icon( "settings_playback" ), i18n( "Configure Playback" ) );
    addPage( engine,      i18n( "Engine" ), Amarok::icon( "settings_engine" ), i18n( "Configure Engine" ) );
    addPage( osd,         i18n( "OSD" ), Amarok::icon( "settings_indicator" ), i18n( "Configure On-Screen-Display" ) );
    addPage( lastfm,      i18n( "last.fm" ), Amarok::icon( "audioscrobbler" ), i18n( "Configure last.fm Support" ) );
    addPage( collection,  i18n( "Collection" ), Amarok::icon( "collection" ), i18n( "Configure Collection" ) );
    addPage( mediadevice, i18n( "Media Devices" ), Amarok::icon( "device" ), i18n( "Configure Portable Player Support" ) );

    // Show information labels (must be done after insertions)
    {
        QObjectList list = queryList( "QLabel", "infoPixmap" );
        QPixmap const info = KIconLoader::global()->iconPath( "messagebox_info", -K3Icon::SizeHuge );
        for( int labelI = 0; labelI < list.size(); labelI++ )
            qobject_cast<QLabel*>( list.at(labelI) )->setPixmap( info );
    }

    //stop KFont Requesters getting stupidly large
    {
        QObjectList list = queryList( "QLabel", "m_sampleLabel" );
        for( int labelI = 0; labelI < list.size(); labelI++ )
            qobject_cast<QLabel*>( list.at( labelI ) )->setMaximumWidth( 250 );
    }
}

Amarok2ConfigDialog::~Amarok2ConfigDialog()
{
    DEBUG_FUNC_INFO
}


/** Reimplemented from KConfigDialog */
void Amarok2ConfigDialog::addPage( ConfigDialogBase *page, const QString &itemName, const QString &pixmapName, const QString &header, bool manage )
{
    connect( page, SIGNAL( settingsChanged( const QString& ) ), this, SIGNAL( settingsChanged( const QString& ) ) );

    // Add the widget pointer to our list, for later reference
    m_pageList << page;

    KConfigDialog::addPage( page, itemName, pixmapName, header, manage );
}

/** Show page by object name */
void Amarok2ConfigDialog::showPageByName( const QByteArray& page )
{
    for( int index = 0; index < m_pageList.count(); index++ ) {
        if ( m_pageList[index]->objectName() == page ) {
            KConfigDialog::setCurrentPage( qobject_cast<KPageWidgetItem*>( m_pageList[index] ) );
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Update the buttons.
 * REIMPLEMENTED
 */

void Amarok2ConfigDialog::updateButtons()
{
    KConfigDialog::updateButtons();
}

/**
 * Update the settings from the dialog.
 * Example use: User clicks Ok or Apply button in a configure dialog.
 * REIMPLEMENTED
 */
void Amarok2ConfigDialog::updateSettings()
{
    foreach( ConfigDialogBase* page, m_pageList )
        page->updateSettings();
}


/**
 * Update the configuration-widgets in the dialog based on Amarok's current settings.
 * Example use: Initialisation of dialog.
 * Example use: User clicks Reset button in a configure dialog.
 * REIMPLEMENTED
 */
void Amarok2ConfigDialog::updateWidgets()
{
}


/**
 * Update the configuration-widgets in the dialog based on the default values for Amarok's settings.
 * Example use: User clicks Defaults button in a configure dialog.
 * REIMPLEMENTED
 */
void Amarok2ConfigDialog::updateWidgetsDefault()
{
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * @return true if any configuration items we are managing changed from Amarok's stored settings
 * We manage the engine combo box and some of the OSD settings
 * REIMPLEMENTED
 */
bool Amarok2ConfigDialog::hasChanged()
{
    bool changed = false;

    foreach( ConfigDialogBase* page, m_pageList )
        if( page->hasChanged() )
            changed = true;

    return changed;
}


/** REIMPLEMENTED */
bool Amarok2ConfigDialog::isDefault()
{
    bool def = false;

    foreach( ConfigDialogBase* page, m_pageList )
        if( page->hasChanged() )
            def = true;

    return def;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////


#include "configdialognew.moc"
