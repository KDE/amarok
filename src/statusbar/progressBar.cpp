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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "debug.h"
#include "progressBar.h"
#include "statusbar.h"

#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

#include <qlabel.h>


namespace KDE {


ProgressBar::ProgressBar( QWidget *parent, QLabel *label )
        : QProgressBar( parent )
        , m_label( label )
        , m_done( false )
{
    DEBUG_FUNC_INFO

    m_label->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    m_abort = new KPushButton( KStdGuiItem::cancel(), parent );
    m_abort->hide();
    m_abort->setText( i18n("Abort") );
    m_label->show();
    show();
}

ProgressBar::~ProgressBar()
{
    DEBUG_FUNC_INFO
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
    connect( m_abort, SIGNAL(clicked()), this, SLOT(hide()) );
    m_abort->show();

    parentWidget()->adjustSize();

    return *this;
}

ProgressBar&
ProgressBar::setProgressSignal( QObject *sender, const char *signal )
{
    setTotalSteps( 100 );
    connect( sender, signal, Amarok::StatusBar::instance(), SLOT( setProgress ( const QObject*, int ) ) );
    return *this;
}

void
ProgressBar::setDone()
{
    if( !m_done ) {
        m_done = true;
        m_abort->setEnabled( false );
        setStatus( i18n("Done") );
    }
    else
        // then we we're aborted
        setStatus( i18n("Aborted") );
}

void
ProgressBar::hide()
{
    //NOTE naughty

    m_done = true;
    m_abort->setEnabled( false );
    setStatus( i18n("Aborting...") );
}

}
