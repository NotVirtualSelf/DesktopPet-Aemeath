#include "desktop_pet.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    DesktopPet w;
    w.show();
    return a.exec();
}