/*
 *   Copyright (C) 2005 by Ian Monroe <ian@monroe.nu>
 *   Released under GPL 2 or later, see COPYING
 */


#ifndef _EQUALIZERCANVASVIEW_H_
#define _EQUALIZERCANVASVIEW_H_

#include <q3canvas.h>
#include <QPen>
#include <QSlider>

#include <kmainwindow.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <Q3ValueList>
#include <Q3PtrList>

class EqualizerCircle : public Q3CanvasEllipse
{

public:
    enum { RTTI = 1001 };
    
    
    EqualizerCircle(int x, int y, Q3Canvas *canvas, Q3CanvasLine* line1, Q3CanvasLine* line2, Q3PtrList<EqualizerCircle>* circleList ); 
    void setLocation(const QPoint &newLocation);
    int rtti() { return RTTI; }
private:
    enum WhichLine { LEFT = 1, RIGHT = 2 }; 
    void setLine(WhichLine lineNum, Q3CanvasLine* line);

    Q3CanvasLine *m_line1;
    Q3CanvasLine *m_line2;
    Q3PtrList<EqualizerCircle>* m_circleList;
};

/**
 * @short An equalizer widget for amaroK, using a line graph
 * @author Ian Monroe <ian@monroe.nu>
 */
class EqualizerCanvasView : public Q3CanvasView
{
    Q_OBJECT
public:
    EqualizerCanvasView(QWidget *parent, const char *name);
    void init();
    void contentsMousePressEvent(QMouseEvent *event);
    void contentsMouseDoubleClickEvent(QMouseEvent *event);
    void contentsMouseMoveEvent(QMouseEvent *event);
    void contentsMouseReleaseEvent(QMouseEvent *event);
    Q3ValueList<int> currentSettings();
signals:
    void eqChanged();
private:
    int getY(int xCoord);
    Q3CanvasLine* makeLine(QPoint startPoint, QPoint endPoint);
    QPen m_pen;
    Q3CanvasItem* m_selectedItem;
    Q3PtrList<EqualizerCircle>* m_circleList;
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
