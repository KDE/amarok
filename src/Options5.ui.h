/***************************************************************************
begin                : 2004/02/25
copyright            : (C) Frederik Holljen
email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

#include "amarokconfig.h"
#include "configdialog.h"


void Options5::init()
{
    m_pOSDPreview = new OSDPreviewWidget( "amaroK", this ); //must be child!!!
    m_pOSDPreview->setAlignment( (OSDWidget::Alignment)AmarokConfig::osdAlignment() );
    m_pOSDPreview->setOffset( AmarokConfig::osdXOffset(), AmarokConfig::osdYOffset() );

    connect( m_pOSDPreview, SIGNAL(positionChanged()), SLOT(slotPositionChanged()) );

    const int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
        kcfg_OsdScreen->insertItem( QString::number( i ) );

    connect( kcfg_OsdDrawShadow, SIGNAL(toggled(bool)), m_pOSDPreview, SLOT(setShadow(bool)) );
    connect( kcfg_OsdTextColor, SIGNAL(changed(const QColor&)), m_pOSDPreview, SLOT(setTextColor(const QColor&)) );
    connect( kcfg_OsdBackgroundColor, SIGNAL(changed(const QColor&)), m_pOSDPreview, SLOT(setBackgroundColor(const QColor&)) );
    connect( kcfg_OsdFont, SIGNAL(fontSelected(const QFont&)), m_pOSDPreview, SLOT(setFont(const QFont&)) );
    connect( kcfg_OsdScreen, SIGNAL(activated(int)), m_pOSDPreview, SLOT(setScreen(int)) );
    connect( kcfg_OsdEnabled, SIGNAL(toggled(bool)), m_pOSDPreview, SLOT(setShown(bool)) );
    connect( kcfg_OsdText, SIGNAL(changed(const QString& )), m_pOSDPreview, SLOT(setText(const QString& )) );
}

void
Options5::slotPositionChanged()
{
    kcfg_OsdScreen->blockSignals( true );
    kcfg_OsdScreen->setCurrentItem( m_pOSDPreview->screen() );
    kcfg_OsdScreen->blockSignals( false );

//    emit settingsChanged();
}

void
Options5::hideEvent( QHideEvent * )
{
    m_pOSDPreview->hide();
}

void
Options5::showEvent( QShowEvent * )
{
    useCustomColorsToggled( kcfg_OsdUseCustomColors->isChecked() );

    m_pOSDPreview->setFont( kcfg_OsdFont->font() );
    m_pOSDPreview->setScreen( kcfg_OsdScreen->currentItem() );
    m_pOSDPreview->setShown( kcfg_OsdEnabled->isChecked() );
}

void
Options5::useCustomColorsToggled( bool on )
{
    if( on )
    {
        m_pOSDPreview->setTextColor( kcfg_OsdTextColor->color() );
        m_pOSDPreview->setBackgroundColor( kcfg_OsdBackgroundColor->color() );
    }
    else m_pOSDPreview->unsetColors();
}
