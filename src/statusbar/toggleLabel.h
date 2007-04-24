/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

/** WARNING this is not meant for use outside this unit! */

#ifndef AMAROK_TOGGLELABEL_H
#define AMAROK_TOGGLELABEL_H

#include "debug.h"
// #include "overlayWidget.h"
#include "popupMessage.h"

#include <ktoggleaction.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kpassivepopup.h>
#include <QIcon>
#include <QLabel>
#include <QStyle>
#include <QTimer>
#include <QToolTip>
//Added by qt3to4:
#include <QEvent>
#include <QMouseEvent>

class ToggleLabel : public QLabel
{
    Q_OBJECT

    KToggleAction const*const m_action;

    signals:
        void toggled( bool );

    public:
        ToggleLabel( KToggleAction const*const action, QWidget *parent )
                : QLabel( parent )
                , m_action( action )
                , m_tooltip( 0 )
                , m_tooltipShowing( false )
                , m_tooltipHidden( false )
        {
            connect( this,   SIGNAL(toggled( bool )), action, SLOT(setChecked( bool )) );
            connect( action, SIGNAL(toggled( bool )), this,   SLOT(setChecked( bool )) );
            connect( action, SIGNAL(enabled( bool )), this,   SLOT(setEnabled( bool )) );

            setChecked( isChecked() );
        }

        inline bool isChecked() const { return m_action->isChecked(); }
        inline bool isEnabled() const { return m_action->isEnabled(); }

    protected:
        void mousePressEvent( QMouseEvent* )
        {
            hideToolTip();
            const bool b = !isChecked();
            if( isEnabled() )
            {
                setChecked( b );
                emit toggled( b );
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
        void setChecked( bool on )
        {
            setPixmap( m_action->icon().pixmap( style()->pixelMetric(QStyle::PM_SmallIconSize), on ? QIcon::Normal : QIcon::Disabled ) );
        }

        void setEnabled( bool /*on*/ ) { }

    private slots:
        void aboutToShow()
        {
            if( testAttribute(Qt::WA_UnderMouse) && !m_tooltipHidden )
                showToolTip();
        }

    private:
        void showToolTip()
        {
            if( m_tooltipShowing )
                return;

            m_tooltipShowing = true;

            QString action = m_action->text().remove('&');
            QString tip = m_action->isChecked() ? i18n("%1: on", action) : i18n("%1: off", action);

            if( !isEnabled() )
                tip += i18n("&nbsp;<br>&nbsp;<i>Disabled</i>");

            tip += "&nbsp;";

            KHBox *khb = new KHBox( this );
            QLabel *lab = new QLabel( tip, khb );
            lab->setPixmap( m_action->icon().pixmap(QSize(22, 22) ) );

            KPassivePopup::message( QString(), tip, m_action->icon().pixmap(QSize(22,22)), this );

            m_tooltip = new KPassivePopup( parentWidget() );
            m_tooltip->setView( khb );
            m_tooltip->show( rect().topLeft() );
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

        KPassivePopup      *m_tooltip;
        bool               m_tooltipShowing;
        bool               m_tooltipHidden;
};


#endif
