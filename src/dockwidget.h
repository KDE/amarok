#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QDockWidget>
#include <QPainter>
#include <QPaintEvent>

/**
 * @class DockWidget
 * @short Customized QDockWidget, which limits some features and adds others
 * @author T.R.Shashwath
 */
class DockWidget : public QDockWidget
{
public:
    DockWidget( const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0 );
    DockWidget();
    void paintEvent(QPaintEvent *e);
};

#endif
