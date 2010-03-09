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

#include "Amarok.h"
#include "Debug.h"

#include <KHBox>
#include <KLineEdit>
#include <KListWidget>
#include <KPushButton>
#include <KStandardDirs>
#include <KVBox>

#include <QCloseEvent>
#include <QDir>
#include <QFrame>
#include <QGridLayout>

#define DEBUG_PREFIX "CoverFoundDialog"

CoverFoundDialog::CoverFoundDialog( QWidget *parent,
                                    Meta::AlbumPtr album,
                                    const QList<QPixmap> &covers )
    : KDialog( parent )
    , m_album( album )
    , m_covers( covers )
{
    setButtons( KDialog::Ok | KDialog::Details | KDialog::Cancel |
                KDialog::User1 ); // User1: clear icon view
    setInitialSize( QSize( 480, 350 ) );

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::clear() );
    connect( button( KDialog::User1 ), SIGNAL(clicked()), SLOT(clearView()) );

    m_save = button( KDialog::Ok );

    KVBox *box = new KVBox( this );
    box->setSpacing( 4 );

    KHBox *searchBox = new KHBox( box );
    box->setSpacing( 4 );

    m_search = new KLineEdit( searchBox );
    m_search->setClearButtonShown( true );
    m_search->setClickMessage( i18n( "Enter Custom Search" ) );

    KPushButton *searchButton = new KPushButton( KStandardGuiItem::find(), searchBox );

    connect( m_search, SIGNAL(returnPressed(const QString&)),
             this,     SIGNAL(newCustomQuery(const QString&)) );
    connect( searchButton, SIGNAL(pressed()),
             this,         SLOT(searchButtonPressed()) );

    m_view = new KListWidget( box );
    m_view->setGridSize( QSize( 140, 140 ) );
    m_view->setIconSize( QSize( 120, 120 ) );
    m_view->setSpacing( 4 );
    m_view->setUniformItemSizes( true );
    m_view->setViewMode( QListView::IconMode );

    connect( m_view, SIGNAL(itemClicked(QListWidgetItem*)),
             this,   SLOT(itemClicked(QListWidgetItem*)) );
    connect( m_view, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this,   SLOT(itemDoubleClicked(QListWidgetItem*)) );

    QFrame *m_details = new QFrame( this );
    m_details->setFrameShadow( QFrame::Plain );
    m_details->setFrameShape( QFrame::Box );

    QLabel *artistLabel = new QLabel( "<b>" + i18n( "Artist" ) + "</b>", m_details );
    QLabel *albumLabel  = new QLabel( "<b>" + i18n( "Album"  ) + "</b>", m_details );

    artistLabel->setAlignment( Qt::AlignRight );
    albumLabel->setAlignment( Qt::AlignRight );

    m_detailsLayout = new QGridLayout( m_details );
    m_detailsLayout->addWidget( artistLabel, 0, 0 );
    m_detailsLayout->addWidget( albumLabel,  1, 0 );
    m_detailsLayout->addWidget( new QLabel( m_details ), 0, 1 );
    m_detailsLayout->addWidget( new QLabel( m_details ), 1, 1 );

    setMainWidget( box );
    setDetailsWidget( m_details );

    connect( m_save, SIGNAL(clicked()), SLOT(accept()) );

    add( covers );
    CoverFoundItem *firstItem = dynamic_cast< CoverFoundItem* >( m_view->item( 0 ) );
    if( firstItem )
        m_pixmap = firstItem->pixmap();

    updateGui();
}

void CoverFoundDialog::keyPressEvent( QKeyEvent *event )
{
    if( !m_search->hasFocus() )
        KDialog::keyPressEvent( event );
}

void CoverFoundDialog::closeEvent( QCloseEvent *event )
{
    m_covers.clear();
    event->accept();
}

void CoverFoundDialog::clearView()
{
    m_view->clear();
    m_covers.clear();
    updateGui();
}

void CoverFoundDialog::itemClicked( QListWidgetItem *item )
{
    m_pixmap = dynamic_cast< CoverFoundItem* >( item )->pixmap();
    updateDetails();
}


void CoverFoundDialog::itemDoubleClicked( QListWidgetItem *item )
{
    m_pixmap = dynamic_cast< CoverFoundItem* >( item )->pixmap();
    KDialog::accept();
}

void CoverFoundDialog::searchButtonPressed()
{
    const QString text = m_search->text();
    emit newCustomQuery( text );
}

void CoverFoundDialog::updateGui()
{
    updateTitle();
    updateDetails();

    if( !m_search->hasFocus() )
        setButtonFocus( KDialog::Ok );
    update();
}

void CoverFoundDialog::updateDetails()
{
    if( m_album )
    {
        const QPixmap pixmap = m_pixmap;
        const QString artist = m_album->hasAlbumArtist()
                             ? m_album->albumArtist()->prettyName()
                             : i18n( "Various Artists" );

        QLabel *artistName = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 0, 1 )->widget() );
        QLabel *albumName  = qobject_cast< QLabel * >( m_detailsLayout->itemAtPosition( 1, 1 )->widget() );

        artistName->setText( artist );
        albumName->setText( m_album->prettyName() );
    }
}

void CoverFoundDialog::updateTitle()
{
    const QString caption = m_covers.isEmpty()
                          ? i18n( "Cover Not Found" )
                          : i18np( "1 Cover Found", "%1 Covers Found", m_view->count() );
    this->setCaption( caption );
}

void CoverFoundDialog::add( QPixmap cover )
{
    m_covers << cover;
    CoverFoundItem *item = new CoverFoundItem( cover );

    const QString size = QString( "%1x%2" )
        .arg( QString::number( cover.width() ) )
        .arg( QString::number( cover.height() ) );
    const QString tip = i18n( "Size:" ) + size;
    item->setToolTip( tip );

    m_view->addItem( item );
}

void CoverFoundDialog::add( QList< QPixmap > covers )
{
    foreach( const QPixmap &cover, covers )
    {
        add( cover );
    }
    updateGui();
}

#include "CoverFoundDialog.moc"
