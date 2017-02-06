#ifndef HEXTEXTEDITORCOLORCONFIGURATIONEDLG_H
#define HEXTEXTEDITORCOLORCONFIGURATIONEDLG_H

#include <QDialog>
#include "qframe.h"

class QPushButton;
class QMouseEvent;
class QPaintEvent;
class QHBoxLayout;
class MainWindow;

class ColorArea : public QFrame
{
    Q_OBJECT

protected:
    void paintEvent(QPaintEvent * e);

    void mouseReleaseEvent(QMouseEvent *event);

public:
    ColorArea(QWidget* parent ,const QColor& color = QColor(Qt::darkGreen));


    void setBackgroundColor(const QColor& color);

signals:
    void clicked( const QColor& color );

protected:
    QColor  mBackgroundColor;

};


class HexTextEditorColorConfigureDlg : public QDialog
{
    Q_OBJECT

public:
    HexTextEditorColorConfigureDlg( MainWindow* parent );

public slots:

    // 响应按钮点击函数
    void    onLinenumberCorlorClick(const QColor &c);
    void    onLinenumberBackgroundCorlorClick(const QColor &);
    void    onHexTextColorClick(const QColor &);
    void    onHexTextBackgroundColorClick(const QColor&);
    void    onHexTextSelectColorClick(const QColor&);

    QHBoxLayout*    addColorAreaAndButton( const QString& btnTex, ColorArea* btn  );
protected:
    MainWindow*   mWindow;
    ColorArea*    mLinenumber;
    ColorArea*    mLinenumberBackground;
    ColorArea*    mHexText;
    ColorArea*    mHexTextBackground;
    ColorArea*    mHexTextSelect;
};

#endif // HEXTEXTEDITORCOLORCONFIGURATIONEDLG_H
