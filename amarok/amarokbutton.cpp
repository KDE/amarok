/***************************************************************************
                     amarokbutton.cpp  -  description
                        -------------------
begin                : Oct 31 2003
copyright            : (C) 2003 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokbutton.h"

#include <qlabel.h>
#include <qpixmap.h>
#include <qstring.h>

// CLASS AmarokButton ------------------------------------------------------------

AmarokButton::AmarokButton( QWidget *parent, QString activePixmap, QString inactivePixmap, bool toggleButton )
        : QLabel( parent )
{
    m_activePixmap = QPixmap( activePixmap );
    m_inactivePixmap = QPixmap( inactivePixmap );
    m_isToggleButton = toggleButton;

    setOn( false );
    m_clicked = false;

    setBackgroundMode( Qt::FixedPixmap );
    setBackgroundOrigin( QWidget::WindowOrigin );
}


AmarokButton::~AmarokButton()
{}


void AmarokButton::mousePressEvent( QMouseEvent * )
{
    m_clicked = true;

    if ( m_isToggleButton )
    {
        setPixmap( m_activePixmap );
    }
    else
    {
        setOn( true );
    }
}


void AmarokButton::mouseReleaseEvent( QMouseEvent *e )
{
    if ( m_clicked )
    {
        if ( rect().contains( e->pos() ) )
        {
            if ( m_isToggleButton )
            {
                if ( m_on )
                    setOn( false );
                else
                    setOn( true );

                emit toggled( m_on );
            }
            else
            {
                setOn( false );
                emit clicked();
            }
        }
        else
        {
            if ( m_isToggleButton )
            {
                if ( ! m_on )
                    setOn( false );
            }
            else
                setOn( false );
        }

        m_clicked = false;
    }
}


void AmarokButton::setOn( bool enable )
{
    if ( enable )
    {
        m_on = true;
        setPixmap( m_activePixmap );
    }
    else
    {
        m_on = false;
        setPixmap( m_inactivePixmap );
    }
}


bool AmarokButton::isOn()
{
    return m_on;
}


#include "amarokbutton.moc"
