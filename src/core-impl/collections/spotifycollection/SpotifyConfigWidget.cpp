#include "spotifyconfigwidget.h"
#include "ui_spotifyconfigwidget.h"

SpotifyConfigWidget::SpotifyConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpotifyConfigWidget)
{
    ui->setupUi(this);
}

SpotifyConfigWidget::~SpotifyConfigWidget()
{
    delete ui;
}
