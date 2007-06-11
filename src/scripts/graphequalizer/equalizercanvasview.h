/*
 *   Copyright (C) 2005 by Ian Monroe <ian@monroe.nu>
 *   Released under GPL 2 or later, see COPYING
 */


#ifndef _EQUALIZERCANVASVIEW_H_
#define _EQUALIZERCANVASVIEW_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#include <qcanvas.h>
#include <qpen.h>
#include <qslider.h>

#endif

#include <kmainwindow.h>

class EqualizerCircle : public QCanvasEllipse
{

public:
    enum { RTTI = 1001 };
    
    
    EqualizerCircle(int x, int y, QCanvas *canvas, QCanvasLine* line1, QCanvasLine* line2, QPtrList<EqualizerCircle>* circleList ); 
    void setLocation(const QPoint &newLocation);
    int rtti() { return RTTI; }
private:
    enum WhichLine { LEFT = 1, RIGHT = 2 }; 
    void setLine(WhichLine lineNum, QCanvasLine* line);

    QCanvasLine *m_line1;
    QCanvasLine *m_line2;
    QPtrList<EqualizerCircle>* m_circleList;
};

/**
 * @short An equalizer widget for Amarok, using a line graph
 * @author Ian Monroe <ian@monroe.nu>
 */
class EqualizerCanvasView : public QCanvasView
{
    Q_OBJECT
public:
    EqualizerCanvasView(QWidget *parent, const char *name);
    void init();
    void contentsMousePressEvent(QMouseEvent *event);
    void contentsMouseDoubleClickEvent(QMouseEvent *event);
    void contentsMouseMoveEvent(QMouseEvent *event);
    void contentsMouseReleaseEvent(QMouseEvent *event);
    QValueList<int> currentSettings();
signals:
    void eqChanged();
private:
    int getY(int xCoord);
    QCanvasLine* makeLine(QPoint startPoint, QPoint endPoint);
    QPen m_pen;
    QCanvasItem* m_selectedItem;
    QPtrList<EqualizerCircle>* m_circleList;
};

class CallAmarok : public QObject
{
    Q_OBJECT
public:
    CallAmarok(QObject* parent, const char *name,
        EqualizerCanvasView* canvasView, QSlider* preampSlider) 
    : QObject(parent, name) 
    {
        m_canvasView = canvasView;
        m_preampSlider = preampSlider;
    }
public slots:
    void updateEq();
private:
    QSlider* m_preampSlider;
    EqualizerCanvasView* m_canvasView;
};

#endif // _EQUALIZERCANVASVIEW_H_
