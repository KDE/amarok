/****************************************************************************************
 * Copyright (c) 2006 Sebastien Laout <slaout@linux62.org>                              *
 * Copyright (c) 2008,2009 Valerio Pilo <amroth@kmess.org>                              *
 * Copyright (c) 2008,2009 Sjors Gielen <sjors@kmess.org>                               *
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

#ifndef LIKEBACKBAR_H
#define LIKEBACKBAR_H

#include <QTimer>
#include <QWidget>

#include "LikeBack.h"

#include "ui_LikeBackBar.h"



class LikeBackBar : public QWidget, private Ui::LikeBackBar
{
    Q_OBJECT

public:
    // Constructor
    LikeBackBar( LikeBack *likeBack );
    // Destructor
    ~LikeBackBar();
    // Show or hide the bar
    void setBarVisible( bool visible );

private slots:
    // The Bug button has been clicked
    void bugClicked();
    // Move the bar to the new active window
    void changeWindow( QWidget *oldWidget, QWidget *newWidget );
    // The Dislike button has been clicked
    void dislikeClicked();
    // Place the bar on the correct corner of the window
    bool eventFilter( QObject *obj, QEvent *event );
    // The Feature button has been clicked
    void featureClicked();
    // The Like button has been clicked
    void likeClicked();

private:
    // Whether we're connected to the window focus signal or not
    bool m_connected;
    // The parent LikeBack instance
    LikeBack *m_likeBack;
};

#endif
