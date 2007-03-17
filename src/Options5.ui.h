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
#include <klocale.h>
#include "qstringx.h"
#include <qtooltip.h>


void Options5::init()
{
    m_pOSDPreview = new OSDPreviewWidget( this ); //must be child!!!
    m_pOSDPreview->setAlignment( static_cast<OSDWidget::Alignment>( AmarokConfig::osdAlignment() ) );
    m_pOSDPreview->setOffset( AmarokConfig::osdYOffset() );

    connect( m_pOSDPreview, SIGNAL( positionChanged() ), SLOT( slotPositionChanged() ) );

    const int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
        kcfg_OsdScreen->insertItem( QString::number( i ) );

    connect( kcfg_OsdDrawShadow,      SIGNAL( toggled(bool) ),
             m_pOSDPreview,           SLOT( setDrawShadow(bool) ) );
    connect( kcfg_OsdTextColor,       SIGNAL( changed(const QColor&) ),
             m_pOSDPreview,           SLOT( setTextColor(const QColor&) ) );
    connect( kcfg_OsdUseCustomColors, SIGNAL( toggled(bool) ),
             this,                    SLOT( useCustomColorsToggled(bool) ) );
    connect( kcfg_OsdBackgroundColor, SIGNAL( changed(const QColor&) ),
             m_pOSDPreview,           SLOT( setBackgroundColor(const QColor&) ) );
    connect( kcfg_OsdFont,            SIGNAL( fontSelected(const QFont&) ),
             m_pOSDPreview,           SLOT( setFont(const QFont&) ) );
    connect( kcfg_OsdScreen,          SIGNAL( activated(int) ),
             m_pOSDPreview,           SLOT( setScreen(int) ) );
    connect( kcfg_OsdEnabled,         SIGNAL( toggled(bool) ),
             m_pOSDPreview,           SLOT( setShown(bool) ) );

    Amarok::QStringx text = i18n(
            "<h3>Tags Displayed in OSD</h3>"
            "You can use the following tokens:"
                "<ul>"
                "<li>Title - %1"
                "<li>Album - %2"
                "<li>Artist - %3"
                "<li>Genre - %4"
                "<li>Bitrate - %5"
                "<li>Year - %6"
                "<li>Track Length - %7"
                "<li>Track Number - %8"
                "<li>Filename - %9"
                "<li>Directory - %10"
                "<li>Type - %11"
                "<li>Comment - %12"
                "<li>Score - %13"
                "<li>Playcount - %14"
                "<li>Disc Number - %15"
                "<li>Rating - %16"
                "<li>Moodbar - %17"
		"<li>Elapsed Time - %18"
                "</ul>"
            "If you surround sections of text that contain a token with curly-braces, that section will be hidden if the token is empty, for example:"
                "<pre>%19</pre>"
            "Will not show <b>Score: <i>%score</i></b> if the track has no score." );

    QToolTip::add( kcfg_OsdText, text.args( QStringList()
            // we don't translate these, it is not sensible to do so
            << "%title"  << "%album"   << "%artist"  << "%genre"     << "%bitrate"
            << "%year "  << "%length"  << "%track"   << "%filename"  << "%directory"
            << "%type"   << "%comment" << "%score"   << "%playcount" << "%discnumber"
            << "%rating" << "%moodbar" << "%elapsed"
            << "%title {" + i18n( "Score: %1" ).arg( "%score" ) +'}' ) );
}

void
Options5::slotPositionChanged()
{
    kcfg_OsdScreen->blockSignals( true );
    kcfg_OsdScreen->setCurrentItem( m_pOSDPreview->screen() );
    kcfg_OsdScreen->blockSignals( false );

    // Update button states (e.g. "Apply")
    emit settingsChanged();
}

void
Options5::hideEvent( QHideEvent* )
{
    m_pOSDPreview->hide();
}

void
Options5::showEvent( QShowEvent* )
{
    useCustomColorsToggled( kcfg_OsdUseCustomColors->isChecked() );

    m_pOSDPreview->setFont( kcfg_OsdFont->font() );
    m_pOSDPreview->setScreen( kcfg_OsdScreen->currentItem() );
    m_pOSDPreview->setShown( kcfg_OsdEnabled->isChecked() );
}

void
Options5::useCustomColorsToggled( bool on )
{
    m_pOSDPreview->setUseCustomColors( on, kcfg_OsdTextColor->color(), kcfg_OsdBackgroundColor->color() );
}
