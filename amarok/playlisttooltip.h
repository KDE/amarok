/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  playlisttooltip.h  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#include <qtooltip.h>

class MetaBundle;
class QWidget;

class PlaylistToolTip : public QToolTip
{
    public:
        PlaylistToolTip( QWidget * parent );
        static void add( QWidget * widget, const MetaBundle & tags );

};
