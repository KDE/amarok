/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TranscodingAssistantDialog.h"
#include "TranscodingJob.h"
#include "core/transcoding/TranscodingController.h"

#include <KIcon>
#include <KPushButton>

namespace Transcoding
{

AssistantDialog::AssistantDialog( QWidget *parent )
    : KDialog( parent, Qt::Dialog )
{
    DEBUG_BLOCK
    QWidget *uiBase = new QWidget( this );
    setMainWidget( uiBase);
    ui.setupUi( uiBase );
    setModal( true );
    setWindowTitle( i18n( "Transcode Tracks" ) );
    setMinimumSize( 590, 340 );
    setMaximumWidth( 590 );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );

    setButtons( Ok|Cancel );
    onCurrentChanged( 0 );
    button( Ok )->setText( i18n( "Transc&ode" ) );

    ui.explanatoryTextLabel->setText( i18n(
            "You are about to copy one or more tracks.\nWhile copying, you can also choose "
            "to transcode your music files into another format with an encoder (codec). "
            "This can be done to save space or to make your files readable by a portable "
            "music player or a particular software program." ) );

    ui.justCopyButton->setIcon( KIcon( "edit-copy" ) );
    ui.transcodeWithDefaultsButton->setIcon( KIcon( "audio-x-generic" ) );
    ui.transcodeWithOptionsButton->setIcon( KIcon( "tools-rip-audio-cd" ) );

    ui.justCopyButton->setMinimumHeight( ui.justCopyButton->iconSize().height() + 2*10 ); //we make room for the pretty icon
    connect( ui.justCopyButton, SIGNAL( clicked() ),
             this, SLOT( onJustCopyClicked() ) );
    ui.transcodeWithDefaultsButton->setMinimumHeight( ui.transcodeWithDefaultsButton->iconSize().height() + 2*10 );
    ui.transcodeWithDefaultsButton->setDescription( i18nc(
            "Attention translators. This description *must* fit in 2 rows, because of a "
            "hardcoded constraint in QCommandLinkButton.",
            "As you copy, transcode the tracks using the preset encoding parameters.\nMedium "
            "compression, high quality Ogg Vorbis (lossy)." ) );
    connect( ui.transcodeWithDefaultsButton, SIGNAL( clicked() ),
             this, SLOT( onTranscodeWithDefaultsClicked() ) );
    ui.transcodeWithOptionsButton->setMinimumHeight( ui.transcodeWithOptionsButton->iconSize().height() + 2*10 );
    connect( ui.transcodeWithOptionsButton, SIGNAL( clicked() ),
             this, SLOT( onTranscodeWithOptionsClicked() ) );


    connect( button( Cancel ), SIGNAL( clicked() ),
             this, SLOT( reject() ) );
    connect( ui.stackedWidget, SIGNAL( currentChanged( int ) ),
             this, SLOT( onCurrentChanged( int ) ) );

    //Let's set up the codecs page...
    ui.backButton->setMinimumSize( ui.formatListWidget->width(), ui.backButton->height() + 5 ); //no description text
    ui.backButton->setMaximumSize( ui.formatListWidget->width(), ui.backButton->height() + 5 );
    ui.backButton->setIcon( KIcon( "go-previous" ) );
    connect( ui.backButton, SIGNAL( clicked() ),
             this, SLOT( onBackClicked() ) );

    populateFormatList();
    connect( ui.formatListWidget, SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ),
             this, SLOT( onFormatSelect( QListWidgetItem * ) ) );

    ui.formatIconLabel->hide();
    ui.formatNameLabel->hide();
    ui.formatDescriptionLabel->hide();
    connect( button( Ok ), SIGNAL( clicked() ),
             this, SLOT( onTranscodeClicked() ) );
}

void
AssistantDialog::populateFormatList()
{
    foreach( Format *format, The::transcodingController()->availableFormats() )
    {
        QListWidgetItem *item = new QListWidgetItem( format->icon(), format->prettyName() );
        item->setToolTip( format->description() );
        item->setData( Qt::UserRole, format->encoder() );
        ui.formatListWidget->addItem( item );
    }
}

void
AssistantDialog::onJustCopyClicked() //SLOT
{
    KDialog::done( KDialog::Accepted );
}

void
AssistantDialog::onTranscodeWithDefaultsClicked() //SLOT
{
    m_configuration = Configuration( VORBIS );
    foreach( Property property, The::transcodingController()->format( VORBIS )->propertyList() )
    {
        switch( property.type() )
        {
        case Property::NUMERIC:
            m_configuration.addProperty( property.name(), property.defaultValue() );
        case Property::TEXT:
            m_configuration.addProperty( property.name(), property.defaultText() );
        case Property::LIST:
            m_configuration.addProperty( property.name(), property.defaultIndex() );
        }
    }
    KDialog::done( KDialog::Accepted );
}

void
AssistantDialog::onTranscodeWithOptionsClicked() //SLOT
{
    ui.stackedWidget->setCurrentIndex( 1 );
}

void
AssistantDialog::onBackClicked() //SLOT
{
    ui.stackedWidget->setCurrentIndex( 0 );
}

void
AssistantDialog::onCurrentChanged( int page ) //SLOT
{
    button( Ok )->setVisible( page );
    if( ui.formatListWidget->currentRow() < 0 )
        button( Ok )->setEnabled( false );
}

void
AssistantDialog::onTranscodeClicked() //SLOT
{
    m_configuration = ui.transcodingOptionsStackedWidget->configuration();
    KDialog::done( KDialog::Accepted );
}

void
AssistantDialog::onFormatSelect( QListWidgetItem *item ) //SLOT
{
    if( item )
    {
        ui.formatIconLabel->show();
        ui.formatNameLabel->show();
        ui.formatDescriptionLabel->show();
        Encoder encoder = static_cast< Encoder >( item->data( Qt::UserRole ).toInt() );
        const Format *format = The::transcodingController()->format( encoder );
        ui.formatIconLabel->setPixmap( format->icon().pixmap( 32, 32 ) );
        ui.formatNameLabel->setText( format->prettyName() );
        ui.formatDescriptionLabel->setText( format->description() );
        ui.transcodingOptionsStackedWidget->switchPage( encoder );
        button( Ok )->setEnabled( true );
    }
}

} //namespace Transcoding

#include "TranscodingAssistantDialog.moc"
