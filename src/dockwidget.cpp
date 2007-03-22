#include "dockwidget.h"

DockWidget::DockWidget( const QString &title, QWidget *parent, Qt::WindowFlags flags ) :
    QDockWidget( title, parent, flags )
{
}

DockWidget::DockWidget() :
    QDockWidget()
{
    setFeatures( QDockWidget::DockWidgetMovable );
}
void DockWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.eraseRect( e->rect() );
}

