/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/** WARNING this is not meant for use outside this unit! */

#ifndef AMAROK_SELECTLABEL_H
#define AMAROK_SELECTLABEL_H

#include "actionclasses.h"
#include "overlayWidget.h"
#include "popupMessage.h"

#include <kglobalsettings.h>
#include <kiconloader.h>
#include <qiconset.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtooltip.h>

class SelectLabel : public QLabel
{
    Q_OBJECT

    Amarok::SelectAction const*const m_action;

    signals:
        void activated( int );

    public:
        SelectLabel( Amarok::SelectAction const*const action, QWidget *parent )
                : QLabel( parent )
                , m_action( action )
                , m_tooltip( 0 )
                , m_tooltipShowing( false )
                , m_tooltipHidden( false )
        {
            connect( this,   SIGNAL( activated( int ) ), action, SLOT( setCurrentItem( int ) ) );
            connect( action, SIGNAL( activated( int ) ), this,   SLOT( setCurrentItem( int ) ) );
            connect( action, SIGNAL( enabled( bool )  ), this,   SLOT( setEnabled( bool )    ) );

            setCurrentItem( currentItem() );
        }

        inline int currentItem() const { return m_action->currentItem(); }
        inline bool isEnabled()  const { return m_action->isEnabled();   }

    protected:
        void mousePressEvent( QMouseEvent* )
        {
            bool shown = m_tooltipShowing;
            hideToolTip();
            int n = currentItem();
            do //TODO doesn't handle all of them being disabled, but we don't do that anyways.
            {
                n = ( uint( n ) == m_action->items().count() - 1 ) ? 0 : n + 1;
            } while ( !m_action->popupMenu()->isItemEnabled( n ) );
            if( isEnabled() )
            {
                setCurrentItem( n );
                emit activated( n );
                if( shown )
                {
                    m_tooltipHidden = false;
                    showToolTip();
                }
            }
        }

        void enterEvent( QEvent* )
        {
            //Show the tooltip after 1/2 second
            m_tooltipHidden = false;
            QTimer::singleShot( 500, this, SLOT(aboutToShow()) );
        }

        void leaveEvent( QEvent* )
        {
            hideToolTip();
        }

    public slots:
        void setCurrentItem( int )
        {
            if( isEnabled() && !m_action->currentIcon().isNull() )
                setPixmap( SmallIcon( m_action->currentIcon() ) );
        }

        void setEnabled( bool /*on*/ )
        {
            if( !m_action->currentIcon().isNull() )
                setPixmap( SmallIconSet( m_action->currentIcon() ).pixmap( QIconSet::Small, QIconSet::Disabled ) );
        }

    private slots:
        void aboutToShow()
        {
            if( hasMouse() && !m_tooltipHidden )
                showToolTip();
        }

    private:
        void showToolTip()
        {
            if( m_tooltipShowing )
                return;

            m_tooltipShowing = true;

            QString tip = i18n("%1: %2")
                          .arg( m_action->text().remove( '&' ) )
                          .arg( m_action->currentText().remove( '&' ) );

            if( !isEnabled() )
                tip += i18n("&nbsp;<br>&nbsp;<i>Disabled</i>");
            else if( AmarokConfig::favorTracks() &&
                     m_action == Amarok::actionCollection()->action( "random_mode" ) ) //hack?
            {
                KSelectAction *a = static_cast<KSelectAction*>( Amarok::actionCollection()->action( "favor_tracks" ) );
                tip += QString("<br><br>") + i18n("%1: %2")
                                             .arg( a->text().remove( '&' ), a->currentText().remove( '&' ) );
            }

            tip += "&nbsp;";

            m_tooltip = new KDE::PopupMessage( parentWidget()->parentWidget(), parentWidget(), 0 /*timeout*/ );
            m_tooltip->setShowCloseButton( false );
            m_tooltip->setShowCounter( false );
            m_tooltip->setMaskEffect( KDE::PopupMessage::Plain );
            m_tooltip->setText( tip );
            const QPixmap pix = KGlobal::iconLoader()
                                ->loadIconSet( m_action->currentIcon(), KIcon::Toolbar, KIcon::SizeHuge )
                                .pixmap( QIconSet::Large, m_action->isEnabled()
                                                          ? QIconSet::Normal
                                                          : QIconSet::Disabled );
            m_tooltip->setImage( pix );

            m_tooltip->reposition();
            m_tooltip->display();
        }

        void hideToolTip()
        {
            m_tooltipHidden = true;
            if( m_tooltipShowing )
            {
                m_tooltip->close();
                m_tooltipShowing = false;
            }
        }

        KDE::PopupMessage *m_tooltip;
        bool               m_tooltipShowing;
        bool               m_tooltipHidden;

};

#endif
