/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "FormatSelectionDialog.h"

#include "AudioCdCollection.h"

FormatSelectionDialog::FormatSelectionDialog( QWidget *parent )
    : QDialog( parent )
{
    setupUi( this );

    connect( oggButton, SIGNAL( toggled( bool ) ), this, SLOT( selectionChanged( bool ) ) );
    connect( flacButton, SIGNAL( toggled( bool ) ), this, SLOT( selectionChanged( bool ) ) );
    connect( wavButton, SIGNAL( toggled( bool ) ), this, SLOT( selectionChanged( bool ) ) );
    connect( mp3Button, SIGNAL( toggled( bool ) ), this, SLOT( selectionChanged( bool ) ) );
    
    oggButton->setChecked( true );
}


FormatSelectionDialog::~FormatSelectionDialog()
{
}

void FormatSelectionDialog::selectionChanged( bool checked )
{
    if ( !checked )
        return;

    if( sender() == oggButton )
    {
        descriptionLabel->setText( i18n( "Ogg Vorbis is a fully free and unencumbered compressed audio format that is perfect for storing your compressed music on your computer. The sound quality is slightly better than Mp3 at the same bitrate. Note that not all mobile player support the Ogg Vorbis format" ) );

        m_selectedFormat = AudioCdCollection::OGG;
    }
    else if( sender() == flacButton )
    {
        descriptionLabel->setText( i18n( "Flac is a lossless compressed audio format free of any patents or licence fees. It maintains perfect CD audio quality while reducing fivawle size by about 50%. Because the filesize is much larger than Ogg Vorbis or Mp3 it is not reccomeded if you want to transfer your music to a mobile player." ) );

        m_selectedFormat = AudioCdCollection::FLAC;
    }
    else if( sender() == wavButton )
    {
        descriptionLabel->setText( i18n( "Wav is a basic, uncompressed audio file format. It takes up a lot of space but maintains perfect quality. It is generally not reccomended unless you know what you are doing. If you want perfect quailty, use Flac instead." ) );

        m_selectedFormat = AudioCdCollection::WAV;
    }
    else if( sender() == mp3Button )
    {
        descriptionLabel->setText( i18n( "Mp3 is the defacto standard in compressed audio compatible with almost all mobile players. It is however non free and genrally not reccomended" ) );

        m_selectedFormat = AudioCdCollection::MP3;
    }
        
}

void FormatSelectionDialog::accept()
{
    emit formatSelected( m_selectedFormat );
    QDialog::accept();
}


#include "FormatSelectionDialog.moc"

