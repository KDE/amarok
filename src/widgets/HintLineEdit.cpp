/****************************************************************************************
 * Copyright (c) 2006 Martin Aumueller <aumuell@reserv.at>                              *
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

#include "HintLineEdit.h"

#include "widgets/BoxWidget.h"

#include <QFont>
#include <QLabel>

HintLineEdit::HintLineEdit( const QString &hint, const QString &text, QWidget *parent )
   : QLineEdit( text, nullptr )
   , m_vbox( new BoxWidget( true, parent ) )
{
    init();
    m_hint->setText( hint );
}

HintLineEdit::HintLineEdit( const QString &text, QWidget *parent )
   : QLineEdit( text, nullptr )
   , m_vbox( new BoxWidget( true, parent ) )
{
    init();
}

HintLineEdit::HintLineEdit( QWidget *parent )
   : QLineEdit( nullptr )
   , m_vbox( new BoxWidget( true, parent ) )
{
    init();
}

void
HintLineEdit::init()
{
    setParent( m_vbox );
    show();
    m_hint = new QLabel( m_vbox );
    //m_hint->setBuddy( this );
    m_hint->setFocusPolicy( Qt::NoFocus );
    QFont font;
    font.setPointSize( font.pointSize() - 2);
    m_hint->setFont( font );
}

HintLineEdit::~HintLineEdit()
{
    setParent( nullptr );
    delete m_vbox;
}

void
HintLineEdit::setHint( const QString &hint )
{
    m_hint->setText( hint );
}

QObject *
HintLineEdit::parent()
{
    return m_vbox->parent();
}

