/***************************************************************************
                              likebackbar.cpp
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

#include <QResizeEvent>

#include <KApplication>
#include <KStandardDirs>

#include "likeback.h"
#include "likebackbar.h"


// Constructor
LikeBackBar::LikeBackBar( LikeBack *likeBack )
: QWidget( 0 )
, Ui::LikeBackBar()
, connected_( false )
, m_likeBack( likeBack )
{
  // Set up the user interface
  setupUi( this );
  resize( sizeHint() );
  setObjectName( "LikeBackBar" );

  // Set the button icons
  m_likeButton   ->setIcon( QIcon( KStandardDirs::locate( "data", "amarok/images/likeback_like.png"    ) ) );
  m_dislikeButton->setIcon( QIcon( KStandardDirs::locate( "data", "amarok/images/likeback_dislike.png" ) ) );
  m_bugButton    ->setIcon( QIcon( KStandardDirs::locate( "data", "amarok/images/likeback_bug.png"     ) ) );
  m_featureButton->setIcon( QIcon( KStandardDirs::locate( "data", "amarok/images/likeback_feature.png" ) ) );

  // Show buttons for the enabled types of feedback only
  LikeBack::Button buttons = likeBack->buttons();
  m_likeButton   ->setShown( buttons & LikeBack::Like    );
  m_dislikeButton->setShown( buttons & LikeBack::Dislike );
  m_bugButton    ->setShown( buttons & LikeBack::Bug     );
  m_featureButton->setShown( buttons & LikeBack::Feature );

#ifdef DEBUG_LIKEBACK
  kDebug() << "CREATED.";
#endif
}



// Destructor
LikeBackBar::~LikeBackBar()
{
#ifdef DEBUG_LIKEBACK
  kDebug() << "DESTROYED.";
#endif
}



// The Bug button has been clicked
void LikeBackBar::bugClicked()
{
  m_likeBack->execCommentDialog( LikeBack::Bug );
}



// Move the bar to the new active window
void LikeBackBar::changeWindow( QWidget *oldWidget, QWidget *newWidget )
{
  QWidget *oldWindow = ( oldWidget ? oldWidget->window() : 0 );
  QWidget *newWindow = ( newWidget ? newWidget->window() : 0 );

#ifdef DEBUG_LIKEBACK
  kDebug() << "Focus change:" << oldWindow << "->" << newWindow;
#endif

  if(  oldWindow == newWindow
  || ( oldWindow == 0 && newWindow == 0 ) )
  {
#ifdef DEBUG_LIKEBACK
    kDebug() << "Invalid/unchanged windows.";
#endif
    return;
  }

  // Do not detach if the old window is null, a popup or tool or whatever
  if(  oldWindow != 0
  && ( oldWindow->windowType() == Qt::Window
  ||   oldWindow->windowType() == Qt::Dialog ) )
  {
#ifdef DEBUG_LIKEBACK
    kDebug() << "Removing from old window:" << oldWindow;
#endif
    oldWindow->removeEventFilter( this );
    // Reparenting allows the bar to not be destroyed if the window which
    // has lost focus is being destroyed
    setParent( 0 );
    hide();
  }

  // Do not perform the switch if the new window is null, a popup or tool etc,
  // or if it's the send feedback window
  if(  newWindow != 0
  &&   newWindow->objectName() != "LikeBackFeedBack"
  && ( newWindow->windowType() == Qt::Window
  ||   newWindow->windowType() == Qt::Dialog ) )
  {
#ifdef DEBUG_LIKEBACK
    kDebug() << "Adding to new window:" << newWindow;
#endif
    setParent( newWindow );
    newWindow->installEventFilter( this );
    eventFilter( newWindow, new QResizeEvent( newWindow->size(), QSize() ) );
    show();
  }
}



// The Dislike button has been clicked
void LikeBackBar::dislikeClicked()
{
  m_likeBack->execCommentDialog( LikeBack::Dislike );
}



// Place the bar on the correct corner of the window
bool LikeBackBar::eventFilter(QObject *obj, QEvent *event)
{
  if( obj != parent() )
  {
#ifdef DEBUG_LIKEBACK
    kDebug() << "Incorrect event source";
#endif
    return false;
  }

  if( event->type() != QEvent::Resize )
  {
    return false;
  }

  // No need to move the feedback bar if the user has a RTL language.
  if( layoutDirection() == Qt::RightToLeft )
  {
    return false;
  }

  QResizeEvent *resizeEvent = static_cast<QResizeEvent*>( event );

#ifdef DEBUG_LIKEBACK
  kDebug() << "Resize event:" << resizeEvent->oldSize() << "->" << resizeEvent->size() << "my size:" << size();
#endif

  // Move the feedback bar to the top right corner of the window
  move( resizeEvent->size().width() - width(), 0 );
  return false;
}



// The Feature button has been clicked
void LikeBackBar::featureClicked()
{
  m_likeBack->execCommentDialog( LikeBack::Feature );
}



// The Like button has been clicked
void LikeBackBar::likeClicked()
{
  m_likeBack->execCommentDialog( LikeBack::Like );
}



// Show or hide the bar
void LikeBackBar::setBarVisible( bool visible )
{
  if( visible && ! isVisible() )
  {
#ifdef DEBUG_LIKEBACK
    kDebug() << "Setting visible, connected?" << connected_;
#endif

    // Avoid duplicated connections
    if( ! connected_ )
    {
      connect( kapp, SIGNAL( focusChanged(QWidget*,QWidget*) ),
               this, SLOT  ( changeWindow(QWidget*,QWidget*) ) );
      connected_ = true;
    }

    changeWindow( 0, kapp->activeWindow() );
  }
  else if( ! visible && isVisible() )
  {
#ifdef DEBUG_LIKEBACK
    kDebug() << "Setting hidden, connected?" << connected_;
#endif
    hide();

    if( connected_ )
    {
      disconnect( kapp, SIGNAL( focusChanged(QWidget*,QWidget*) ),
                  this, SLOT  ( changeWindow(QWidget*,QWidget*) ) );
      connected_ = false;
    }

    if( parent() )
    {
      parent()->removeEventFilter( this );
      setParent( 0 );
    }
  }
#ifdef DEBUG_LIKEBACK
  else
  {
    kDebug() << "Not changing status, connected?" << connected_;
  }
#endif
}



#include "likebackbar.moc"
