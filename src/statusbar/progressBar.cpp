/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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

#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include "progressBar.h"
#include <qlabel.h>


namespace KDE {


ProgressBar::ProgressBar( QWidget *parent, QLabel *label )
        : QProgressBar( parent )
        , m_label( label )
        , m_done( false )
{
    m_label->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    m_abort = new KPushButton( KStdGuiItem::cancel(), parent );
    m_abort->hide();
    m_abort->setText( i18n("Abort") );
    m_label->show();
    show();
}

ProgressBar::~ProgressBar()
{
    //we have to manage the label (which is no child() of us)
    delete m_label;
    delete m_abort;
}

ProgressBar&
ProgressBar::setDescription( const QString &text )
{
    m_description = text;
    m_label->setText( text );

    return *this;
}

ProgressBar&
ProgressBar::setStatus( const QString &text )
{
    QString s = m_description;
    s += " [";
    s += text;
    s += ']';

    m_label->setText( s );
    parentWidget()->adjustSize();

    return *this;
}

ProgressBar&
ProgressBar::setAbortSlot( QObject *receiver, const char *slot )
{
    connect( m_abort, SIGNAL(clicked()), receiver, slot );
    connect( m_abort, SIGNAL(clicked()), m_abort, SLOT(hide()) );
    m_abort->show();

    parentWidget()->adjustSize();

    return *this;
}

void
ProgressBar::setDone()
{
    m_done = true;
    m_abort->setEnabled( false );
    setStatus( i18n("Done") );
}

}
