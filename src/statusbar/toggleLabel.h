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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/** WARNING this is not meant for use outside this unit! */

#ifndef AMAROK_TOGGLELABEL_H
#define AMAROK_TOGGLELABEL_H

#include <kactionclasses.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <qiconset.h>
#include <qlabel.h>
#include <qtooltip.h>


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
    {
        connect( this,   SIGNAL(toggled( bool )), action, SLOT(setChecked( bool )) );
        connect( action, SIGNAL(toggled( bool )), this,   SLOT(setChecked( bool )) );

        setChecked( isChecked() );
        setToolTip();
    }

    inline bool isChecked() const { return m_action->isChecked(); }

protected:
    void mousePressEvent( QMouseEvent* )
    {
        if( KGlobalSettings::singleClick() )
            mouseDoubleClickEvent( 0 );
    }

    void mouseDoubleClickEvent( QMouseEvent* )
    {
        const bool b = !isChecked();

        setChecked( b );
        emit toggled( b );
    }

public slots:
    void setChecked( bool on )
    {
        setPixmap( m_action->iconSet().pixmap( QIconSet::Small, on ? QIconSet::Normal : QIconSet::Disabled ) );
        setToolTip();
    }

private:
    void setToolTip()
    {
        QString tip = "<qt><img src='%1' style='margin:auto'><br>&nbsp;";
        tip += m_action->isChecked() ? i18n("%2: on") : i18n("%2: off");
        tip += "&nbsp;";
        const QString path = KGlobal::iconLoader()->iconPath( m_action->icon(), -KIcon::SizeHuge );

        QToolTip::remove( this );
        QToolTip::add( this, tip.arg( path ).arg( m_action->text().remove('&') ) );
    }

};

#endif
