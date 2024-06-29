/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "FormatSelectionDialog.h"

#include "AudioCdCollection.h"

#include <KCMultiDialog>
#include <KLocalizedString>

FormatSelectionDialog::FormatSelectionDialog( QWidget *parent )
    : QDialog( parent )
{
    setupUi( this );

    connect( oggButton, &QRadioButton::toggled, this, &FormatSelectionDialog::selectionChanged );
    connect( flacButton,  &QRadioButton::toggled, this, &FormatSelectionDialog::selectionChanged );
    connect( wavButton,  &QRadioButton::toggled, this, &FormatSelectionDialog::selectionChanged );
    connect( mp3Button,  &QRadioButton::toggled, this, &FormatSelectionDialog::selectionChanged );

    connect( advancedButton,  &QRadioButton::clicked, this, &FormatSelectionDialog::showAdvancedSettings );


    //restore format from last time, if any.
    KConfigGroup config = Amarok::config("Audio CD Collection");
    QString format = config.readEntry( "Import Format", "ogg" );

    if ( format.compare( QStringLiteral("ogg"), Qt::CaseInsensitive ) == 0 )
        oggButton->setChecked( true );
    else if ( format.compare( QStringLiteral("flac"), Qt::CaseInsensitive ) == 0 )
        flacButton->setChecked( true );
    else if ( format.compare( QStringLiteral("wav"), Qt::CaseInsensitive ) == 0 )
        wavButton->setChecked( true );
    else if ( format.compare( QStringLiteral("mp3"), Qt::CaseInsensitive ) == 0 )
        mp3Button->setChecked( true );
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
        descriptionLabel->setText( i18n( "Ogg Vorbis is a fully free and unencumbered compressed audio format that is perfect for storing your compressed music on your computer. The sound quality is slightly better than MP3 at the same bitrate. Note that not all mobile players support the Ogg Vorbis format." ) );

        m_selectedFormat = Collections::AudioCdCollection::OGG;
    }
    else if( sender() == flacButton )
    {
        descriptionLabel->setText( i18n( "FLAC is a lossless compressed audio format free of any patents or license fees. It maintains perfect CD audio quality while reducing file size by about 50%. Because the filesize is much larger than Ogg Vorbis or MP3 it is not recommended if you want to transfer your music to a mobile player." ) );

        m_selectedFormat = Collections::AudioCdCollection::FLAC;
    }
    else if( sender() == wavButton )
    {
        descriptionLabel->setText( i18n( "WAV is a basic, uncompressed audio file format. It takes up a lot of space but maintains perfect quality. It is generally not recommended unless you know what you are doing. If you want perfect quality, use FLAC instead." ) );

        m_selectedFormat = Collections::AudioCdCollection::WAV;
    }
    else if( sender() == mp3Button )
    {
        descriptionLabel->setText( i18n( "MP3 is the de facto standard in compressed audio compatible with almost all mobile players. It is however non free and generally not recommended." ) );

        m_selectedFormat = Collections::AudioCdCollection::MP3;
    }
        
}

void FormatSelectionDialog::accept()
{

    //store to config for next download:

    QString format;
    
    if( m_selectedFormat == Collections::AudioCdCollection::OGG )
        format = QStringLiteral("ogg");
    else if( m_selectedFormat == Collections::AudioCdCollection::FLAC )
        format = QStringLiteral("flac");
    else if( m_selectedFormat == Collections::AudioCdCollection::WAV )
        format = QStringLiteral("wav");
    else if( m_selectedFormat == Collections::AudioCdCollection::MP3 )
        format = QStringLiteral("mp3");

    KConfigGroup config = Amarok::config("Audio CD Collection");
    config.writeEntry( "Import Format", format );
    
    Q_EMIT formatSelected( m_selectedFormat );
    QDialog::accept();
}

void FormatSelectionDialog::showAdvancedSettings()
{
    KCMultiDialog KCM;
    KCM.setWindowTitle( i18n( "Audio CD settings - Amarok" ) );
    QString searchPath = QStringLiteral( "plasma/kcms/systemsettings_qwidgets/kcm_audiocd.so" );
    KPluginMetaData plugin = KPluginMetaData( searchPath );
    while( !plugin.isValid() && searchPath.indexOf( QLatin1Char('/') ) >= 0 )
    {
        searchPath = searchPath.mid( searchPath.indexOf( QLatin1Char('/') ) + 1 );
        qDebug() << "didn't find kcm_audiocd with first attempt, trying"<< searchPath;
        plugin = KPluginMetaData( searchPath );
    }
    KCM.addModule( plugin );

    KCM.exec();
}



