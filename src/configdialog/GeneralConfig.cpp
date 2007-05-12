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

#include "GeneralConfig.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "moodbar.h"

#include <KStandardDirs>


GeneralConfig::GeneralConfig( QWidget* parent )
    : ConfigDialogBase( parent )
    , m_gui( new Ui_GeneralConfig() )
{
    m_gui->setupUi( this ); 

    connect( m_gui->kComboBox_browser, SIGNAL( activated( int ) ), parent, SLOT( updateButtons() ) );
    connect( m_gui->kLineEdit_customBrowser, SIGNAL( textChanged( const QString& ) ), parent, SLOT( updateButtons() ) );

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

    m_gui->kComboBox_browser->insertItems( -1, browsers );
    m_gui->kLineEdit_customBrowser->setText( AmarokConfig::externalBrowser() );
    int index = browsers.indexOf( AmarokConfig::externalBrowser() );
    if( index >= 0 )
        m_gui->kComboBox_browser->setCurrentItem( AmarokConfig::externalBrowser() );
    else if( AmarokConfig::externalBrowser() ==
#ifdef Q_WS_MAC
            "open"
#else
            "kfmclient openUrl"
#endif
      )
    {
        m_gui->kComboBox_browser->setCurrentItem( 0 );
    }
    else
    {
        m_gui->checkBox_customBrowser->setChecked( true );
    }
}

GeneralConfig::~GeneralConfig()
{
    delete m_gui;
}

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

void GeneralConfig::updateSettings()
{
    Amarok::setUseScores( m_gui->kcfg_UseScores->isChecked() );
    Amarok::setUseRatings( m_gui->kcfg_UseRatings->isChecked() );

    // The following makes everything with a moodbar redraw itself.
    Amarok::setMoodbarPrefs( m_gui->kcfg_ShowMoodbar->isChecked(),
                             m_gui->kcfg_MakeMoodier->isChecked(),
                             m_gui->kcfg_AlterMood->currentIndex(),
                             m_gui->kcfg_MoodsWithMusic->isChecked() );
}

void
GeneralConfig::slotUpdateMoodFrame()
{
    if( Moodbar::executableExists() )
      {
        m_gui->moodbarHelpLabel->hide();
        m_gui->moodFrame->setEnabled(true);

        m_gui->kcfg_MakeMoodier->setEnabled(m_gui->kcfg_ShowMoodbar->isChecked());
        m_gui->kcfg_AlterMood->setEnabled(m_gui->kcfg_ShowMoodbar->isChecked() && m_gui->kcfg_MakeMoodier->isChecked());
        m_gui->kcfg_MoodsWithMusic->setEnabled(m_gui->kcfg_ShowMoodbar->isChecked());
      }

    else
      {
        m_gui->moodbarHelpLabel->show();
        m_gui->kcfg_ShowMoodbar->setChecked(false);
        m_gui->moodFrame->setEnabled(false);
      }
}

#include "GeneralConfig.moc"

