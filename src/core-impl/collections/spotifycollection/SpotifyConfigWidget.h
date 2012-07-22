#ifndef SPOTIFYCONFIGWIDGET_H
#define SPOTIFYCONFIGWIDGET_H

#include <QWidget>

namespace Ui {
class SpotifyConfigWidget;
}

class SpotifyConfigWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit SpotifyConfigWidget(QWidget *parent = 0);
    ~SpotifyConfigWidget();
    
private:
    Ui::SpotifyConfigWidget *ui;
};

#endif // SPOTIFYCONFIGWIDGET_H
