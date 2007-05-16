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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "GeneralConfig.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "config-amarok.h"
#include "moodbar.h"

#include <KStandardDirs>


GeneralConfig::GeneralConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this ); 

    slotUpdateMoodFrame();

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
#ifdef Q_WS_MAC
    if ( !KStandardDirs::findExe( "open" ).isEmpty() )
        browsers.prepend( i18n( "Default Browser" ) );
#else
    if ( !KStandardDirs::findExe( "kfmclient" ).isEmpty() )
        browsers.prepend( i18n( "Default KDE Browser" ) );
#endif

    kComboBox_browser->insertItems( -1, browsers );
    kLineEdit_customBrowser->setText( AmarokConfig::externalBrowser() );
    int index = browsers.indexOf( AmarokConfig::externalBrowser() );
    if( index >= 0 )
        kComboBox_browser->setCurrentItem( AmarokConfig::externalBrowser() );
    else if( AmarokConfig::externalBrowser() ==
#ifdef Q_WS_MAC
            "open"
#else
            "kfmclient openUrl"
#endif
      )
    {
        kComboBox_browser->setCurrentItem( 0 );
    }
    else
    {
        checkBox_customBrowser->setChecked( true );
    }

    connect( kComboBox_browser, SIGNAL( activated( int ) ), parent, SLOT( updateButtons() ) );
    connect( kLineEdit_customBrowser, SIGNAL( textChanged( const QString& ) ), parent, SLOT( updateButtons() ) );
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
    Amarok::setUseScores( kcfg_UseScores->isChecked() );
    Amarok::setUseRatings( kcfg_UseRatings->isChecked() );

    // The following makes everything with a moodbar redraw itself.
    Amarok::setMoodbarPrefs( kcfg_ShowMoodbar->isChecked(),
                             kcfg_MakeMoodier->isChecked(),
                             kcfg_AlterMood->currentIndex(),
                             kcfg_MoodsWithMusic->isChecked() );
}


///////////////////////////////////////////////////////////////
// SLOTS 
///////////////////////////////////////////////////////////////

void
GeneralConfig::slotUpdateMoodFrame() //SLOT
{
    if( Moodbar::executableExists() )
      {
        moodbarHelpLabel->hide();
        moodFrame->setEnabled(true);

        kcfg_MakeMoodier->setEnabled(kcfg_ShowMoodbar->isChecked());
        kcfg_AlterMood->setEnabled(kcfg_ShowMoodbar->isChecked() && kcfg_MakeMoodier->isChecked());
        kcfg_MoodsWithMusic->setEnabled(kcfg_ShowMoodbar->isChecked());
      }

    else
      {
        moodbarHelpLabel->show();
        kcfg_ShowMoodbar->setChecked(false);
        moodFrame->setEnabled(false);
      }
}

#include "GeneralConfig.moc"

