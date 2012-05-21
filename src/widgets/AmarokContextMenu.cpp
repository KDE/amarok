/****************************************************************************************
 * Copyright (c) 2012 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "AmarokContextMenu.h"

#include <QHelpEvent>
#include <QToolTip>

#include "core/support/Debug.h"

Amarok::ContextMenu::ContextMenu( QWidget *parent )
    : KMenu( parent )
{
}

bool
Amarok::ContextMenu::event( QEvent *e )
{
    switch( e->type() )
    {
        case QEvent::ToolTip:
        {
            //show action tooltip instead of widget tooltip
            QHelpEvent* he = dynamic_cast<QHelpEvent *>( e );
            QAction* act = actionAt( he->pos() );

            if( !act )
                return false;

            //QAction has default toolTip ~ text, don't show duplicate info
            // '&' used for accelerator shortcut key
            if( act->text().remove( '&' ) == act->toolTip() )
            {
                QToolTip::hideText();
                return true;
            }
            else
            {
                QToolTip::showText( he->globalPos(), act->toolTip(), this );
                return true;
            }
        }

        default: break;
    }

    return KMenu::event( e );
}

void
Amarok::ContextMenu::setAlternatives( QAction *action, QAction *alternative,
                                           Qt::Key trigger)
{
    m_alternativeActionMap.insertMulti( (int)trigger, QActionPair( action, alternative) );

    alternative->setVisible( false );
}

void
Amarok::ContextMenu::keyPressEvent( QKeyEvent *e )
{
    if( m_alternativeActionMap.keys().contains( e->key() ) )
    {
        QList<QActionPair> pairs =
                m_alternativeActionMap.values( e->key() );
        foreach( QActionPair alt, pairs )
        {
            alt.first->setVisible( false);
            alt.second->setVisible( true );
        }
    }
    KMenu::keyPressEvent( e );
}

void
Amarok::ContextMenu::keyReleaseEvent( QKeyEvent *e )
{
    if( m_alternativeActionMap.keys().contains( e->key() ) )
    {
        QList<QActionPair> pairs =
                m_alternativeActionMap.values( e->key() );
        foreach( QActionPair alt, pairs )
        {
            alt.first->setVisible( true );
            alt.second->setVisible( false );
        }
    }

    KMenu::keyPressEvent( e );
}
