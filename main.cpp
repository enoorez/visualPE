#include "mainwindow.h"
#include <QApplication>
#include "QTranslator.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator tsor;           //创建翻译器
    tsor.load( "zh_CN.qm" );    //加载语言包
    a.installTranslator( &tsor ); //安装翻译器

    MainWindow w;
    w.show();

    return a.exec();
}
