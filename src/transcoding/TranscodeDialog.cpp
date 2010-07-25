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

#include "TranscodeDialog.h"
#include "TranscodeJob.h"

#include <KIcon>
#include <KPushButton>

TranscodeDialog::TranscodeDialog( QWidget *parent )
    : KDialog( parent, Qt::Dialog )
    , m_format( TranscodeFormat::Null() )
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

    ui.explanatoryTextLabel->setText( i18n( "You are about to copy one or more tracks.\nWhile copying, you can also choose to transcode your music files into another format with an encoder (codec). This can be done to save space or to make your files readable by a portable music player or a particular software program." ) );

    ui.justCopyButton->setIcon( KIcon( "edit-copy" ) );
    ui.transcodeWithDefaultsButton->setIcon( KIcon( "audio-x-generic" ) );
    ui.transcodeWithOptionsButton->setIcon( KIcon( "tools-rip-audio-cd" ) );

    ui.justCopyButton->setMinimumHeight( ui.justCopyButton->iconSize().height() + 2*10 ); //we make room for the pretty icon
    connect( ui.justCopyButton, SIGNAL( clicked() ),
             this, SLOT( onJustCopyClicked() ) );
    ui.transcodeWithDefaultsButton->setMinimumHeight( ui.transcodeWithDefaultsButton->iconSize().height() + 2*10 );
    ui.transcodeWithDefaultsButton->setDescription(
            i18nc( "Attention translators. This description must fit in 2 rows, because of a hardcoded constraint in QCommandLinkButton.",
                   "As you copy, transcode the tracks using the preset encoding parameters.\nMedium compression, high quality Ogg Vorbis (lossy).") );
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
}

void
TranscodeDialog::populateFormatList()
{
    for( int i = 1 /*we skip the null codec*/; i < TranscodeFormat::NUM_CODECS; ++i )
    {
        TranscodeFormat::Encoder encoder = static_cast< TranscodeFormat::Encoder >( i );
        QString prettyName = TranscodeFormat::prettyName( encoder );
        QString description = TranscodeFormat::description( encoder );
        KIcon icon = TranscodeFormat::icon( encoder );
        QListWidgetItem *item = new QListWidgetItem( icon, prettyName );
        item->setToolTip( description );
        item->setData( Qt::UserRole, encoder );
        ui.formatListWidget->addItem( item );
    }
}

void
TranscodeDialog::onJustCopyClicked() //SLOT
{
    KDialog::done( KDialog::Accepted );
}

void
TranscodeDialog::onTranscodeWithDefaultsClicked() //SLOT
{
    m_format = TranscodeFormat::Vorbis();
    KDialog::done( KDialog::Accepted );
}

void
TranscodeDialog::onTranscodeWithOptionsClicked() //SLOT
{
    ui.stackedWidget->setCurrentIndex( 1 );
}

void
TranscodeDialog::onBackClicked() //SLOT
{
    ui.stackedWidget->setCurrentIndex( 0 );
}

void
TranscodeDialog::onCurrentChanged( int page ) //SLOT
{
    button( Ok )->setVisible( page );
}

TranscodeFormat
TranscodeDialog::transcodeFormat() const
{
    return m_format;
}

void
TranscodeDialog::onFormatSelect( QListWidgetItem *item ) //SLOT
{
    if( item )
    {
        ui.formatIconLabel->show();
        ui.formatNameLabel->show();
        ui.formatDescriptionLabel->show();
        TranscodeFormat::Encoder encoder = static_cast< TranscodeFormat::Encoder >( item->data( Qt::UserRole ).toInt() );
        ui.formatIconLabel->setPixmap( TranscodeFormat::icon( encoder).pixmap( 32, 32 ) );
        ui.formatNameLabel->setText( TranscodeFormat::prettyName( encoder ) );
        ui.formatDescriptionLabel->setText( TranscodeFormat::description( encoder ) );
        ui.transcodeOptionsWidget->switchPage( encoder );
    }
}

#include "TranscodeDialog.moc"
