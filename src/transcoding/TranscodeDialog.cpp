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

TranscodeDialog::TranscodeDialog( const KUrl::List &urlList, QWidget *parent )
    : KDialog( parent, Qt::Dialog )
{
    DEBUG_BLOCK
    QWidget *uiBase = new QWidget( this );
    setMainWidget( uiBase);
    ui.setupUi( uiBase );
    setModal( true );
    setWindowTitle( i18n( "Transcode Tracks" ) );


    TranscodeFormat format = TranscodeFormat::Vorbis( 7 );
    debug() << "\nFormat encoder is: " << format.encoder();
    debug() << "\nabout to fetch ffmpeg parameters";
    QString ffmpp = format.ffmpegParameters();
    debug() << "\nParameters: ";
    debug() << ffmpp.toAscii();
    /*KJob *doTranscode = new TranscodeJob( url, *format,this );
    //doTranscode->start();
    */
    setButtons( User1 );
    button( User1 ).setText( "LOL Transcode NAO!" );
    connect( button( User1 ), SIGNAL( clicked() ), this, SLOT(  ) );
}

#include "TranscodeDialog.moc"
