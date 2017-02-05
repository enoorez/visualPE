#include "mainwindow.h"
#include <QApplication>
#include "QTranslator.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator tsor;           //����������
    tsor.load( "zh_CN.qm" );    //�������԰�
    a.installTranslator( &tsor ); //��װ������

    MainWindow w;
    w.show();

    return a.exec();
}
