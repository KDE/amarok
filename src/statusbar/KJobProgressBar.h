/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef KJOBPROGRESSBAR_H
#define KJOBPROGRESSBAR_H

#include "statusbar/ProgressBar.h"

#include <KJob>

/**
A specialized progress bar that takes a KJob in the constructor and keeps itself updated

	@author
*/
class KJobProgressBar : public ProgressBarNG
{
    Q_OBJECT
public:
    KJobProgressBar( QWidget *parent, KJob * job );

    ~KJobProgressBar();

private slots:

    void updateJobStatus( KJob*, unsigned long );

};

#endif
