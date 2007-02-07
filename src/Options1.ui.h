//Released under GPLv2 or later. (C) 2005 Ian Monroe <ian@monroe.nu>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <config.h>

#include "amarokconfig.h"
#include "moodbar.h"
#include <kstandarddirs.h>


void Options1::init()
{
    slotUpdateMoodFrame();

    QStringList browsers;
    browsers << "konqueror" << "firefox" << "opera" << "galeon" << "epiphany"
             << "safari" << "mozilla";

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
    if ( KStandardDirs::findExe( "open" ) != QString::null )
        browsers.prepend( i18n( "Default Browser" ) );
#else
    if ( KStandardDirs::findExe( "kfmclient" ) != QString::null )
        browsers.prepend( i18n( "Default KDE Browser" ) );
#endif

    kComboBox_browser->insertStringList( browsers );
    kLineEdit_customBrowser->setText( AmarokConfig::externalBrowser() );
    int index = browsers.findIndex( AmarokConfig::externalBrowser() );
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
}



void Options1::slotUpdateMoodFrame()
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
