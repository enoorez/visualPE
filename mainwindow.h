#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qboxlayout.h"
#include "Src\HexTextEditor\hextexteditor.h"
#include "Src\TypeTree.h"

namespace Ui {
class MainWindow;
}

class QTreeWidget;
class QLabel;
class HexTextEditorColorConfigureDlg;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


    void dropEvent( QDropEvent * );
    void dragEnterEvent( QDragEnterEvent * );

    void openFile( const QString& path );
    void analysisPEFile();

public slots:
    void onItemClicked( QTreeWidgetItem * item , int column );
    void onCurrentItemChanged( QTreeWidgetItem * current , QTreeWidgetItem * previous );
    void onEditHexTextColumn( int nLine , int nRow );
    void onEditStringColumn( int nLine , int nRow );

    void onMenuOpenAction( );
    void onMenuSaveAction( );
    void onMenuSaveasAction( );
    void onMenuColorConfigtrue( );
    
protected:


public:
    HexTextEditorColorConfigureDlg* mColorCfgDlg;
    QString         mFilePath;
    Ui::MainWindow* ui;
    HexTextEditor*  mHexTextEditor;
    QTreeWidget*    mTreeWidget;
    TypeTree*       mStructTree;
    QByteArray      mFileData;
    QLabel*         mLineInfo;
    QLabel*         mLinePosition;
    QLabel*         mLineDataComment;

};

#endif // MAINWINDOW_H
