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
    m_pOSDPreview = new OSDPreviewWidget( "amaroK" );
    m_pOSDPreview->setText( i18n("OSD preview - drag to reposition" ) );
    m_pOSDPreview->setFont( AmarokConfig::osdFont() );
    m_pOSDPreview->setShadow( AmarokConfig::osdDrawShadow() );
    m_pOSDPreview->setPosition( (OSDWidget::Position) AmarokConfig::osdAlignment() );
    m_pOSDPreview->setOffset( AmarokConfig::osdXOffset(), AmarokConfig::osdYOffset() );
    
    connect( m_pOSDPreview, SIGNAL( positionChanged( int, OSDWidget::Position, int, int ) ) ,
             SLOT( osdPositionChanged( int, OSDWidget::Position, int, int ) ) );

    int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
        kcfg_OsdScreen->insertItem( QString::number( i ) );
}


void Options5::destroy()
{
    delete m_pOSDPreview;
}


void Options5::fontChanged(const QFont &font )
{
    m_pOSDPreview->setFont( font );
}


void Options5::textColorChanged( const QColor &color )
{
    m_pOSDPreview->setTextColor( color );
}


void Options5::backgroundColorChanged( const QColor &color )
{
    m_pOSDPreview->setBackgroundColor( color );
}


void Options5::alignmentChanged( int alignment )
{
    m_pOSDPreview->setPosition( (OSDWidget::Position)alignment );
}


void Options5::screenChanged( int screen )
{
    m_pOSDPreview->setScreen( screen );
}


void Options5::osdPositionChanged( int screen, OSDWidget::Position alignment, int /*xOffset*/, int /*yOffset*/ )
{
    // atomic
    kcfg_OsdScreen->blockSignals( true );
    kcfg_OsdAlignment->blockSignals( true );

    kcfg_OsdScreen->setCurrentItem( screen );
    kcfg_OsdAlignment->setCurrentItem( alignment );

    kcfg_OsdScreen->blockSignals( false );
    kcfg_OsdAlignment->blockSignals( false );

    AmarokConfigDialog* conf = static_cast<AmarokConfigDialog*>( KConfigDialog::exists( "settings" ) );
    if ( conf ) conf->triggerChanged();
}


void Options5::hideEvent( QHideEvent * )
{
    m_pOSDPreview->hide();
}


void Options5::showEvent( QShowEvent * )
{
    useCustomColorsToggled( kcfg_OsdUseCustomColors->isChecked() );

    m_pOSDPreview->setFont( kcfg_OsdFont->font() );
    m_pOSDPreview->setPosition( (OSDWidget::Position)kcfg_OsdAlignment->currentItem() );
    m_pOSDPreview->setScreen( kcfg_OsdScreen->currentItem() );
    m_pOSDPreview->setShown( kcfg_OsdEnabled->isChecked() );
}


void Options5::osdEnabledChange( bool on )
{
    m_pOSDPreview->setShown( on );
}


void Options5::useCustomColorsToggled( bool on )
{
    if( on )
    {
        m_pOSDPreview->setTextColor( kcfg_OsdTextColor->color() );
        m_pOSDPreview->setBackgroundColor( kcfg_OsdBackgroundColor->color() );
    }
    else m_pOSDPreview->unsetColors();
}


void Options5::drawShadowToggled( bool on )
{
    m_pOSDPreview->setShadow( on );
}
