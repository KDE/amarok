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

#include <KLocale>
#include <KVBox>
#include <KPushButton>

#include <QCloseEvent>

CoverFoundDialog::CoverFoundDialog( QWidget *parent, const QList<QPixmap> &covers )
    : KDialog( parent )
    , m_covers( covers )
    , m_index( 0 )
{
    setButtons( KDialog::Ok     |
                KDialog::Cancel |
                KDialog::User1  | // next
                KDialog::User2 ); // prev

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::forward() );
    setButtonGuiItem( KDialog::User2, KStandardGuiItem::back() );

    m_next = button( KDialog::User1 );
    m_prev = button( KDialog::User2 );
    m_save = button( KDialog::Ok );

    m_label = new QLabel( this );
    m_label->setMinimumHeight( 300 );
    m_label->setMinimumWidth( 300 );
    m_label->setAlignment( Qt::AlignCenter );
    m_label->setPixmap( covers.first() );
    m_label->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    setMainWidget( m_label );

    connect( m_prev, SIGNAL(clicked()), SLOT(prevPix()) );
    connect( m_save, SIGNAL(clicked()), SLOT(accept())  );
    connect( m_next, SIGNAL(clicked()), SLOT(nextPix()) );

    updateGui();
    updatePixmap();
}

void CoverFoundDialog::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event )

    if( m_label && !m_label->pixmap()->isNull() )
    {
        const QSize pixmapSize = m_label->pixmap()->size();
        QSize scaledSize = pixmapSize;
        scaledSize.scale( m_label->size(), Qt::KeepAspectRatio );

        if( scaledSize != pixmapSize )
            updatePixmap();
    }
    QWidget::resizeEvent(event);
}

void CoverFoundDialog::closeEvent( QCloseEvent *event )
{
    m_index = 0;
    m_covers.clear();
    event->accept();
}

void CoverFoundDialog::updateGui()
{
    setTitle();
    updateButtons();
    m_label->setPixmap( m_covers.at( m_index ) );
}

void CoverFoundDialog::updatePixmap()
{
    m_label->setPixmap( m_covers.at( m_index ).scaled( m_label->size(),
                                                       Qt::KeepAspectRatio,
                                                       Qt::SmoothTransformation) );
}

void CoverFoundDialog::updateButtons()
{
    if( m_index < m_covers.length() - 1 )
        m_next->setEnabled( true );
    else
        m_next->setEnabled( false );

    if( m_index == 0 )
        m_prev->setEnabled( false );
    else
        m_prev->setEnabled( true );
}

void CoverFoundDialog::setTitle()
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
