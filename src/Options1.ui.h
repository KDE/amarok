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

void Options1::init()
{
#ifndef HAVE_EXSCALIBAR
    moodFrame->hide();
#endif

    kComboBox_browser->insertItem( "Konqueror" );
    kComboBox_browser->insertItem( "Firefox" );
    kComboBox_browser->insertItem( "Opera" );
    kComboBox_browser->insertItem( "Galeon" );
    kComboBox_browser->insertItem( "Epiphany" );
    kComboBox_browser->insertItem( "Safari" );
}


void Options1::slotUpdateMoodFrame()
{
    kcfg_MakeMoodier->setEnabled(kcfg_ShowMoodbar->isChecked());
    kcfg_AlterMood->setEnabled(kcfg_ShowMoodbar->isChecked() && kcfg_MakeMoodier->isChecked());
    kcfg_MoodsWithMusic->setEnabled(kcfg_ShowMoodbar->isChecked());
}
