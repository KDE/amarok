/* 
   Copyright (C) 2008 Jeff Mitchell <mitchell@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef POPUPDROPPER_H
#define POPUPDROPPER_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimeLine>

class QGraphicsSvgItem;
class QGraphicsItem;
class QSvgRenderer;
class QWidget;
class PopupDropper;
class PopupDropperItem;
class PopupDropperPrivate;
class PopupDropperView;
class PopupDropperViewPrivate;

class PopupDropper : public QObject
{
    Q_OBJECT

    Q_PROPERTY( Fading fading READ fading WRITE setFading )
    Q_PROPERTY( bool standalone READ standalone )
    Q_PROPERTY( bool quitOnDragLeave READ quitOnDragLeave WRITE setQuitOnDragLeave )
    Q_PROPERTY( QColor windowColor READ windowColor WRITE setWindowColor )
    Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
    Q_PROPERTY( QString svgFile READ svgFile WRITE setSvgFile )
    Q_PROPERTY( QSvgRenderer* svgRenderer READ svgRenderer WRITE setSvgRenderer )
    Q_PROPERTY( int totalItems READ totalItems WRITE setTotalItems )
    Q_PROPERTY( const QTimeLine* fadeTimer READ fadeTimer )

public:    
    enum Fading { NoFade, FadeIn, FadeOut, FadeInOut };
    Q_ENUMS( Fading )
    enum HideReason { BackgroundChange, DragLeave, Unknown };

    PopupDropper( QWidget *parent, bool standalone = false );
    ~PopupDropper();

    bool isValid() const;
    bool standalone() const;
    
    void setWindowTitle( const QString &title );
    
    void show();
    void hide( PopupDropper::HideReason = PopupDropper::Unknown );
    bool isHidden() const;
    
    bool quitOnDragLeave() const;
    void setQuitOnDragLeave( bool quit );
    
    int fadeInTime() const;
    void setFadeInTime( const int msecs );    
    int fadeOutTime() const;
    void setFadeOutTime( const int msecs );
    PopupDropper::Fading fading() const;
    void setFading( PopupDropper::Fading fade );
    const QTimeLine* fadeTimer() const;
    
    void forceUpdate();
    void textUpdated();
    
    QColor windowColor() const;
    void setWindowColor( const QColor &window );
    QColor textColor() const;
    void setTextColor( const QColor &text );
    void setColors( const QColor &window, const QColor &text );
    void setPalette( const QColor &window, const QColor &text );
    
    QString svgFile() const;
    void setSvgFile( const QString &file );
    QSvgRenderer* svgRenderer();
    void setSvgRenderer( QSvgRenderer *renderer );

    int totalItems() const;
    void setTotalItems( int items );
    void addItem( QGraphicsSvgItem *item, bool useSharedRenderer = true );

private:
    friend class PopupDropperPrivate;
    PopupDropperPrivate* const d;

    bool closeAtEndOfHide;
};

#endif //POPUPDROPPER_H
