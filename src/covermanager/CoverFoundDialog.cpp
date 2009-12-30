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

#include <KLocale>
#include <KVBox>
#include <KPushButton>

CoverFoundDialog::CoverFoundDialog( QWidget *parent,
                                    const QList<QPixmap> &covers,
                                    const QString &productname ) : KDialog( parent )
{
    m_curCover = 0;
    m_covers.clear();
    m_covers = covers;
    this->setButtons( None );
    this->showButtonSeparator( false );
    KVBox *box = new KVBox( this );
    this->setMainWidget(box);
    box->setSpacing( 4 );

    m_labelPix  = new QLabel( box );
    m_labelName = new QLabel( box );
    m_buttons   = new KHBox( box );
    m_prev      = new KPushButton( KStandardGuiItem::back(), m_buttons );
    m_save      = new KPushButton( KStandardGuiItem::save(), m_buttons );
    m_cancel    = new KPushButton( KStandardGuiItem::cancel(), m_buttons );
    m_next      = new KPushButton( KStandardGuiItem::forward(), m_buttons );

    if( m_covers.length() == 1 )
        m_next->setEnabled( false );
    else
        m_next->setEnabled( true );

    m_prev->setEnabled( false );

    m_labelPix ->setMinimumHeight( 300 );
    m_labelPix ->setMinimumWidth( 300 );
    m_labelPix ->setAlignment( Qt::AlignHCenter );
    m_labelName->setAlignment( Qt::AlignHCenter );
    m_labelPix ->setPixmap( m_covers.at( m_curCover ) );
    m_labelPix ->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_labelName->setText( productname );

    m_save->setDefault( true );
    this->setTitle();

    connect( m_prev   ,SIGNAL(clicked()) ,SLOT(prevPix()) );
    connect( m_save   ,SIGNAL(clicked()) ,SLOT(accept())  );
    connect( m_cancel ,SIGNAL(clicked()) ,SLOT(reject())  );
    connect( m_next   ,SIGNAL(clicked()) ,SLOT(nextPix()) );
}

void CoverFoundDialog::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event )

    QSize scaledSize = m_labelPix->pixmap()->size();
    scaledSize.scale( m_labelPix->size(), Qt::KeepAspectRatio );

    if( !m_labelPix->pixmap() || scaledSize != m_labelPix->pixmap()->size() )
        updatePixmapSize();
}

void CoverFoundDialog::updatePixmapSize()
{
    m_labelPix->setPixmap( m_covers.at( m_curCover ).scaled( m_labelPix->size(),
                                                             Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation) );
}

void CoverFoundDialog::setTitle()
{
    QString caption = i18n( "Cover Found" );
    if( m_covers.size() > 1 )
    {
        const QString position = i18n( "%1/%2", m_curCover + 1, m_covers.size() );
        caption +=  ": " + position;
    }
    this->setCaption( caption );
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
    if( m_curCover < m_covers.length()-1 )
    {
        m_curCover++;
        m_labelPix ->setPixmap( m_covers.at( m_curCover ) );
        m_prev->setEnabled( true );
    }

    if( m_curCover >= m_covers.length()-1 )
        m_next->setEnabled( false );
    else
        m_next->setEnabled( true );

    this->setTitle();
    updatePixmapSize();
}

//SLOT
void CoverFoundDialog::prevPix()
{
    if( m_curCover > 0 )
    {
        m_curCover--;
        m_labelPix ->setPixmap( m_covers.at( m_curCover ) );
        m_next->setEnabled( true );
    }

    if( m_curCover == 0 )
        m_prev->setEnabled( false );
    else
        m_prev->setEnabled( true );

    this->setTitle();
    updatePixmapSize();
}

#include "CoverFoundDialog.moc"
