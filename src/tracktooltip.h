/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  tracktooltip.h  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#ifndef TRACKTOOLTIP_H
#define TRACKTOOLTIP_H

#include <qobject.h>
#include <qptrlist.h>
#include <metabundle.h>

class QWidget;

class TrackToolTip: public QObject
{
    Q_OBJECT

    public:
    TrackToolTip();
    static TrackToolTip* instance();

    void addToWidget( QWidget *widget );
    void removeFromWidget( QWidget *widget );

    void setTrack( const MetaBundle &tags, bool force = false );
    void setPos( int pos );
    void clear();

    private slots:
    void slotCoverChanged( const QString &artist, const QString &album );
    void slotColumnsChanged();

    private:
    QString tooltip();
    void updateWidgets();

    static TrackToolTip *s_instance;
    QPtrList<QWidget> m_widgets;
    MetaBundle m_tags;
    int        m_pos;
    QString    m_cover;
    QString    m_tooltip;
    bool       m_haspos;
};

#endif
