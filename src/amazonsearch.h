// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#ifndef AMAZONSEARCH_H
#define AMAZONSEARCH_H

#include <qdialog.h>
#include <qimage.h>

class QLabel;
class KLineEdit;

class AmazonSearch : public QDialog
{
    Q_OBJECT

signals:
    void imageReady( const QImage& image );

public:
    AmazonSearch( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );

    QLabel* m_textLabel;
    KLineEdit* m_searchString;

private slots:
    void openFile();

};

#endif // AMAZONSEARCH_H
