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
  email:     chris@chris.de
*/

#ifndef AMAROK_OSD_H
#define AMAROK_OSD_H

#include <qimage.h>
#include <qpixmap.h> //stack allocated
#include <qtimer.h>  //stack allocated
#include <qvaluelist.h>
#include <qwidget.h> //baseclass


class QStringList;
class MetaBundle;

class OSDWidget : public QWidget
{
    Q_OBJECT
      public:
        enum Alignment { Left, Middle, Center, Right };

        OSDWidget( const QString &appName, QWidget *parent = 0, const char *name = "osd" );

        void unsetColors();

        int screen()    { return m_screen; }
        int alignment() { return m_alignment; }
        int y()         { return m_y; }
        bool showCover() { return m_cover; }

      public slots:
        void show( const QString&, bool preemptive = false, bool useImage = false );
        void show();

        void setDuration( int ms );
        void setFont( const QFont &newfont );
        void setShadow( bool shadow );
        void setTextColor( const QColor &newcolor );
        void setBackgroundColor(const QColor &newColor );
        void setOffset( int x, int y );
        void setAlignment( Alignment );
        void setScreen( int screen );
        void setText( const QString &text ) { m_currentText = text; refresh(); }

      protected slots:
        void minReached();

      protected:
        /* render text into osdBuffer */
        void renderOSDText( const QString& );
        void mousePressEvent( QMouseEvent* );
        bool event( QEvent* );

        /* call to reposition a new OSD text or when position attributes change */
        void reposition( QSize newSize = QSize() );

        void loadImage( QString &location );
        QPixmap createGradient( QSize size );

        /* called after most set*() calls to update the OSD */
        void refresh();

        static const int MARGIN = 15;

        QString     m_appName;
        int         m_duration;
        QTimer      timer;
        QTimer      timerMin;
        QPixmap     osdBuffer;
        QStringList textBuffer;
        QValueList<QImage> imageBuffer;
        QString     m_currentText;
        QImage      m_image;
        bool        m_useImage;
        bool        m_shadow;
        bool        m_cover;

        Alignment   m_alignment;
        int         m_screen;
        uint        m_y;

        bool m_dirty; //if dirty we will be re-rendered before we are shown
};



// do not pollute OSDWidget with this preview stuff
class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT
public:
    OSDPreviewWidget( const QString &appName, QWidget *parent = 0, const char *name = "osdpreview" );

    static QPoint m_previewOffset;

signals:
    void positionChanged();

protected:
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );

private:
    bool   m_dragging;
    QPoint m_dragOffset;
};



namespace amaroK
{
    class OSD : public OSDWidget
    {
      Q_OBJECT
        public:
            static OSD *instance();

            void applySettings();
            void setImage( const MetaBundle &bundle );

        public slots:
            void showTrack( const MetaBundle &bundle );
            void showTrack() { show( m_text, false, true ); }

            //this function is for the showOSD global shortcut, it should always work //FIXME sucks
            void forceShowTrack() { bool b = isEnabled(); setEnabled( true ); showTrack(); setEnabled( b ); }

        private:
            OSD() : OSDWidget( "amaroK" ) {}

            QString m_text;
    };
}

#endif /*AMAROK_OSD_H*/
