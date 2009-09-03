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

#include "hintlineedit.h"

#include <kvbox.h>

#include <QFont>
#include <QLabel>

HintLineEdit::HintLineEdit( const QString &hint, const QString &text, QWidget *parent )
   : KLineEdit( text, 0 )
   , m_vbox( new KVBox( parent ) )
{
    init();
    m_hint->setText( hint );
}

HintLineEdit::HintLineEdit( const QString &text, QWidget *parent )
   : KLineEdit( text, 0 )
   , m_vbox( new KVBox( parent ) )
{
    init();
}

HintLineEdit::HintLineEdit( QWidget *parent )
   : KLineEdit( 0 )
   , m_vbox( new KVBox( parent ) )
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
    setParent( 0 );
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

#include "hintlineedit.moc"
