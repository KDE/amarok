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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "AppearanceConfig.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"

#include <KApplication>
#include <KFileDialog>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTar>


AppearanceConfig::AppearanceConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this ); 

}

AppearanceConfig::~AppearanceConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
AppearanceConfig::hasChanged()
{
    return false;
}

bool
AppearanceConfig::isDefault()
{
    return false;
}


void
AppearanceConfig::retrievePushButton_clicked() //SLOT
{
#if 0 //FIXME: KNS2
    // Delete KNewStuff's configuration entries. These entries reflect which styles
    // are already installed. As we cannot yet keep them in sync after uninstalling
    // styles, we deactivate the check marks entirely.
    Amarok::config()->deleteGroup( "KNewStuffStatus" );

    // we need this because KNewStuffGeneric's install function isn't clever enough
    AmarokThemeNewStuff *kns = new AmarokThemeNewStuff( "amarok/themes", this );
    KNS::Engine *engine = new KNS::Engine( kns, "amarok/theme", this );
    KNS::DownloadDialog* d = new KNS::DownloadDialog( engine, this );
    d->setType( "amarok/theme" );
    // you have to do this by hand when providing your own Engine
    KNS::ProviderLoader *p = new KNS::ProviderLoader( this );
    connect( p, SIGNAL( providersLoaded(Provider::List*) ), d, SLOT( slotProviders(Provider::List *) ) );
    p->load( "amarok/theme", "http://amarok.kde.org/knewstuff/amarokthemes-providers.xml" );

    connect( d, SIGNAL( finished() ), d, SLOT( delayedDestruct() ) );
    connect( d, SIGNAL( finished() ), this, SLOT( updateStyleComboBox() ) );

    // Due to kdelibs idiocy, KNS::DownloadDialog is /always/ non-modal. So we have to
    // ensure that closing the settings dialog before the DownloadDialog doesn't crash.
    QTimer::singleShot( 0, d, SLOT( exec() ) );
#endif
}




#include "AppearanceConfig.moc"

