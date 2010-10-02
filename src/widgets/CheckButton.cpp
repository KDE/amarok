/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#include "CheckButton.h"

#include <QFontMetrics>

CheckButton::CheckButton( QWidget *parent )
           : QPushButton( parent )
{
    init();
}

CheckButton::CheckButton( const QString &text, QWidget *parent )
           : QPushButton( text, parent )
{
    init();
}

CheckButton::CheckButton( const QIcon &icon, const QString &text, QWidget *parent)
           : QPushButton( icon, text, parent )
{
    init();
}

CheckButton::~CheckButton()
{
    delete checkBox;
}

void
CheckButton::init()
{
    checkBox = new QCheckBox( this );

    QPalette palette = checkBox->palette();
    palette.setColor( QPalette::Button, Qt::white );
    checkBox->setPalette( palette );

    moveCheckBox();
}

void
CheckButton::moveCheckBox()
{
    QFontMetrics fm = fontMetrics();

    if( height() < checkBox->height() + 2 )
        resize( width(), checkBox->height() + 2 );

    if( width() < fm.width( text() ) + checkBox->width() + 10 )
        resize( fm.width( text() ) + checkBox->width() + 10, height() );

    checkBox->move( 1, ( height() - checkBox->height() ) / 2 );
}

void
CheckButton::setText( const QString &text )
{
    QPushButton::setText( text );
    moveCheckBox();
}

void
CheckButton::resizeEvent( QResizeEvent *event )
{
    QPushButton::resizeEvent( event );
    moveCheckBox();
}

bool
CheckButton::isChecked() const
{
    return checkBox->isChecked();
}

QString CheckButton::checkHint() const
{
    return checkBox->toolTip();
}

void
CheckButton::setCheckHint( const QString &text )
{
    checkBox->setToolTip( text );
}
