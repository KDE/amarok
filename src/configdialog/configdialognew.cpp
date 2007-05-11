/***************************************************************************
begin                : 2004/02/07
copyright            : (C) Mark Kretschmann
email                : markey@web.de
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
#include "app.h"
#include "collectiondb.h"
#include "config-amarok.h" // Has USE_MYSQL
#include "configdialognew.h"
#include "debug.h"

#include "AppearanceConfig.h"

#include <q3groupbox.h>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <qspinbox.h>
#include <QTextCodec>
#include <QToolTip>
#include <kvbox.h>
#include <QPixmap>

#include <kapplication.h> //kapp
#include <kcombobox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

Amarok2ConfigDialog::Amarok2ConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
   : KConfigDialog( parent, name, config )
{
    setAttribute( Qt::WA_DeleteOnClose );

    AppearanceConfig* appearance = new AppearanceConfig( this );

    //addPage( m_opt1, i18n( "General" ), Amarok::icon( "settings_general" ), i18n( "Configure General Options" ) );
    addPage( appearance, i18n( "Appearance" ), Amarok::icon( "settings_view" ), i18n( "Configure Amarok's Appearance" ) );
    //addPage( m_opt4, i18n( "Playback" ), Amarok::icon( "settings_playback" ), i18n( "Configure Playback" ) );
    //addPage( opt5,   i18n( "OSD" ), Amarok::icon( "settings_indicator" ), i18n( "Configure On-Screen-Display" ) );
    //addPage( opt6,   i18n( "Engine" ), Amarok::icon( "settings_engine" ), i18n( "Configure Engine" ) );
    //addPage( m_opt7, i18n( "Collection" ), Amarok::icon( "collection" ), i18n( "Configure Collection" ) );
    //addPage( opt8,   i18n( "last.fm" ), Amarok::icon( "audioscrobbler" ), i18n( "Configure last.fm Support" ) );
    //addPage( opt9,   i18n( "Media Devices" ), Amarok::icon( "device" ), i18n( "Configure Portable Player Support" ) );

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
    // Add the widget pointer to our list, for later reference
    m_pageList << page;

    KConfigDialog::addPage( page, itemName, pixmapName, header, manage );
}

/** Show page by object name */
void Amarok2ConfigDialog::showPageByName( const QByteArray& page )
{
    for( uint index = 0; index < m_pageList.count(); index++ ) {
        if ( m_pageList[index]->name() == page ) {
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
