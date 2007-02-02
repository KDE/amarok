/*
 *   Copyright (C) 2005 by Ian Monroe
 *   Released under GPL 2 or later, see COPYING
 */


#include "equalizercanvasview.h"

#include <QLabel>
#include <q3canvas.h>
#include <QPen>
//Added by qt3to4:
#include <Q3ValueList>
#include <QMouseEvent>
#include <Q3PtrList>
#include <Q3MemArray>

#include <dcopclient.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

EqualizerCanvasView::EqualizerCanvasView(QWidget *parent = 0, const char *name = 0)
    : Q3CanvasView(parent, name)
{
    m_pen.setWidth(5);
    m_circleList = new Q3PtrList<EqualizerCircle>();
}

//called after setCanvas
void
EqualizerCanvasView::init()
{

//     QCanvasLine* line = new QCanvasLine(this->canvas());
//     line->setPoints(0,100,400,100);
//     line->setPen(QPen(m_pen));
//     line->setBrush(QBrush(Qt::black));
    Q3CanvasLine* lineLeft = makeLine(QPoint(0,canvas()->height()/2)
                ,QPoint(canvas()->width()/2,canvas()->height()/2));
    Q3CanvasLine* lineRight= makeLine(QPoint(canvas()->width()/2,canvas()->height()/2)
                ,QPoint(canvas()->width(),canvas()->height()/2));
    EqualizerCircle* circleMid = new EqualizerCircle(canvas()->width()/2
              ,canvas()->height()/2
              ,canvas()
              ,lineLeft
              ,lineRight
              ,m_circleList);
    EqualizerCircle* circleLeft = new EqualizerCircle(0,100,this->canvas(),NULL,lineLeft,m_circleList);
    EqualizerCircle* circleRight =  new EqualizerCircle(400,100,this->canvas(),lineRight,NULL,m_circleList);
    circleLeft->show();
    circleRight->show();
    circleMid->show();
//    line->show();
    this->canvas()->update();
}

void
EqualizerCanvasView::contentsMousePressEvent(QMouseEvent *event)
{
    Q3CanvasItemList items = canvas()->collisions(event->pos());
    if (items.empty())
        m_selectedItem = 0;
    else
        m_selectedItem = *(items.begin());
}


void
EqualizerCanvasView::contentsMouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == LeftButton && m_selectedItem
            && m_selectedItem->rtti() == Q3CanvasLine::RTTI)
    {
        Q3CanvasLine* line = (Q3CanvasLine*) m_selectedItem;
        int y = getY(event->x());

        EqualizerCircle* circle = new EqualizerCircle(event->x(), y
              ,canvas()
              ,makeLine(line->startPoint(),QPoint(event->x(),event->y()))
              ,makeLine(QPoint(event->x(),event->y()),line->endPoint())
              ,m_circleList);
        //kDebug() << event->x() << 'x' << y << " cursor at " << event->x() << 'x' << event->y() << endl;
        circle->show();
        canvas()->update();
        m_selectedItem=0;
        delete line;
        canvas()->update();
    }
}
void
EqualizerCanvasView::contentsMouseMoveEvent(QMouseEvent *event)
{
    if ((event->state() & LeftButton) && m_selectedItem ) {
      //  kDebug() << "dragging " << m_selectedItem->rtti() << endl;
        if (m_selectedItem->rtti() == Q3CanvasEllipse::RTTI)
        {
            EqualizerCircle* circle = (EqualizerCircle*) m_selectedItem;
            circle->setLocation(event->pos());
      //      kDebug() << "dragged" <<endl;
        }
    }
}
void
EqualizerCanvasView::contentsMouseReleaseEvent(QMouseEvent */*event*/)
{
    if (m_selectedItem && m_selectedItem->rtti() == Q3CanvasEllipse::RTTI)
    {
        emit eqChanged();
    }
}

/**
 * m =(y1-y2)/(x1-x2)
 * b = m*x1 - y1
 * y = m*xCoord -b
 */
int
EqualizerCanvasView::getY(int xCoord)
{
//     int y1 = line->startPoint().y();
//     int y2 = line->endPoint().y();
//     int x1 = line->startPoint().x();
//     int x2 = line->endPoint().x();
//     double slope = double(y1-y2) / double(x1-x2);
//     kDebug() << slope << "= (" << y1 << " - " << y2 << ")/(" << x1  << " - " << x2 << ')' << endl;
//     double b = slope*double(x1 - y1);
//     kDebug() << b << " = " << slope <<" * "<< x1 << " - " << y1 << endl;
//     double y = double(xCoord)*slope - b;
//     kDebug() << y << " = " << xCoord << '*' << slope << " - " << b << endl;
    //QPointArray* yAxis = new QPointArray(canvas->height());
    //yAxis->makeEllipse(xCoord, canvas()->height()/2,1,canvas->height());
    //canvas()->collisions(yAxis,0,false);
    //delete yAxis;
    Q3MemArray<int> collidedPoints(20);
    unsigned int collisionNum = 0;
    for(int i=0; i<canvas()->height();i++)
    {
        Q3CanvasItemList list = canvas()->collisions(QPoint(xCoord,i));
        if( ! list.isEmpty())
        {
            if(collidedPoints.size() <= collisionNum)
                collidedPoints.resize(collidedPoints.size()+100);//for the steep slops
            collidedPoints[collisionNum] = i;
            collisionNum++;
        }
    }
    collisionNum--;
    return collidedPoints[collisionNum] - collisionNum/2;
}

Q3CanvasLine*
EqualizerCanvasView::makeLine(QPoint startPoint, QPoint endPoint)
{
    Q3CanvasLine* line = new Q3CanvasLine(canvas());
    line->setPoints(startPoint.x(),
                   startPoint.y(),
                   endPoint.x(),
                   endPoint.y());
    line->setZ(-50);
    line->setPen(QPen(m_pen));
    line->show();
    return line;
}
Q3ValueList<int>
EqualizerCanvasView::currentSettings()
{
    int step = canvas()->width()/10;
    int i;
    Q3ValueList<int> ret;
    ret << (int)(m_circleList->first()->y() - 100)*-1;
    for(i=1; i<9; i++)
        ret << (getY(i*step) - 100)*-1;
    ret << (int)(m_circleList->last()->y() - 100)*-1;
//    kDebug() << "sending " << ret << endl;
    return ret;
}

//////////////////////
//EqualizerCircle
//////////////////////
EqualizerCircle::EqualizerCircle(int x, int y, Q3Canvas *canvas, Q3CanvasLine* line1, Q3CanvasLine* line2,
Q3PtrList<EqualizerCircle>* circleList )
: Q3CanvasEllipse(15, 15, canvas)
{
    m_line1 = line1;
    m_line2 = line2;
    setBrush(QBrush(Qt::blue));
    move(x,y);
    m_circleList = circleList;

    EqualizerCircle* it;
    int index = -1;
    bool inserted = false;
    for ( it = m_circleList->first(); it; it = m_circleList->next() )
    {
        if(it->x() > x)
        {
            index = m_circleList->find(it);
            //if(index != 0)
            //{
                m_circleList->insert(index,this);
                inserted = true;
            //}
            break;
        }
    }
    if( !inserted )
        m_circleList->append(this);

    //clean up the loose pointer for the line that was split
    unsigned int circleIndex = m_circleList->find(this);
    if(circleIndex > 0)
        m_circleList->at(circleIndex-1)->setLine(RIGHT,m_line1);
    if(circleIndex < m_circleList->count()-1)
         m_circleList->at(circleIndex+1)->setLine(LEFT,m_line2);
}

void EqualizerCircle::setLocation(const QPoint &newLocation)
{
    unsigned int circleIndex = m_circleList->find(this);
    double xMin, xMax;
    QPoint correctedLoc(newLocation);
    if(m_line1 == 0 || m_line2 == 0)
    {//if the line is on the edges of the board, make it only move vertically
        correctedLoc.setX( (int) x());
    }
    else
    {
        xMin = 0; xMax = canvas()->width();
        if(circleIndex > 0)
        {
        // kDebug() << "circleIndex " << circleIndex << endl;
            xMin = m_circleList->at(circleIndex - 1)->x();
        }
        if(circleIndex < m_circleList->count()-1)
        {
            //kDebug() << "circleIndex " << circleIndex << " count " << m_circleList->count() << endl;
            xMax = m_circleList->at(circleIndex + 1)->x();
        }
        if(newLocation.x() < xMin)
        {
            //kDebug() << "set " << newLocation.x() << " to xMin " << xMin << endl;
            correctedLoc.setX( (int) (xMin + 1.0) );
        }
        else if (newLocation.x() > xMax)
        {
        //kDebug() << "set " << newLocation.x() << " to xMax " << xMax << endl;
            correctedLoc.setX( (int) (xMax - 1.0) );
        }
    }
    if(newLocation.y() > canvas()->height())
        correctedLoc.setY(canvas()->height());
    else if (newLocation.y() < 0)
        correctedLoc.setY(0);
    move(correctedLoc.x(), correctedLoc.y());
    if (m_line1) //account for circles on the edge
        m_line1->setPoints( m_line1->startPoint().x()
                       ,m_line1->startPoint().y()
                       ,correctedLoc.x()
                       ,correctedLoc.y());
    if(m_line2)
        m_line2->setPoints(correctedLoc.x()
                       , correctedLoc.y()
                       , m_line2->endPoint().x()
                       , m_line2->endPoint().y());
    canvas()->update();
}

void EqualizerCircle::setLine(WhichLine lineNum, Q3CanvasLine* line)
{
    if(lineNum == LEFT)
        m_line1 = line;
    else
        m_line2 = line;
}

void CallAmarok::updateEq()
{
        QByteArray data;
        QDataStream arg(data, QIODevice::WriteOnly);
        arg << m_preampSlider->value() *-1;
        Q3ValueList<int> cs = m_canvasView->currentSettings();
        Q3ValueList<int>::iterator it;
        for ( it = cs.begin(); it != cs.end(); ++it )
            arg << (*it);
        KApplication::dcopClient()->send("amarok", "player",
            "setEqualizer(int,int,int,int,int,int,int,int,int,int,int)"
            , data);
}


#include "equalizercanvasview.moc"
