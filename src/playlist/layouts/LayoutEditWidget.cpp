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
#include "DragStack.h"
#include "playlist/PlaylistDefines.h"

#include "Debug.h"

#include <KHBox>

#include <QCheckBox>
#include <QSpinBox>

using namespace Playlist;

LayoutEditWidget::LayoutEditWidget( QWidget *parent )
    : KVBox(parent)
{
    m_tokenFactory = new TokenWithLayoutFactory;
    m_dragstack = new DragStack( "application/x-amarok-tag-token", this );
    m_dragstack->setCustomTokenFactory( m_tokenFactory );
    connect ( m_dragstack, SIGNAL( focussed(QWidget*) ), this, SIGNAL( focussed(QWidget*) ) );
    
    m_showCoverCheckBox = new QCheckBox( i18n( "Show Cover" ) , this );
}


LayoutEditWidget::~LayoutEditWidget()
{
//     delete m_tokenFactory; m_tokenFactory = 0;
}

void LayoutEditWidget::readLayout( Playlist::LayoutItemConfig config )
{
    int rowCount = config.rows();

    m_showCoverCheckBox->setChecked( config.showCover() );

    m_dragstack->clear();

    for( int i = 0; i < rowCount; i++ )
    {
        //get the row config
        Playlist::LayoutItemConfigRow rowConfig = config.row( i );

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
            m_dragstack->insertToken( token, i, j );
            token->setWidth( element.size() * 100.0 );
        }

    }
}

Playlist::LayoutItemConfig LayoutEditWidget::config()
{

    LayoutItemConfig config;
    config.setShowCover( m_showCoverCheckBox->isChecked() );
    
    int noOfRows = m_dragstack->rows();

    for( int i = 0; i < noOfRows; i++ )
    {

        LayoutItemConfigRow currentRowConfig;

        QList<Token *> tokens = m_dragstack->drags( i );

        foreach( Token * token, tokens ) {
            if ( TokenWithLayout *twl = dynamic_cast<TokenWithLayout *>( token ) )
            {
                qreal width = 0.0;
                if ( twl->widthForced() && twl->width() > 0.01) {
                    width = twl->width();
                }
                currentRowConfig.addElement( LayoutItemConfigRowElement( twl->value(), width, twl->bold(), twl->italic(),
                                                                         twl->alignment(), twl->prefix(), twl->suffix() ) );
            }
        }

        config.addRow( currentRowConfig );

    }
    return config;
}


#include "LayoutEditWidget.moc"

