/***************************************************************************
                                likebackbar.h
                             -------------------
    begin                : unknown
    imported to LB svn   : 3 june, 2009
    copyright            : (C) 2006 by Sebastien Laout
                           (C) 2008-2009 by Valerio Pilo, Sjors Gielen
    email                : sjors@kmess.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LIKEBACKBAR_H
#define LIKEBACKBAR_H

#include <QTimer>
#include <QWidget>

#include "likeback.h"

#include "ui_likebackbar.h"



class LikeBackBar : public QWidget, private Ui::LikeBackBar
{
  Q_OBJECT

  public:
    // Constructor
             LikeBackBar( LikeBack *likeBack );
    // Destructor
            ~LikeBackBar();
    // Show or hide the bar
    void     setBarVisible( bool visible );

  private slots:
    // The Bug button has been clicked
    void     bugClicked();
    // Move the bar to the new active window
    void     changeWindow( QWidget *oldWidget, QWidget *newWidget );
    // The Dislike button has been clicked
    void     dislikeClicked();
    // Place the bar on the correct corner of the window
    bool     eventFilter( QObject *obj, QEvent *event );
    // The Feature button has been clicked
    void     featureClicked();
    // The Like button has been clicked
    void     likeClicked();

  private:
    // Whether we're connected to the window focus signal or not
    bool     connected_;
    // The parent LikeBack instance
    LikeBack *m_likeBack;

};

#endif
