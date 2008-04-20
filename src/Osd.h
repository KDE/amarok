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

#include "meta/Meta.h"

#include <QPixmap>
#include <QImage>
#include <QWidget> //baseclass


class OSDWidget : public QWidget
{
    Q_OBJECT

    public:
        enum Alignment { Left, Middle, Center, Right };

        explicit OSDWidget( QWidget *parent, const char *name = "osd" );

        /** resets the colours to defaults */
        void unsetColors();

      public slots:
        /** calls setText() then show(), after setting image if needed */
        void show( const QString &text, QImage newImage = QImage::QImage() );
        void ratingChanged( const short rating );
        void ratingChanged( const QString& path, int rating );
        void volChanged( unsigned char volume );

        /** reimplemented, shows the OSD */
        virtual void show();

        /**
         * For the sake of simplicity, when these settings are
         * changed they do not take effect until the next time
         * the OSD is shown!
         *
         * To force an update call show();
         */
        void setDuration( int ms ) { m_duration = ms; }
        void setTextColor( const QColor &color )
        {
            QPalette palette = this->palette();
            palette.setColor( QPalette::Active, QPalette::WindowText, color );
            setPalette(palette);
        }
        void setBackgroundColor( const QColor &color )
        {
            QPalette palette = this->palette();
            palette.setColor( QPalette::Active, QPalette::Window, color );
            setPalette(palette);
        }
        void setOffset( int y ) { m_y = y; }
        void setAlignment( Alignment alignment ) { m_alignment = alignment; }
        void setImage( const QImage &image ) { m_cover = image; }
        void setScreen( int screen );
        void setText( const QString &text ) { m_text = text; }
        void setDrawShadow( const bool b ) { m_drawShadow = b; }
        void setTranslucent( const bool b ) { setWindowOpacity( b ? TRANSLUCENCY : 1.0 ); }
        void setRating( const short rating ) { if ( isEnabled() ) m_rating = rating; }

    protected:
        /** determine new size and position */
        QRect determineMetrics( const uint marginMetric );

        /** reimplemented */
        virtual void paintEvent( QPaintEvent* );
        virtual void mousePressEvent( QMouseEvent* );
        virtual bool event( QEvent* );

        bool useMoodbar( void );

        /** distance from screen edge */
        static const int MARGIN = 15;
        static const double TRANSLUCENCY = 0.7;

        uint        m_m;
        QSize       m_size;
        int         m_duration;
        QTimer     *m_timer;
        Alignment   m_alignment;
        int         m_screen;
        uint        m_y;
        bool        m_drawShadow;
        short       m_rating;
        unsigned char m_newvolume;
        bool        m_volume;
        QString     m_text;
        QImage      m_cover;
        QPixmap     m_scaledCover;
};



class OSDPreviewWidget : public OSDWidget
{
    Q_OBJECT

public:
    OSDPreviewWidget( QWidget *parent );

    int screen()    { return m_screen; }
    int alignment() { return m_alignment; }
    int y()         { return m_y; }

public slots:
    void setTextColor( const QColor &color ) { OSDWidget::setTextColor( color ); doUpdate(); }
    void setBackgroundColor( const QColor &color ) { OSDWidget::setBackgroundColor( color ); doUpdate(); }
    void setDrawShadow( bool b ) { OSDWidget::setDrawShadow( b ); doUpdate(); }
    void setFont( const QFont &font ) { OSDWidget::setFont( font ); doUpdate(); }
    void setScreen( int screen ) { OSDWidget::setScreen( screen ); doUpdate(); }
    void setTranslucent( bool translucent ) { OSDWidget::setTranslucent( translucent ); doUpdate(); }
    void setUseCustomColors( const bool use, const QColor &fg, const QColor &bg )
    {
        if( use ) {
            OSDWidget::setTextColor( fg );
            OSDWidget::setBackgroundColor( bg );
        } else
            unsetColors();
        doUpdate();
    }

private:
    inline void doUpdate() { if( !isHidden() ) show(); }

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



namespace Amarok
{
    class OSD : public OSDWidget
    {
        Q_OBJECT

    public:
        static OSD *instance()
        {
            static OSD *s_instance = new OSD;
            return s_instance;
        }

        void applySettings();
        virtual void show( Meta::TrackPtr track );

    public slots:
        /**
         * When user pushs global shortcut or uses DCOP OSD is toggle
         * even if it is disabled()
         */
        void forceToggleOSD();

    private:
        OSD();

    private slots:
        void slotCoverChanged( const QString &artist, const QString &album );
        void slotImageChanged( const QString &remoteURL );
    };
}

#endif /*AMAROK_OSD_H*/
