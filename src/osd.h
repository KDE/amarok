/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  osd.h   -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/


#ifndef OSD_H
#define OSD_H

#include <qpixmap.h>
#include <qwidget.h> //baseclass

class QFont;
class QString;
class QStringList;
class QTimer;
class MetaBundle;

class OSDWidget : public QWidget
{
    Q_OBJECT
      public:
        enum Position {
          TopLeft,
          TopRight,
          BottomLeft,
          BottomRight,
          Center
        };

        OSDWidget(const QString &appName, QWidget *parent = 0, const char *name = "osd");
        void setDuration(int ms);
        void setFont(QFont newfont);
        void setTextColor(QColor newcolor);
        void setBackgroundColor(QColor newColor);
        void setOffset(int x, int y);
        void setPosition(Position pos);
        void setScreen(uint screen);

      public slots:
        void showOSD(const QString&, bool preemptive=false );
        void removeOSD() { hide(); } //inlined as is convenience function

      protected slots:
        void minReached();

      protected:
        /* render text into osdBuffer */
        void renderOSDText(const QString &text);
        void paintEvent(QPaintEvent*);
        void mousePressEvent( QMouseEvent* );
        //void showEvent( QShowEvent* );
        void show();

        /* always call rePosition if the size of osdBuffer has changed */
        void rePosition();

        //called after most set*() calls to update the OSD
        void refresh();

        QString     m_appName;
        int         m_duration;
        QTimer      *timer;
        QTimer      *timerMin;
        QPixmap     osdBuffer;
        QStringList textBuffer;
        QString     m_currentText;

        QPoint m_offset;
        Position m_position;
        int m_screen;

        bool m_dirty; //if dirty we will be re-rendered before we are shown
};



// do not pollute OSDWidget with this preview stuff
class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT
public:
    OSDPreviewWidget( const QString &appName );

signals:
    void positionChanged( int screen, OSDWidget::Position alignment, int XOffset, int YOffset );

protected:
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );

private:
    bool m_dragging;
    QPoint m_dragOffset;
};



namespace amaroK {

class OSD : public OSDWidget
{
Q_OBJECT
public:
    OSD() : OSDWidget( "amaroK" ) {}

public slots:
    void showTrack( const MetaBundle &bundle );
    void showTrack() { showOSD( m_text ); }

    //this function is for the showOSD global shortcut, it should always work //FIXME sucks
    void forceShowTrack() { bool b = isEnabled(); setEnabled( true ); showTrack(); setEnabled( b ); }

private:
    QString m_text;
};

}

#endif
