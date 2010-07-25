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

#include "TranscodeOptionsWidget.h"

#include "core/support/Debug.h"

#include <KIcon>

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

TranscodeOptionsWidget::TranscodeOptionsWidget( QWidget *parent )
    : QStackedWidget( parent )
{
    initWelcomePage();
    for( int i = 1 /*we skip the null codec*/; i < TranscodeFormat::NUM_CODECS; ++i )
    {
        TranscodeFormat::Encoder encoder = static_cast< TranscodeFormat::Encoder >( i );
        m_pagesMap.insert( encoder, initCodecPage( encoder ) );
    }


}

void
TranscodeOptionsWidget::initWelcomePage()
{
    QWidget *welcomeWidget = new QWidget( this );
    QVBoxLayout *vbl = new QVBoxLayout( welcomeWidget );
    vbl->addStretch();
    QHBoxLayout *hbl = new QHBoxLayout( welcomeWidget );
    vbl->addLayout( hbl );
    hbl->addStretch();
    QLabel *arrow = new QLabel( welcomeWidget );
    arrow->setPixmap( KIcon( "arrow-left" ).pixmap( 16, 16 ) );
    QLabel *message = new QLabel( i18n( "In order to configure the parameters of the transcoding operation, please pick an encoder from the list." ), this );
    message->setWordWrap( true );
    hbl->addWidget( arrow );
    hbl->addWidget( message );
    hbl->addStretch();
    vbl->addStretch();

    insertWidget( 0, welcomeWidget );
}

int
TranscodeOptionsWidget::initCodecPage( TranscodeFormat::Encoder encoder )
{
    QWidget *codecWidget = new QWidget( this );

    QVBoxLayout *mainLayout = new QVBoxLayout( codecWidget );
    mainLayout->addStretch( 1 );
    if( encoder == TranscodeFormat::AAC ||
        encoder == TranscodeFormat::VORBIS ||
        encoder == TranscodeFormat::WMA2 )
    {
        QHBoxLayout *lineLayout = new QHBoxLayout( codecWidget );
        mainLayout->addLayout( lineLayout );
        QLabel *qualityLabel = new QLabel( i18n( "Quality" ), codecWidget );
        lineLayout->addWidget( qualityLabel );
        QSpinBox *qualityEdit = new QSpinBox( codecWidget );
        lineLayout->addWidget( qualityEdit );
        qualityEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
        lineLayout->setStretch( lineLayout->indexOf( qualityEdit ), 1 );
        lineLayout->addStretch( 2 );
    }
    else if( encoder == TranscodeFormat::ALAC )
    {
        //NO IDEA WHAT TO DO WITH THIS!
    }
    else if( encoder == TranscodeFormat::FLAC )
    {
        //initLevel();
    }
    else if( encoder == TranscodeFormat::MP3 )
    {
        //initVRating();
    }
    else
    {
        debug() << "Bad encoder.";
    }

    return addWidget( codecWidget );
}

void
TranscodeOptionsWidget::switchPage(TranscodeFormat::Encoder encoder)
{
    setCurrentIndex( m_pagesMap.value( encoder ) );
    emit formatChanged( encoder );
}
