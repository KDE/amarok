/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#include "CoverFoundDialog.h"
#include "Debug.h"
#include "PaletteHandler.h"

#include <KLocale>
#include <KVBox>
#include <KPushButton>

#include <QCloseEvent>
#include <QGridLayout>

CoverFoundDialog::CoverFoundDialog( QWidget *parent,
                                    Meta::AlbumPtr album,
                                    const QList<QPixmap> &covers )
    : KDialog( parent )
    , m_album( album )
    , m_covers( covers )
    , m_index( 0 )
{
    setButtons( KDialog::Ok     |
                KDialog::Details |
                KDialog::Cancel |
                KDialog::User1  | // next
                KDialog::User2 ); // prev

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::forward() );
    setButtonGuiItem( KDialog::User2, KStandardGuiItem::back() );

    m_next = button( KDialog::User1 );
    m_prev = button( KDialog::User2 );
    m_save = button( KDialog::Ok );

    setButtonText( KDialog::User1, QString() );
    setButtonText( KDialog::User2, QString() );

    m_prev->hide();
    m_next->hide();

    m_labelPixmap = new QLabel( this );
    m_labelPixmap->setMinimumHeight( 300 );
    m_labelPixmap->setMinimumWidth( 300 );
    m_labelPixmap->setAlignment( Qt::AlignCenter );
    m_labelPixmap->setPixmap( covers.first() );
    m_labelPixmap->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    QFrame *m_details = new QFrame( this );
    m_details->setFrameShadow( QFrame::Plain );
    m_details->setFrameShape( QFrame::Box );

    QLabel *artistLabel = new QLabel( "<b>" + i18n( "Artist" )     + "</b>", m_details );
    QLabel *albumLabel  = new QLabel( "<b>" + i18n( "Album"  )     + "</b>", m_details );
    QLabel *sizeLabel   = new QLabel( "<b>" + i18n( "Cover size" ) + "</b>", m_details );

    artistLabel->setAlignment( Qt::AlignRight );
    albumLabel->setAlignment( Qt::AlignRight );
    sizeLabel->setAlignment( Qt::AlignRight );

    m_detailsLayout = new QGridLayout( m_details );
    m_detailsLayout->addWidget( artistLabel, 0, 0 );
    m_detailsLayout->addWidget( albumLabel,  1, 0 );
    m_detailsLayout->addWidget( sizeLabel,   2, 0 );
    m_detailsLayout->addWidget( new QLabel( m_details ), 0, 1 );
    m_detailsLayout->addWidget( new QLabel( m_details ), 1, 1 );
    m_detailsLayout->addWidget( new QLabel( m_details ), 2, 1 );

    setMainWidget( m_labelPixmap );
    setDetailsWidget( m_details );

    connect( m_prev, SIGNAL(clicked()), SLOT(prevPix()) );
    connect( m_save, SIGNAL(clicked()), SLOT(accept())  );
    connect( m_next, SIGNAL(clicked()), SLOT(nextPix()) );

    updateGui();
    updatePixmap();
}

void CoverFoundDialog::resizeEvent( QResizeEvent *event )
{
    if( m_labelPixmap && !m_labelPixmap->pixmap()->isNull() )
    {
        const QSize pixmapSize = m_labelPixmap->pixmap()->size();
        QSize scaledSize = pixmapSize;
        scaledSize.scale( m_labelPixmap->size(), Qt::KeepAspectRatio );

        if( scaledSize != pixmapSize )
            updatePixmap();
    }
    QWidget::resizeEvent( event );
}

void CoverFoundDialog::closeEvent( QCloseEvent *event )
{
    m_index = 0;
    m_covers.clear();
    event->accept();
}

void CoverFoundDialog::wheelEvent( QWheelEvent *event )
{
    if( event->delta() > 0 )
        prevPix();
    else
        nextPix();

    event->accept();
}

void CoverFoundDialog::updateGui()
{
    updateTitle();
    updateDetails();
    updateButtons();

    setButtonFocus( KDialog::Ok );
    update();
}

void CoverFoundDialog::updatePixmap()
{
    m_labelPixmap->setPixmap( m_covers.at( m_index ).scaled( m_labelPixmap->size(),
                                                             Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation) );
}

void CoverFoundDialog::updateButtons()
{
    const int count = m_covers.length();

    if( count > 1 )
    {
        m_prev->show();
        m_next->show();
    }
    else
    {
        return;
    }

    if( m_index < count - 1 )
        m_next->setEnabled( true );
    else
        m_next->setEnabled( false );

    if( m_index == 0 )
        m_prev->setEnabled( false );
    else
        m_prev->setEnabled( true );
}

void CoverFoundDialog::updateDetails()
{
    const QString artist = m_album->hasAlbumArtist()
                         ? m_album->albumArtist()->prettyName()
                         : i18n( "Various Artists" );

    const QPixmap pixmap = m_covers.at( m_index );

    QLabel *artistName = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 0, 1 )->widget() );
    QLabel *albumName  = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 1, 1 )->widget() );
    QLabel *coverSize  = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 2, 1 )->widget() );

    artistName->setText( artist );
    albumName->setText( m_album->prettyName() );
    coverSize->setText( QString::number( pixmap.width() ) + 'x' + QString::number( pixmap.height() ) );
}

void CoverFoundDialog::updateTitle()
{
    QString caption = i18n( "Cover Found" );

    if( m_covers.size() > 1 )
    {
        const QString position = i18n( "%1/%2", m_index + 1, m_covers.size() );
        caption +=  ": " + position;
    }
    this->setCaption( caption );
}

//SLOT
void CoverFoundDialog::add( QPixmap cover )
{
    m_covers << cover;
    updateGui();
}

//SLOT
void CoverFoundDialog::add( QList< QPixmap > covers )
{
    m_covers << covers;
    updateGui();
}

//SLOT
void CoverFoundDialog::accept()
{
    if( qstrcmp( sender()->objectName().toAscii(), "NewSearch" ) == 0 )
        done( 1000 );
    else if( qstrcmp( sender()->objectName().toAscii(), "NextCover" ) == 0 )
        done( 1001 );
    else
        KDialog::accept();
}

//SLOT
void CoverFoundDialog::nextPix()
{
    if( m_index < m_covers.length() - 1 )
    {
        m_index++;
        updateGui();
        updatePixmap();
    }
}

//SLOT
void CoverFoundDialog::prevPix()
{
    if( m_index >= 1 )
    {
        m_index--;
        updateGui();
        updatePixmap();
    }
}

#include "CoverFoundDialog.moc"
