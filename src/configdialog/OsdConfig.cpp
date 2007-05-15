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

#include "OsdConfig.h"
#include "amarokconfig.h"
#include "osd.h"
#include "qstringx.h"

#include <QDesktopWidget>


OsdConfig::OsdConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this ); 

    m_osdPreview = new OSDPreviewWidget( this ); //must be child!!!
    m_osdPreview->setAlignment( static_cast<OSDWidget::Alignment>( AmarokConfig::osdAlignment() ) );
    m_osdPreview->setOffset( AmarokConfig::osdYOffset() );

    connect( m_osdPreview, SIGNAL( positionChanged() ), SLOT( slotPositionChanged() ) );

    const int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
        kcfg_OsdScreen->addItem( QString::number( i ) );

    connect( kcfg_OsdDrawShadow,      SIGNAL( toggled(bool) ),
             m_osdPreview,           SLOT( setDrawShadow(bool) ) );
    connect( kcfg_OsdTextColor,       SIGNAL( changed(const QColor&) ),
             m_osdPreview,           SLOT( setTextColor(const QColor&) ) );
    connect( kcfg_OsdUseCustomColors, SIGNAL( toggled(bool) ),
             this,                    SLOT( useCustomColorsToggled(bool) ) );
    connect( kcfg_OsdBackgroundColor, SIGNAL( changed(const QColor&) ),
             m_osdPreview,           SLOT( setBackgroundColor(const QColor&) ) );
#if 0
    connect( kcfg_OsdFont,            SIGNAL( fontSelected(const QFont&) ),
             m_osdPreview,           SLOT( setFont(const QFont&) ) );
#endif
    connect( kcfg_OsdScreen,          SIGNAL( activated(int) ),
             m_osdPreview,           SLOT( setScreen(int) ) );
    connect( kcfg_OsdEnabled,         SIGNAL( toggled(bool) ),
             m_osdPreview,           SLOT( setShown(bool) ) );

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

    kcfg_OsdText->setToolTip( text.args( QStringList()
            // we don't translate these, it is not sensible to do so
            << "%title"  << "%album"   << "%artist"  << "%genre"     << "%bitrate"
            << "%year "  << "%length"  << "%track"   << "%filename"  << "%directory"
            << "%type"   << "%comment" << "%score"   << "%playcount" << "%discnumber"
            << "%rating" << "%moodbar" << "%elapsed"
            << "%title {" + i18n( "Score: %1" ).arg( "%score" ) +'}' ) );
}

OsdConfig::~OsdConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
OsdConfig::hasChanged()
{
    return false;
}

bool
OsdConfig::isDefault()
{
    return false;
}

void
OsdConfig::updateSettings()
{
}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////

void
OsdConfig::slotPositionChanged()
{
    kcfg_OsdScreen->blockSignals( true );
    kcfg_OsdScreen->setCurrentIndex( m_osdPreview->screen() );
    kcfg_OsdScreen->blockSignals( false );

    // Update button states (e.g. "Apply")
    //emit settingsChanged();
}

void
OsdConfig::hideEvent( QHideEvent* )
{
    m_osdPreview->hide();
}

void
OsdConfig::showEvent( QShowEvent* )
{
    useCustomColorsToggled( kcfg_OsdUseCustomColors->isChecked() );

    //m_osdPreview->setFont( kcfg_OsdFont->font() );
    m_osdPreview->setScreen( kcfg_OsdScreen->currentIndex() );
    m_osdPreview->setShown( kcfg_OsdEnabled->isChecked() );
}

void
OsdConfig::useCustomColorsToggled( bool on )
{
    m_osdPreview->setUseCustomColors( on, kcfg_OsdTextColor->color(), kcfg_OsdBackgroundColor->color() );
}


#include "OsdConfig.moc"


