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
 
#include "TokenWithLayout.h"

#include "Debug.h"

#include <KAction>
#include <KLocale>

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QMenu>


Token * TokenWithLayoutFactory::createToken(const QString &text, const QString &iconName, int value, QWidget *parent)
{
    return new TokenWithLayout( text, iconName, value, parent );
}

TokenWithLayout::TokenWithLayout( const QString &text, const QString &iconName, int value, QWidget *parent )
    : Token( text, iconName, value, parent  )
{
    m_alignment = Qt::AlignCenter;
    m_bold = false;
}


TokenWithLayout::~TokenWithLayout()
{
}

void TokenWithLayout::contextMenuEvent( QContextMenuEvent * event )
{

    DEBUG_BLOCK
    
    QMenu menu;
    
    menu.setTitle(   i18n( "Layout" ) );

    KAction *boldAction = new KAction( i18n( "Bold" ), &menu );
    boldAction->setCheckable( true );
    boldAction->setChecked( m_bold );

    KAction *alignLeftAction = new KAction( i18n( "Left" ), &menu );
    KAction *alignCenterAction = new KAction( i18n( "Center" ), &menu );
    KAction *alignRightAction = new KAction( i18n( "Right" ), &menu );
    alignLeftAction->setCheckable( true );
    alignCenterAction->setCheckable( true );
    alignRightAction->setCheckable( true );

    if ( m_alignment == Qt::AlignLeft )
        alignLeftAction->setChecked( true );
    else if ( m_alignment == Qt::AlignCenter )
        alignCenterAction->setChecked( true );
    else if ( m_alignment == Qt::AlignRight )
        alignRightAction->setChecked( true );

    QActionGroup *alignmentGroup = new QActionGroup( &menu );
    alignmentGroup->addAction( alignLeftAction );
    alignmentGroup->addAction( alignCenterAction );
    alignmentGroup->addAction( alignRightAction );


    menu.addAction( boldAction );
    menu.addSeparator()->setText( i18n( "Alignment" ) );
    menu.addAction( alignLeftAction );
    menu.addAction( alignCenterAction );
    menu.addAction( alignRightAction );



    QAction* a = menu.exec( mapToGlobal( event->pos() ) );
    
    if( a == alignLeftAction )
        setAlignment( Qt::AlignLeft );
    else if( a == alignCenterAction )
        setAlignment( Qt::AlignCenter );
    else if( a == alignRightAction )
        setAlignment( Qt::AlignRight );
    else if( a == boldAction )
        setBold( boldAction->isChecked() );
    
}

Qt::Alignment TokenWithLayout::alignment()
{
    return m_alignment;
}

void TokenWithLayout::setAlignment( Qt::Alignment alignment )
{
    m_alignment = alignment;
    m_label->setAlignment( alignment );
}

bool TokenWithLayout::bold()
{
    return m_bold;
}

void TokenWithLayout::setBold( bool bold )
{
    m_bold = bold;
    QFont font = m_label->font();
    font.setBold( bold );
    m_label->setFont( font );
}




