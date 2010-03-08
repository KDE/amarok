/***************************************************************************
                              likeback_p.h
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

#ifndef LIKEBACK_PRIVATE_H
#define LIKEBACK_PRIVATE_H

#include <QTimer>


class QButtonGroup;

class Kaction;



class LikeBackPrivate
{
  public:
                          LikeBackPrivate();
                         ~LikeBackPrivate();

  LikeBackBar             *bar;
  KConfigGroup             config;
  const KAboutData        *aboutData;
  LikeBack::Button         buttons;
  QString                  hostName;
  QString                  remotePath;
  quint16                  hostPort;
  QStringList              acceptedLocales;
  LikeBack::WindowListing  windowListing;
  bool                     showBarByDefault;
  bool                     showBar;
  int                      disabledCount;
  QString                  fetchedEmail;
  KAction                 *sendAction;
  KToggleAction           *showBarAction;
};



// Constructor
LikeBackPrivate::LikeBackPrivate()
 : bar(0)
 , aboutData(0)
 , buttons(LikeBack::DefaultButtons)
 , hostName()
 , remotePath()
 , hostPort(80)
 , acceptedLocales()
 , windowListing(LikeBack::NoListing)
 , showBar(false)
 , disabledCount(0)
 , fetchedEmail()
 , sendAction(0)
 , showBarAction(0)
{
}



// Destructor
LikeBackPrivate::~LikeBackPrivate()
{
  delete bar;
  delete sendAction;
  delete showBarAction;

  aboutData = 0;
}


#endif // LIKEBACK_PRIVATE_H
