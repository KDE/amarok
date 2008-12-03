/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *   Copyright (C) 2005      by Ian Monroe <ian@monroe.nu>                 *
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

#include "GeneralConfig.h"
#include "Amarok.h"
#include "amarokconfig.h"
#include <config-amarok.h>  

#include <KStandardDirs>


GeneralConfig::GeneralConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this ); 

    QStringList browsers;
    browsers << "konqueror" << "firefox" << "opera" << "galeon" << "epiphany" << "safari" << "mozilla";

#if 0
    // Remove browsers which are not actually installed
    for( QStringList::Iterator it = browsers.begin(), end = browsers.end(); it != end; ) {
        if( KStandardDirs::findExe( *it ).isEmpty() )
            it = browsers.erase( it );
        else
            ++it;
    }
#endif

    // WARNING: if you change the strings here, remember to change them in Amarok::invokeBrowser
    // as we need to hack around KConfigDialog saving translated text to the config file
#ifdef Q_WS_MAC
    if ( !KStandardDirs::findExe( "open" ).isEmpty() )
        browsers.prepend( i18n( "Default Browser" ) );
#else
    if ( !KStandardDirs::findExe( "kfmclient" ).isEmpty() )
        browsers.prepend( i18n( "Default KDE Browser" ) );
#endif

    kcfg_ExternalBrowser->insertItems( -1, browsers );
    int index = browsers.indexOf( AmarokConfig::externalBrowser() );
    if( index >= 0 )
        kcfg_ExternalBrowser->setCurrentIndex( index );
    else if( AmarokConfig::externalBrowser() ==
#ifdef Q_WS_MAC
             "open"
#else
             "xdg-open"
#endif
           )
    {
        kcfg_ExternalBrowser->setCurrentIndex( 0 );
    } else {
        kcfg_ExternalBrowser->addItem( AmarokConfig::externalBrowser() );
        kcfg_ExternalBrowser->setCurrentIndex( kcfg_ExternalBrowser->count() - 1 );
    }


    connect( kcfg_ExternalBrowser, SIGNAL( textChanged( int ) ), parent, SLOT( updateButtons() ) );
    //connect( kLineEdit_customBrowser, SIGNAL( textChanged( const QString& ) ), parent, SLOT( updateButtons() ) );
}

GeneralConfig::~GeneralConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
GeneralConfig::hasChanged()
{
    return false;
}

bool
GeneralConfig::isDefault()
{
    return false;
}

void
GeneralConfig::updateSettings() //SLOT
{
}


#include "GeneralConfig.moc"

