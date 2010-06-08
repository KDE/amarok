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

TranscodeDialog::TranscodeDialog( const KUrl::List &urlList, QWidget *parent )
    : KDialog( parent, Qt::Dialog )
    , m_urlList( urlList )
{
    DEBUG_BLOCK
    QWidget *uiBase = new QWidget( this );
    setMainWidget( uiBase);
    ui.setupUi( uiBase );
    setModal( true );
    setWindowTitle( i18n( "Transcode Tracks" ) );

    setButtons( None );
    connect( ui.transcodeWithDefaultsButton, SIGNAL( clicked() ), this, SLOT( onTranscodeClicked() ) );

    ui.explanatoryTextLabel->setText( i18n( "You are about to copy one or more tracks.\nWhile copying, you can also choose to transcode your music files into another format with an encoder (codec). This can be done to save space or to make your files readable by a portable music player or a particular software program." ) );

    ui.justCopyButton->setIcon( KIcon( "edit-copy" ) );
    ui.transcodeWithDefaultsButton->setIcon( KIcon( "audio-x-generic" ) );
    ui.transcodeWithOptionsButton->setIcon( KIcon( "tools-rip-audio-cd" ) );

    connect( ui.buttonBox->button( KDialogButtonBox::Cancel ), SIGNAL( clicked() ), this, SLOT( reject() ) );
}

void
TranscodeDialog::onTranscodeClicked() //SLOT
{
    /*TranscodeFormat format = TranscodeFormat::Vorbis( ui.spinBox->value() );
    debug() << "\nFormat encoder is: " << format.encoder();
    debug() << "\nabout to fetch ffmpeg parameters";
    debug() << "\nParameters: ";
    debug() << format.ffmpegParameters();
    KUrl url = m_urlList.first();
    KJob *doTranscode = new TranscodeJob( url, format,this );
    doTranscode->start();*/

    KDialog::done( KDialog::Accepted );
}

#include "TranscodeDialog.moc"
