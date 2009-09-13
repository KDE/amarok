#include <QCoreApplication>


#include "signer.h"


int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    signer sig;
	if (sig.setParams(argc, argv)) {
		return app.exec();
	} else {
		return 0;
	}
}
