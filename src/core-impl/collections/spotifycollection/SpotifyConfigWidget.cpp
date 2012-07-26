#include "SpotifyConfigWidget.h"
#include "ui_SpotifyConfigWidget.h"

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
