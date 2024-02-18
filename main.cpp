#include "etiquetas.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication a(argc, argv);
    Etiquetas w;
    w.show();
    return a.exec();
}
