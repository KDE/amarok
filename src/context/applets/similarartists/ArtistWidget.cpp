/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
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

#include "ArtistWidget.h"

//Amarok
#include "Debug.h"

//Qt
#include <QGridLayout>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <QLabel>

ArtistWidget::ArtistWidget(QWidget *parent) : QWidget(parent)
{
    //TODO not use a fixed size
    setFixedSize(420, 240);

    m_layout=new QGridLayout(this);
    
    m_image=new QLabel( this );
    m_image->setAttribute( Qt::WA_TranslucentBackground, true); // The background of the QLabel is transparent
    m_image->setScaledContents(true);                           // The QLabel scale is content
    
    m_name=new QLabel( this );
    m_name->setAttribute( Qt::WA_TranslucentBackground, true); // The background of the QLabel is transparent
    m_name->setAlignment(Qt::AlignCenter);

    m_genre=new QLabel( this );
    m_genre->setAttribute( Qt::WA_TranslucentBackground, true); // The background of the QLabel is transparent
    m_genre->setAlignment(Qt::AlignCenter);


    m_layout->addWidget(m_image,0,0,2,1); // the image display is extended on two row
    m_layout->addWidget(m_name,0,1);
    m_layout->addWidget(m_genre,1,1);

}


ArtistWidget::~ArtistWidget()
{
     delete m_layout;
     delete m_image;
     delete m_name;
     delete m_genre;
}


 /**
  * Change the photo of the artist
  * @param photo The new artist photo
  */
void
ArtistWidget::setPhoto( const QPixmap & photo) {
    m_image->setPixmap(photo);
}

 /**
  * Change the photo of the artist with a photo load from an Url
  * @param photo The url of the new artist photo
  */
void ArtistWidget::setPhoto(const KUrl& urlPhoto)
{
    KJob* job = KIO::storedGet( urlPhoto, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result( KJob* )), SLOT(setImageFromInternet( KJob* ) ));
}


void ArtistWidget::setImageFromInternet(KJob* job)
{
    if( job )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        QPixmap image;
        image.loadFromData(storedJob->data());
        m_image->setPixmap(image);
    }
    else
    {
        m_image->clear();
        m_image->setText(i18n("No picture"));
    }
}


/**
 * Change the artist name and the url which permit to display a page
 * which contains informations about this artist
 * @param nom The name of this artist
 * @param url The url of the artist about page
 */
void
ArtistWidget::setArtist( const QString &nom, const KUrl &url) {
    DEBUG_BLOCK
    m_name->setText("<a href='" + url.url() + "'>" + nom +"</a>");
}

 /**
  * Change the match pourcentage of the artist
  * @param match The match of this artist
  */
void
ArtistWidget::setMatch( const int match) {
    m_genre->setText(i18n( "Match") + " : " + QString::number(match) + "%");
}

/**
 * Clean the widget => the content of the QLabel are empty
 */
void
ArtistWidget::clear() {
    m_image->clear();
    m_name->clear();
    m_genre->clear();
}