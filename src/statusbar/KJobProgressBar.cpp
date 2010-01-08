/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "KJobProgressBar.h"

KJobProgressBar::KJobProgressBar( QWidget *parent, KJob * job )
        : ProgressBar( parent )
{
    connect( job, SIGNAL( percent( KJob*, unsigned long ) ),  SLOT( updateJobStatus( KJob *, unsigned long ) ) );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( delayedDone() ) );
}


KJobProgressBar::~KJobProgressBar()
{
}

void KJobProgressBar::updateJobStatus( KJob * job, unsigned long value )
{
    Q_UNUSED( job );
    setValue( value );
    emit( percentageChanged( percentage() ) );
}

#include "KJobProgressBar.moc"
