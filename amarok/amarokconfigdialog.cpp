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

#include "amarokconfig.h"
#include "amarokconfigdialog.h"
#include "engine/enginebase.h"
#include "Options1.h"
#include "Options2.h"
#include "Options3.h"
#include "Options4.h"
#include "Options5.h"
#include "playerapp.h"
#include "osd.h"

#include <qapplication.h>

#include <kconfigdialog.h>
#include <klocale.h>

// for osd preview
#include <kcombobox.h>
#include <kfontrequester.h>
#include <kcolorbutton.h>
#include <knuminput.h>
#include <kpushbutton.h>

AmarokConfigDialog::AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
        : KConfigDialog( parent, name, config )
          , m_pOsdPreview( 0 ) // do not create before really needed.
{
    //we must handle the "Sound Setting" QComboBox manually, because KConfigDialogManager can't
    //manage dynamic itemLists (at least I don't know how to do it)
    Options4* pOpt4 = new Options4( 0,"Playback" );
    m_pSoundSystem = pOpt4->sound_system;
    m_pSoundSystem->insertStringList( PlayerApp::m_pEngine->listEngines() );
    m_pSoundSystem->setCurrentText  ( AmarokConfig::soundSystem() );

    // add screens
    m_pOpt5 = new Options5( 0,"OSD" );
    int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
        m_pOpt5->kcfg_OsdScreen->insertItem( QString::number( i ) );

    connect( m_pSoundSystem, SIGNAL( activated( int ) ), this, SLOT( settingsChangedSlot() ) );
    connect( m_pOpt5->PreviewButton, SIGNAL( clicked() ), this, SLOT( previewOsd() ) );

    addPage( new Options1( 0,"General" ),  i18n("General"),  "misc",   i18n("Configure general options") );
    addPage( new Options2( 0,"Fonts" ),    i18n("Fonts"),    "fonts",  i18n("Configure fonts") );
    addPage( new Options3( 0,"Colors" ),   i18n("Colors"),   "colors", i18n("Configure colors") );
    addPage( pOpt4,                        i18n("Playback"), "kmix",   i18n("Configure playback") );
    addPage( m_pOpt5,      i18n("OSD" ),     "tv",     i18n("Configure on-screen-display") );

    setInitialSize( QSize( 460, 390 ) );
}


bool AmarokConfigDialog::hasChanged()
{
    return ( m_pSoundSystem->currentText() != AmarokConfig::soundSystem() );
}


bool AmarokConfigDialog::isDefault()
{
    return ( m_pSoundSystem->currentText() == "arts" );
}


void AmarokConfigDialog::updateSettings()
{
    AmarokConfig::setSoundSystem( m_pSoundSystem->currentText() );
    emit settingsChanged();
}

void AmarokConfigDialog::previewOsd()
{
    if( !m_pOsdPreview ) m_pOsdPreview = new OSDWidget( i18n( "amaroK" ) );
    m_pOsdPreview->setFont( m_pOpt5->kcfg_OsdFont->font() );
    m_pOsdPreview->setTextColor( m_pOpt5->kcfg_OsdTextColor->color() );
    m_pOsdPreview->setBackgroundColor( m_pOpt5->kcfg_OsdBackgroundColor->color() );
    m_pOsdPreview->setDuration( m_pOpt5->kcfg_OsdDuration->value() );
    m_pOsdPreview->setPosition( (OSDWidget::Position)m_pOpt5->kcfg_OsdAlignment->currentItem() );
    m_pOsdPreview->setScreen( m_pOpt5->kcfg_OsdScreen->currentItem() );
    m_pOsdPreview->setOffset( m_pOpt5->kcfg_OsdXOffset->value(), m_pOpt5->kcfg_OsdYOffset->value() );
    m_pOsdPreview->showOSD( i18n("OSD preview - The quick brown fox jumps over the lazy dog" ) );
}

#include "amarokconfigdialog.moc"
