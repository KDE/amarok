/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * Maintainer:        Robert Gogolok <gogo@graphics.cs.uni-sb.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include <kdialog.h>
#include <klocale.h>

#include "nmm_configdialogbase.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>


NmmConfigDialogBase::NmmConfigDialogBase( QWidget* parent )
    : QWidget( parent )
{
    setCaption( i18n( "NMM Engine Configuration - amaroK" ) );

    QVBoxLayout *NmmConfigDialogBaseLayout = new QVBoxLayout( this, 11, 6 ); 

    QLabel *nmmLogo = new QLabel( this );
    nmmLogo->setPaletteForegroundColor( QColor( 0, 0, 0 ) );
    nmmLogo->setPaletteBackgroundColor( QColor( 255, 255, 255 ) );
    nmmLogo->setFrameShape( QLabel::StyledPanel );
    nmmLogo->setFrameShadow( QLabel::Raised );
    nmmLogo->setMargin( 1 );
    QPixmap nmm_logo( locate( "data", "amarok/images/nmm_logo.png" ) );
    nmmLogo->setPixmap( nmm_logo );
    nmmLogo->setAlignment( int( QLabel::AlignCenter ) );
    NmmConfigDialogBaseLayout->addWidget( nmmLogo );


    /* audio plugin selection */
    QHBoxLayout *audiopluginlayout = new QHBoxLayout( 0, 0, 6); 

    QLabel *audioPluginLabel = new QLabel( i18n( "Audio plugin:" ), this );
    audiopluginlayout->addWidget( audioPluginLabel );

    audioPlaybackNode = new KComboBox( false, this );
    audioPlaybackNode->clear();
    audioPlaybackNode->insertItem( i18n( "PlaybackNode" ) );
    audioPlaybackNode->insertItem( i18n( "ALSAPlaybackNode" ) );
    audiopluginlayout->addWidget( audioPlaybackNode );

    NmmConfigDialogBaseLayout->addLayout( audiopluginlayout );


    /* audio location group */
    audioGroup = new QButtonGroup( i18n( "Audio Location" ), this );
    audioGroup->setColumnLayout(0, Qt::Vertical );
    audioGroup->layout()->setSpacing( 6 );
    audioGroup->layout()->setMargin( 11 );
    QVBoxLayout *audioGroupLayout = new QVBoxLayout( audioGroup->layout() );
    audioGroupLayout->setAlignment( Qt::AlignTop );

    QVBoxLayout *audiogrouplayout = new QVBoxLayout( 0, 0, 6 );

    /* audio localhost button */
    audioLocalhostButton = new QRadioButton( i18n( "Localhost only" ), audioGroup );
    audiogrouplayout->addWidget( audioLocalhostButton );

    /* audio host list */
    audioHostListButton = new QRadioButton( i18n( "Host list:" ), audioGroup );
    audiogrouplayout->addWidget( audioHostListButton );

    QHBoxLayout *audioHostListHBox = new QHBoxLayout( 0, 0, 6 ); 
    QSpacerItem *spacer1 = new QSpacerItem( 20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    audioHostListHBox->addItem( spacer1 );

    audioListBox = new QListBox( audioGroup, "audioListBox" );
    audioHostListHBox->addWidget( audioListBox );

    QVBoxLayout *audioAddRemoveVBox = new QVBoxLayout( 0, 0, 6 ); 

    /* add audio host button */
    addAudioLocationButton = new QPushButton( i18n( "Add..." ), audioGroup );
    audioAddRemoveVBox->addWidget( addAudioLocationButton );

    /* remove audio host button */
    removeAudioLocationButton = new QPushButton( i18n( "R&emove" ), audioGroup );
    audioAddRemoveVBox->addWidget( removeAudioLocationButton );

    /* add,remove spacer */
    QSpacerItem *spacer_audioAddRemove = new QSpacerItem( 20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding );
    audioAddRemoveVBox->addItem( spacer_audioAddRemove );
    audioHostListHBox->addLayout( audioAddRemoveVBox );
    audiogrouplayout->addLayout( audioHostListHBox );

    /* audio environment button */
    audioEnvironmentButton = new QRadioButton( i18n( "Enviroment variable $SOUND or $DISPLAY" ), audioGroup );
    audiogrouplayout->addWidget( audioEnvironmentButton );

    audioGroupLayout->addLayout( audiogrouplayout );
    NmmConfigDialogBaseLayout->addWidget( audioGroup );


    /* video location group */
    videoGroup = new QButtonGroup( i18n( "Video Location" ), this );
    videoGroup->setColumnLayout(0, Qt::Vertical );
    videoGroup->layout()->setSpacing( 6 );
    videoGroup->layout()->setMargin( 11 );
    QVBoxLayout *videoGroupLayout = new QVBoxLayout( videoGroup->layout() );
    videoGroupLayout->setAlignment( Qt::AlignTop );

    QVBoxLayout *videogrouplayout = new QVBoxLayout( 0, 0, 6); 

    /* video localhost button */
    videoLocalhostButton = new QRadioButton( i18n( "Localhost only" ), videoGroup );
    videogrouplayout->addWidget( videoLocalhostButton );

    /* video host list */
    videoHostListButton = new QRadioButton( i18n( "Host list:" ), videoGroup );
    videogrouplayout->addWidget( videoHostListButton );

    QHBoxLayout *videoHostListHBox = new QHBoxLayout( 0, 0, 6 ); 
    QSpacerItem *spacer2 = new QSpacerItem( 20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum );
    videoHostListHBox->addItem( spacer2 );

    videoListBox = new QListBox( videoGroup );
    videoHostListHBox->addWidget( videoListBox );

    QVBoxLayout *videoAddRemoveVBox = new QVBoxLayout( 0, 0, 6 ); 

    /* add video host button */
    addVideoLocationButton = new QPushButton( i18n( "Add..." ), videoGroup );
    videoAddRemoveVBox->addWidget( addVideoLocationButton );

    /* remove video host button */
    removeVideoLocationButton = new QPushButton( i18n( "R&emove" ), videoGroup );
    videoAddRemoveVBox->addWidget( removeVideoLocationButton );

    /* add,remove spacer */
    QSpacerItem *spacer_videoAddRemove = new QSpacerItem( 20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding );
    videoAddRemoveVBox->addItem( spacer_videoAddRemove );
    videoHostListHBox->addLayout( videoAddRemoveVBox );
    videogrouplayout->addLayout( videoHostListHBox );

    /* video environment button */
    videoEnvironmentButton = new QRadioButton( i18n( "Enviroment variable $DISPLAY" ), videoGroup );
    videogrouplayout->addWidget( videoEnvironmentButton );
    videoGroupLayout->addLayout( videogrouplayout );
    NmmConfigDialogBaseLayout->addWidget( videoGroup );

    /* bottom spacer */
    QSpacerItem *bottom_spacer = new QSpacerItem( 20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding );
    NmmConfigDialogBaseLayout->addItem( bottom_spacer );
    
}

NmmConfigDialogBase::~NmmConfigDialogBase()
{
}

#include "nmm_configdialogbase.moc"
