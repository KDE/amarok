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

#include "meta/Meta.h"

#include <QWidget>
#include <QSystemTrayIcon>

class QLabel;
class QWidget;

// class TrackToolTip: public QObject, public Amarok::ToolTipClient, public Meta::Observer
class TrackToolTip : public QWidget, public Meta::Observer
{
    Q_OBJECT

    public:
    TrackToolTip();
    static TrackToolTip* instance();

    void setTrack( const Meta::TrackPtr track, bool force = false );
    void setPos( int pos );
    void clear();
    void show( const QPoint &bottomRight );

    //Reimplemented from Meta::Observer
    virtual void metaDataChanged( Meta::Track *track ) { ; } //TODO: IMPLEMENT
    virtual void metaDataChanged( Meta::Album *album ) { ; } //TODO: Implement
    virtual void metaDataChanged( Meta::Artist *artist ) { ; } //TODO: Implement

    private:
    QString tooltip() const;
    void updateWidgets();

    static TrackToolTip *s_instance;
    QList<QWidget> m_widgets;
    Meta::TrackPtr m_track;
    int        m_pos;
    QString    m_tooltip;
    bool       m_haspos;
    QString    m_moodbarURL;
    QString m_title;

    QLabel *m_imageLabel;
    QPixmap m_image;
    QLabel *m_titleLabel;
    QLabel *m_otherInfoLabel;
};

#endif
