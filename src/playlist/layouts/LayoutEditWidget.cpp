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
 
#include "LayoutEditWidget.h"
#include "playlist/PlaylistDefines.h"

#include "Debug.h"

#include <KHBox>

#include <QCheckBox>
#include <QSpinBox>

using namespace Playlist;

LayoutEditWidget::LayoutEditWidget( QWidget *parent )
    : KVBox(parent)
{
    m_rowsBox = new KVBox( this );

    KHBox * bottomBox = new KHBox( this );

    m_showCoverCheckBox = new QCheckBox( i18n( "Show Cover" ) , bottomBox );
    m_noOfRowsSpinBox = new QSpinBox( bottomBox );

    m_noOfRowsSpinBox->setPrefix( i18n( "No. of Rows: " ) );

    m_noOfRowsSpinBox->setRange( 0, 5 );
    m_noOfRowsSpinBox->setValue( 1 );
    numberOfRowsChanged( 1 );

    connect( m_noOfRowsSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( numberOfRowsChanged( int ) ) );

}


LayoutEditWidget::~LayoutEditWidget()
{
}

void LayoutEditWidget::numberOfRowsChanged( int noOfRows )
{
    int currentNoOfRows = m_rowMap.count();
    
    if ( noOfRows > currentNoOfRows )
    {
        //we need to add rows...
        int rowsToAdd = noOfRows - currentNoOfRows;

        for( int i = currentNoOfRows; i < rowsToAdd + currentNoOfRows; i++ )
        {
            FilenameLayoutWidget * layoutWidget = new FilenameLayoutWidget( m_rowsBox );
            layoutWidget->setCustomTokenFactory( new TokenWithLayoutFactory() );
            m_rowMap.insert( i, layoutWidget );
        }
        
    }
    else if ( noOfRows < currentNoOfRows )
    {
        //we need to remove rows. We jsut drop the bottom ones for now
        for( int i = noOfRows; i < currentNoOfRows; i++ )
        {
            FilenameLayoutWidget * layoutWidget = m_rowMap.take( i );
            delete layoutWidget;
        }
    }
}

void LayoutEditWidget::readLayout( Playlist::LayoutItemConfig config )
{
    int rowCount = config.rows();

    //clear existing rows (if any)
    numberOfRowsChanged( 0 );
    
    //create the needed rows
    numberOfRowsChanged( rowCount );
    m_noOfRowsSpinBox->setValue( rowCount );

    m_showCoverCheckBox->setChecked( config.showCover() );
    
    for( int i = 0; i < rowCount; i++ )
    {
        //get the row config
        Playlist::LayoutItemConfigRow rowConfig = config.row( i );
        
        //get the row layout
        FilenameLayoutWidget * currentRow = m_rowMap.value( i );

        int elementCount = rowConfig.count();

        //FIXME! for now, each element get the same size. This needs extensions to the token stuff
        //qreal size = 1.0 / (qreal) elementCount;
        
        for( int j = 0; j < elementCount; j++ )
        {
            Playlist::LayoutItemConfigRowElement element = rowConfig.element( j );
            TokenWithLayout *token =  new TokenWithLayout( columnNames[element.value()], iconNames[element.value()], element.value() );
            token->setBold( element.bold() );
            token->setItalic( element.italic() );
            token->setAlignment( element.alignment() );
            currentRow->addToken( token );
            token->setSize( element.size() * 100.0 );
        }

    }
}

Playlist::LayoutItemConfig LayoutEditWidget::config()
{

    LayoutItemConfig config;
    config.setShowCover( m_showCoverCheckBox->isChecked() );
    
    int noOfRows = m_rowMap.count();

    for( int i = 0; i < noOfRows; i++ )
    {

        LayoutItemConfigRow currentRowConfig;

        FilenameLayoutWidget * currentLayoutWidget = m_rowMap.value( i );
        QList<Token *> tokens = currentLayoutWidget->currentTokenLayout();

        foreach( Token * token, tokens ) {
            TokenWithLayout *twl = dynamic_cast<TokenWithLayout *>( token );

            bool bold = false;
            bool italic = false;
            Qt::Alignment alignment = Qt::AlignCenter;
            qreal size = 0.0;
            if ( twl )
            {
                bold = twl->bold();
                italic = twl->italic();
                alignment = twl->alignment();

                if ( twl->size() > 0.01 ) {
                    size = twl->size();
                }
            }

            currentRowConfig.addElement( LayoutItemConfigRowElement( token->value(), size, bold, italic, alignment ) );
        }

        config.addRow( currentRowConfig );

    }
    return config;
}


#include "LayoutEditWidget.moc"

