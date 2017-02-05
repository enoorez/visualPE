#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <windows.h>
#include "Src\HexTextEditor\tokenlist.h"
#include <QDebug>
#include <QTreeWidget>
#include "qevent.h"
#include <QMimeData>
#include "Src\pe\pe.h"
#include "QTextCodec"
#include <QLabel>

MainWindow::MainWindow( QWidget *parent )
    :QMainWindow( parent ) ,
    ui( new Ui::MainWindow )
{

    ui->setupUi( this );
    setWindowTitle( tr( "VisualPETool" ) );

    mHexTextEditor = new HexTextEditor( ui->centralWidget );
    ui->horizontalLayout->addWidget( mHexTextEditor );

    mTreeWidget = new QTreeWidget( ui->centralWidget );
    ui->horizontalLayout->addWidget( mTreeWidget );

    QTreeWidgetItem *qtreewidgetitem = mTreeWidget->headerItem( );
    qtreewidgetitem->setText( 0 , tr( "field Name" ) );
    qtreewidgetitem->setText( 1 , tr( "field value" ) );
    qtreewidgetitem->setText( 2 , tr( "RVA -> offset" ) );
    qtreewidgetitem->setText( 3 , tr( "comment" ) );

    mTreeWidget->setColumnWidth( 0 , 240 );

    mStructTree = new TypeTree( mTreeWidget , mHexTextEditor );
    
    // 连接树控件的节点点击信号.
    connect( mTreeWidget ,
             SIGNAL( itemClicked( QTreeWidgetItem* , int ) ) ,
             this ,
             SLOT( onItemClicked(QTreeWidgetItem* , int  ) ) );

    connect( mTreeWidget ,
             SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem*) ) ,
             this ,
             SLOT( onCurrentItemChanged( QTreeWidgetItem* , QTreeWidgetItem*) ) );


    connect( mHexTextEditor ,
             SIGNAL( mouseOnHexText( int , int ) ) ,
             this ,
             SLOT( onEditHexTextColumn( int , int ) ) );

    connect( mHexTextEditor ,
             SIGNAL( mouseOnString( int , int ) ) ,
             this ,
             SLOT( onEditStringColumn( int , int ) ) );

    mLinePosition = new QLabel;
    mLineInfo = new QLabel;
    mLineDataComment = new QLabel;

    mLinePosition->setMinimumWidth( 80 ); // 鼠标信息
    mLineInfo->setMinimumWidth( 120);     
    mLineDataComment->setMinimumWidth( 210 );

    ui->statusBar->addWidget( mLinePosition );
    ui->statusBar->addWidget( mLineInfo );
    ui->statusBar->addWidget( mLineDataComment );



    setAcceptDrops( true );
    mHexTextEditor->setAcceptDrops( false );
}

MainWindow::~MainWindow()
{
    delete mStructTree;
    delete ui;
}

void MainWindow::onItemClicked( QTreeWidgetItem * item , int column )
{
    Field* field = nullptr;
    if( column == 0 ) {
        field = (Field*)item->data( 0 , Qt::UserRole ).toInt( );
        TypeTree::USERDATAONITEM*  pRvaInfo = ( TypeTree::USERDATAONITEM* )item->data( 2 , Qt::UserRole ).toInt( );
        if( pRvaInfo==nullptr || pRvaInfo->type == 0 ) {

            int nbeg = 0;
            int len = 0;
            field->itemOnTextEditRangle( nbeg , len );
            mHexTextEditor->setSelection( nbeg , nbeg + len );
            
            // 判断要显示的位置是为处于视口
            if( !mHexTextEditor->posIsOnVisual( nbeg + len ) ) {
                mHexTextEditor->setVisualPos( nbeg );
            }
        }
        else if( pRvaInfo->type > 0x200 ) {// 展开重定位表的TypeOffset块
           
            
            field = (Field*)item->data( 0 , Qt::UserRole ).toInt( );
            PE peFile( mFileData.data( ) );
            Typeoffset* pOffset = (Typeoffset*)( pRvaInfo->rva + peFile );
            Field* temp = nullptr;
            QString buff;
            QString comment;
            DWORD   dwRelocalAddressOfs = 0;
            DWORD   dwRelocalAddressToDataVA;
            DWORD   dwRelocalAddressToDataRVA;
            DWORD   dwRelocalAddressToDataOffset;

            char    hexText[ 16 * 3 + 1 ];
            for( int i = 0; i < pRvaInfo->size; ++i ) {

                dwRelocalAddressOfs = peFile.RVAToOfs( pOffset[i].wOffset + pRvaInfo->type );

                dwRelocalAddressToDataVA = *(DWORD*)( dwRelocalAddressOfs + peFile );
                dwRelocalAddressToDataRVA = dwRelocalAddressToDataVA - 0x400000;
                dwRelocalAddressToDataOffset = peFile.RVAToOfs( dwRelocalAddressToDataRVA ) ;
                HexTextEditor::hexToString( (char*)(dwRelocalAddressToDataOffset + peFile) , 16 , hexText );

                buff.sprintf( "ofs:%04X,type:%d," , pOffset[ i ].wOffset , pOffset[ i ].wType );

                comment.sprintf( "[VA:%X]=>[RVA:%X]=>[OFS:%X] => %s" ,
                                 dwRelocalAddressToDataVA ,
                                 dwRelocalAddressToDataRVA ,
                                 dwRelocalAddressToDataOffset ,
                                 hexText );

                temp = mStructTree->addField( e_struct ,
                                              ( "TypeOffset" ) ,
                                              pRvaInfo->rva + i * sizeof( WORD ) ,
                                              sizeof( WORD ) ,
                                              comment ,
                                              buff ,
                                              field ,
                                              0 ,
                                              dwRelocalAddressOfs ,
                                              sizeof( DWORD )
                                              );
            }

            // 删除
            delete pRvaInfo;
            item->setData( 2 , Qt::UserRole , 0 );
        }
    }
    else if( column == 2 ) {
        TypeTree::USERDATAONITEM*  pRvaInfo = ( TypeTree::USERDATAONITEM* )item->data( 2 , Qt::UserRole ).toInt( );
        if( pRvaInfo != nullptr ) {

            mHexTextEditor->setSelection( pRvaInfo->rva , pRvaInfo->rva + pRvaInfo->size );
            mHexTextEditor->setVisualPos( pRvaInfo->rva - ( mHexTextEditor->getMaxVisualLine( ) / 2 ) * 16 );
        }
    }
}

void MainWindow::onCurrentItemChanged( QTreeWidgetItem * current , QTreeWidgetItem * previous )
{
    previous;
    onItemClicked( current , 0 );
}

void MainWindow::dropEvent( QDropEvent * event )
{
    QList<QUrl> urls = event->mimeData( )->urls( );
    QString fileName = urls.first( ).toLocalFile( );

    QFile file( fileName );
    file.open( QIODevice::ReadOnly );
    if( file.isOpen( ) )
    {
        mTreeWidget->clear( );
        mHexTextEditor->cleaer( );
        mStructTree->clear( );

        mFileData = file.readAll( );
        mHexTextEditor->setHexData( mFileData );

        analysisPEFile( );
    }
}


void MainWindow::dragEnterEvent( QDragEnterEvent *event )
{
    if( event->mimeData( )->hasFormat( "text/uri-list" ) ) {
        //允许放下文件
        event->acceptProposedAction( );
    }
}

#define fieldOffset(TYPE, MEMBER) ((unsigned long)(&(((TYPE *)0)->MEMBER)))
#define OPTHDROFFSET( field)                            \
    (bIs32PeFile? (fieldOffset(IMAGE_OPTIONAL_HEADER32,field)) :(fieldOffset(IMAGE_OPTIONAL_HEADER64,field))) 



void MainWindow::analysisPEFile( )
{
    char*               pDllName;
    Field*              temp;
    Field*              parent ;
    Field*              fileHeaderField = nullptr;
    Field*              optionHeaderFiele = nullptr;
    TokenList::Token    token;
    TokenList::Token    tableToken( 0 , 0 , QColor( Qt::green ) );
    int                 parentBase;
    bool                bIs32PeFile = false;
    PE                  peFile( (char*)mFileData.data( ) , true );
    pe_SectionHeader*   pSectionHeader;
    TokenList::Token    sectionDataToken( 0 , 0 , QColor( Qt::darkGreen ) );
    QColor              sectionDatacolor[ 5 ] = { Qt::darkMagenta , Qt::green , Qt::darkYellow , Qt::gray , Qt::darkGreen };
    DWORD               addressTableOffset;

    QString name[ 16 ] = {
        tr( "import table" ) ,
        tr( "exprot table" ) ,
        tr( "resources table" ) ,
        tr( "Exception table" ) ,
        tr( "Certificate table" ) ,
        tr( "Base relocation table" ) ,
        tr( "Debugging information starting" ) ,
        tr( "Architecture-specific data" ) ,
        tr( "Global pointer register relative" ) ,
        tr( "TLS" ) ,
        tr( "Local configuration table" ) ,
        tr( "Bound import table" ) ,
        tr( "IAT" ) ,
        tr( "Delay import descriptor" ) ,
        tr( "CLR Runtime Header" ) ,
        tr( "Reserved" ) ,
    };

    // 判断是否是PE文件
    if( peFile.getDosHeader( )->e_magic != 'ZM'
        || peFile.getNtHeader( )->Signature != 'EP' ) {
        return ;
    }
    

    // 解析文件头
    token( 0 , sizeof( IMAGE_DOS_HEADER ) , QColor( Qt::gray ) );
    parent = mStructTree->addField( e_struct ,
                                    ( "IMAGE_DOS_HEADER" ) ,
                                    0 ,
                                    sizeof( IMAGE_DOS_HEADER ) ,
                                    tr( "pe file dos header" ) ,
                                    0 ,
                                    NULL ,
                                    &token
                                    );
   
    mStructTree->addField( e_base ,
                           ("WORD e_magic" ),
                           fieldOffset( IMAGE_DOS_HEADER , e_magic ) ,
                           sizeof( WORD ) ,
                           tr("magic word" ) ,
                           peFile.getDosHeader( )->e_magic ,
                           parent
                           );

    mStructTree->addField( e_base ,
                           ( "WORD e_cblp" ) ,
                           fieldOffset(IMAGE_DOS_HEADER,e_cblp),
                           sizeof( WORD ) ,
                           "",
                           peFile.getDosHeader( )->e_cblp ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           tr( "WORD e_cblp" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_cblp ) ,
                           sizeof( WORD ) ,
                           "" ,
                           peFile.getDosHeader( )->e_cblp ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           ( "WORD e_cp" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_cp ) ,
                           sizeof( WORD ) ,
                           "" ,
                           peFile.getDosHeader( )->e_cp ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           tr( "WORD e_crlc" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_crlc ) ,
                           sizeof( WORD ) ,
                           "" ,
                           peFile.getDosHeader( )->e_crlc ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           ( "WORD e_cparhdr" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_cparhdr ) ,
                           sizeof( WORD ) ,
                           "" ,
                           peFile.getDosHeader( )->e_cparhdr ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           tr( "WORD e_minalloc" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_minalloc ) ,
                           sizeof( WORD ) ,
                           "" ,
                           peFile.getDosHeader( )->e_minalloc ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           ( "WORD e_maxalloc" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_maxalloc ) ,
                           sizeof( peFile.getDosHeader( )->e_minalloc ) ,
                           "" ,
                           peFile.getDosHeader( )->e_maxalloc ,
                           parent
                           );


    mStructTree->addField( e_base ,
                           tr( "WORD e_ss" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_ss ) ,
                           sizeof( peFile.getDosHeader( )->e_ss ) ,
                           "" ,
                           peFile.getDosHeader( )->e_ss ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           ( "WORD e_sp" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_sp ) ,
                           sizeof( peFile.getDosHeader( )->e_sp ) ,
                           "" ,
                           peFile.getDosHeader( )->e_sp ,
                           parent
                           );
    mStructTree->addField( e_base ,
                           ( "LONG e_lfanew" ) ,
                           fieldOffset( IMAGE_DOS_HEADER , e_lfanew ) ,
                           sizeof( peFile.getDosHeader( )->e_lfanew ) ,
                           tr("pointer to nt header(is a file offset)") ,
                           (int)peFile.getDosHeader( )->e_lfanew ,
                           parent
                           );
   

    // PE文件头
    parentBase = peFile.getDosHeader( )->e_lfanew;
    if( peFile.getFileHeader( )->Characteristics&IMAGE_FILE_32BIT_MACHINE )
    {
        parent = mStructTree->addField( e_struct ,
                                        ( "IMAGE_NT_HEADERS32" ) ,
                                        parentBase ,
                                        sizeof( IMAGE_NT_HEADERS32 ) ,
                                        tr( "pe file Nt header(32Bit)" ) ,
                                        0
                                        ) ;
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        parent = mStructTree->addField( e_struct ,
                                        ( "IMAGE_NT_HEADERS64" ) ,
                                        parentBase ,
                                        sizeof( IMAGE_NT_HEADERS64 ) ,
                                        tr( "PE file nt header(64Bit)" ) ,
                                        0
                                        ) ;
    }
   
   mStructTree->addField( e_base ,
                           ( "DWORD Signature" ) ,
                           parentBase + fieldOffset( IMAGE_NT_HEADERS32 , Signature ) ,
                           sizeof( peFile.getNtHeader( )->Signature ) ,
                           tr( "pe file signature" ) ,
                           (int)peFile.getNtHeader( )->Signature ,
                           parent
                           );


    
    token( parentBase + fieldOffset( IMAGE_NT_HEADERS32 , FileHeader ) , sizeof( peFile.getNtHeader( )->FileHeader ) , QColor( Qt::blue ) );
    fileHeaderField = mStructTree->addField( e_struct ,
                                             ( "IMAGE_FILE_HEADER FileHeader" ) ,
                                             parentBase + fieldOffset( IMAGE_NT_HEADERS32 , FileHeader ) ,
                                             sizeof( peFile.getNtHeader( )->FileHeader ) ,
                                             tr( "PE file fileHeader" ) ,
                                             0 ,
                                             parent,
                                             &token
                                             );

    token( parentBase + fieldOffset( IMAGE_NT_HEADERS32 , OptionalHeader ) , sizeof( peFile.getNtHeader( )->OptionalHeader ) , QColor( Qt::red ) );
    if( peFile.getFileHeader( )->Characteristics&IMAGE_FILE_32BIT_MACHINE ) {
        optionHeaderFiele = mStructTree->addField( e_struct ,
                                                   ( "IMAGE_OPTIONAL_HEADER32 OptionalHeader" ) ,
                                                   parentBase + fieldOffset( IMAGE_NT_HEADERS32 , OptionalHeader ) ,
                                                   peFile.getFileHeader( )->SizeOfOptionalHeader ,
                                                   tr( "32Bit pe file option header" ) ,
                                                   0 ,
                                                   parent ,
                                                   &token
                                                   );
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        optionHeaderFiele = mStructTree->addField( e_struct ,
                                                   ( "IMAGE_NT_HEADERS64 OptionalHeader" ) ,
                                                   parentBase + fieldOffset( IMAGE_NT_HEADERS64 , OptionalHeader ) ,
                                                   peFile.getFileHeader()->SizeOfOptionalHeader ,
                                                   tr( "64Bit pe file option header" ) ,
                                                   0 ,
                                                   parent ,
                                                   &token
                                                   );
    }

    parentBase += fieldOffset( IMAGE_NT_HEADERS32 , FileHeader );
    mStructTree->addField( e_base ,
                           ( "WORD Machine" ) ,
                           parentBase + fieldOffset( IMAGE_FILE_HEADER , Machine ) ,
                           sizeof( peFile.getFileHeader( )->Machine ) ,
                           tr( "CPU Kind" ) ,
                           peFile.getFileHeader()->Machine ,
                           fileHeaderField
                           );
    mStructTree->addField( e_base ,
                           ( "WORD NumberOfSections" ) ,
                           parentBase + fieldOffset( IMAGE_FILE_HEADER , NumberOfSections ) ,
                           sizeof( peFile.getFileHeader( )->NumberOfSections ) ,
                           tr( "PE section Total" ) ,
                           peFile.getFileHeader( )->NumberOfSections ,
                           fileHeaderField
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD TimeDateStamp" ) ,
                           parentBase + fieldOffset( IMAGE_FILE_HEADER , TimeDateStamp ) ,
                           sizeof( peFile.getFileHeader( )->TimeDateStamp ) ,
                           tr( "TimeDateStamp" ) ,
                           (int)peFile.getFileHeader( )->TimeDateStamp,
                           fileHeaderField
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD NumberOfSymbols" ) ,
                           parentBase + fieldOffset( IMAGE_FILE_HEADER , NumberOfSymbols ) ,
                           sizeof( peFile.getFileHeader( )->NumberOfSymbols ) ,
                           tr( "NumberOfSymbols(invalid)" ) ,
                           (int)peFile.getFileHeader( )->NumberOfSymbols ,
                           fileHeaderField
                           );
    mStructTree->addField( e_base ,
                           ( "WORD  SizeOfOptionalHeader" ) ,
                           parentBase + fieldOffset( IMAGE_FILE_HEADER , SizeOfOptionalHeader ) ,
                           sizeof( peFile.getFileHeader( )->SizeOfOptionalHeader ) ,
                           tr( "Size of OptionalHeader" ) ,
                           peFile.getFileHeader( )->SizeOfOptionalHeader ,
                           fileHeaderField
                           );
    mStructTree->addField( e_base ,
                           ( "WORD  Characteristics" ) ,
                           parentBase + fieldOffset( IMAGE_FILE_HEADER , Characteristics ) ,
                           sizeof( peFile.getFileHeader( )->Characteristics ) ,
                           tr( "PE file Characteristics" ) ,
                           peFile.getFileHeader( )->Characteristics ,
                           fileHeaderField
                           );


    // 扩展头   
    // 判断是否是32位的PE文件
    bIs32PeFile = peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC;

    parentBase = peFile.getDosHeader( )->e_lfanew + fieldOffset( IMAGE_NT_HEADERS32 , OptionalHeader );

    mStructTree->addField( e_base ,
                           ( "WORD Magic" ) ,
                           parentBase + ( OPTHDROFFSET( Magic ) ) ,
                           sizeof( peFile.getOptionHeader32( )->Magic ) ,
                           tr( "PE file type (32Bit/64Bit/ROM)" ) ,
                           (int)peFile.getOptionHeader32( )->Magic ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "BYTE MajorLinkerVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MajorLinkerVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MajorLinkerVersion ) ,
                           tr( "Major Linker Version" ) ,
                           peFile.getOptionHeader32( )->MajorLinkerVersion ,
                           optionHeaderFiele
                           );

    mStructTree->addField( e_base ,
                           ( "BYTE MinorLinkerVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MinorLinkerVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MinorLinkerVersion ) ,
                           tr( "Minor linker version" ) ,
                           peFile.getOptionHeader32( )->MinorLinkerVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD SizeOfCode" ) ,
                           parentBase + ( OPTHDROFFSET( SizeOfCode ) ) ,
                           sizeof( peFile.getOptionHeader32( )->SizeOfCode ) ,
                           tr( "PE file code segment size" ) ,
                           (int)peFile.getOptionHeader32( )->SizeOfCode ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD SizeOfInitializedData" ) ,
                           parentBase + ( OPTHDROFFSET( SizeOfInitializedData ) ) ,
                           sizeof( peFile.getOptionHeader32( )->SizeOfInitializedData ) ,
                           tr( "PE file initialized data size" ) ,
                           (int)peFile.getOptionHeader32( )->SizeOfInitializedData ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD SizeOfUninitializedData" ) ,
                           parentBase + ( OPTHDROFFSET( SizeOfUninitializedData ) ) ,
                           sizeof( peFile.getOptionHeader32( )->SizeOfUninitializedData ) ,
                           tr( "PE file uninitialized data size" ) ,
                           (int)peFile.getOptionHeader32( )->SizeOfUninitializedData ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD AddressOfEntryPoint" ) ,
                           parentBase + ( OPTHDROFFSET( AddressOfEntryPoint ) ) ,
                           sizeof( peFile.getOptionHeader32( )->AddressOfEntryPoint ) ,
                           tr( "OEP(is a RVA)" ) ,
                           (int)peFile.getOptionHeader32( )->AddressOfEntryPoint ,
                           optionHeaderFiele , 0 , peFile.RVAToOfs( (int)peFile.getOptionHeader32( )->AddressOfEntryPoint )
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD BaseOfCode" ) ,
                           parentBase + ( OPTHDROFFSET( BaseOfCode ) ) ,
                           sizeof( peFile.getOptionHeader32( )->BaseOfCode ) ,
                           tr( "PE data segment's RVA" ) ,
                           (int)peFile.getOptionHeader32( )->BaseOfCode ,
                           optionHeaderFiele , 0 , peFile.RVAToOfs( (int)peFile.getOptionHeader32( )->BaseOfCode )
                           );

    if( bIs32PeFile ) {
        mStructTree->addField( e_base ,
                               ( "DWORD BaseOfData" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER32 , BaseOfData ) ) ,
                               sizeof( peFile.getOptionHeader32( )->BaseOfData ) ,
                               tr( "PE data segment's RVA" ) ,
                               (int)peFile.getOptionHeader32( )->BaseOfData ,
                               optionHeaderFiele , 0 , peFile.RVAToOfs( (int)peFile.getOptionHeader32( )->BaseOfData )
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD ImageBase" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER32 , ImageBase ) ) ,
                               sizeof( peFile.getOptionHeader32( )->ImageBase ) ,
                               tr( "pe file lao" ) ,
                               (int)peFile.getOptionHeader32( )->ImageBase ,
                               optionHeaderFiele 
                               );
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {

        mStructTree->addField( e_base ,
                               ( "ULONGLONG ImageBase" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER64 , ImageBase ) ) ,
                               sizeof( peFile.getOptionHeader64( )->ImageBase ) ,
                               tr( "image address in memory " ) ,
                               (int)peFile.getOptionHeader64( )->ImageBase ,
                               optionHeaderFiele 
                               );
    }

    mStructTree->addField( e_base ,
                           ( "DWORD SectionAlignment" ) ,
                           parentBase + ( OPTHDROFFSET( SectionAlignment ) ) ,
                           sizeof( peFile.getOptionHeader32( )->SectionAlignment ) ,
                           tr( "Section in memory alignment value" ) ,
                           (int)peFile.getOptionHeader32( )->SectionAlignment ,
                           optionHeaderFiele );

    mStructTree->addField( e_base ,
                           ( "DWORD FileAlignment" ) ,
                           parentBase + ( OPTHDROFFSET( FileAlignment ) ) ,
                           sizeof( peFile.getOptionHeader32( )->FileAlignment ) ,
                           tr( "Section in disk alignment value" ) ,
                           (int)peFile.getOptionHeader32( )->FileAlignment ,
                           optionHeaderFiele
                           );

    mStructTree->addField( e_base ,
                           ( "WORD MajorOperatingSystemVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MajorOperatingSystemVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MajorOperatingSystemVersion ) ,
                           tr( "Major OperatingSystem Version" ) ,
                           (int)peFile.getOptionHeader32( )->MajorOperatingSystemVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD MinorOperatingSystemVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MinorOperatingSystemVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MinorOperatingSystemVersion ) ,
                           tr( "Minor OperatingSystem Version" ) ,
                           (int)peFile.getOptionHeader32( )->MinorOperatingSystemVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD MajorImageVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MajorImageVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MajorImageVersion ) ,
                           tr( " Major PE Image Version" ) ,
                           (int)peFile.getOptionHeader32( )->MajorImageVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD MinorImageVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MinorImageVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MinorImageVersion ) ,
                           tr( "PE Minor Image Version" ) ,
                           (int)peFile.getOptionHeader32( )->MinorImageVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD MajorSubsystemVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MajorSubsystemVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MajorSubsystemVersion ) ,
                           tr( "Major Subsystem Version" ) ,
                           (int)peFile.getOptionHeader32( )->MajorSubsystemVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD MinorSubsystemVersion" ) ,
                           parentBase + ( OPTHDROFFSET( MinorSubsystemVersion ) ) ,
                           sizeof( peFile.getOptionHeader32( )->MinorSubsystemVersion ) ,
                           tr( "Minor Subsystem Version" ) ,
                           (int)peFile.getOptionHeader32( )->MinorSubsystemVersion ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD Win32VersionValue" ) ,
                           parentBase + ( OPTHDROFFSET( Win32VersionValue ) ) ,
                           sizeof( peFile.getOptionHeader32( )->Win32VersionValue ) ,
                           tr( "Win32 Version Value" ) ,
                           (int)peFile.getOptionHeader32( )->Win32VersionValue ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD SizeOfImage" ) ,
                           parentBase + ( OPTHDROFFSET( SizeOfImage ) ) ,
                           sizeof( peFile.getOptionHeader32( )->SizeOfImage ) ,
                           tr( "size of image in memory " ) ,
                           (int)peFile.getOptionHeader32( )->SizeOfImage ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD SizeOfHeaders" ) ,
                           parentBase + ( OPTHDROFFSET( SizeOfHeaders ) ) ,
                           sizeof( peFile.getOptionHeader32( )->SizeOfHeaders ) ,
                           tr( "Size Of Headers" ) ,
                           (int)peFile.getOptionHeader32( )->SizeOfHeaders ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD CheckSum" ) ,
                           parentBase + ( OPTHDROFFSET( CheckSum ) ) ,
                           sizeof( peFile.getOptionHeader32( )->CheckSum ) ,
                           tr( "CheckSum" ) ,
                           (int)peFile.getOptionHeader32( )->CheckSum ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD Subsystem" ) ,
                           parentBase + ( OPTHDROFFSET( Subsystem ) ) ,
                           sizeof( peFile.getOptionHeader32( )->Subsystem ) ,
                           tr( "Subsystem" ) ,
                           (int)peFile.getOptionHeader32( )->Subsystem ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "WORD DllCharacteristics" ) ,
                           parentBase + ( OPTHDROFFSET( DllCharacteristics ) ) ,
                           sizeof( peFile.getOptionHeader32( )->DllCharacteristics ) ,
                           tr( "DllCharacteristics" ) ,
                           (int)peFile.getOptionHeader32( )->DllCharacteristics ,
                           optionHeaderFiele
                           );


    if( bIs32PeFile ) {
        mStructTree->addField( e_base ,
                               ( "DWORD SizeOfStackReserve" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER32 , SizeOfStackReserve ) ) ,
                               sizeof( peFile.getOptionHeader32( )->SizeOfStackReserve ) ,
                               tr( "SizeOfStackReserve" ) ,
                               (int)peFile.getOptionHeader32( )->SizeOfStackReserve ,
                               optionHeaderFiele
                               );
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        mStructTree->addField( e_base ,
                               ( "ULONGLONG SizeOfStackReserve" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER64 , SizeOfStackReserve ) ) ,
                               sizeof( peFile.getOptionHeader64( )->SizeOfStackReserve ) ,
                               tr( "SizeOfStackReserve" ) ,
                               (int)peFile.getOptionHeader64( )->SizeOfStackReserve ,
                               optionHeaderFiele
                               );
    }

    if( bIs32PeFile ) {
        mStructTree->addField( e_base ,
                               ( "DWORD SizeOfStackCommit" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER32 , SizeOfStackCommit ) ) ,
                               sizeof( peFile.getOptionHeader32( )->SizeOfStackCommit ) ,
                               tr( "SizeOfStackCommit" ) ,
                               (int)peFile.getOptionHeader32( )->SizeOfStackCommit ,
                               optionHeaderFiele
                               );
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        mStructTree->addField( e_base ,
                               ( "ULONGLONG SizeOfStackCommit" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER64 , SizeOfStackCommit ) ) ,
                               sizeof( peFile.getOptionHeader64( )->SizeOfStackCommit ) ,
                               tr( "SizeOfStackCommit" ) ,
                               (int)peFile.getOptionHeader64( )->SizeOfStackCommit ,
                               optionHeaderFiele
                               );
    }

    if( bIs32PeFile ) {
        mStructTree->addField( e_base ,
                               ( "DWORD SizeOfHeapReserve" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER32 , SizeOfHeapReserve ) ) ,
                               sizeof( peFile.getOptionHeader32( )->SizeOfHeapReserve ) ,
                               tr( "SizeOfHeapReserve" ) ,
                               (int)peFile.getOptionHeader32( )->SizeOfHeapReserve ,
                               optionHeaderFiele
                               );
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {

        mStructTree->addField( e_base ,
                               ( "ULONGLONG SizeOfHeapReserve" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER64 , SizeOfHeapReserve ) ) ,
                               sizeof( peFile.getOptionHeader64( )->SizeOfHeapReserve ) ,
                               tr( "SizeOfHeapReserve" ) ,
                               (int)peFile.getOptionHeader64( )->SizeOfHeapReserve ,
                               optionHeaderFiele
                               );
    }

    if( bIs32PeFile ) {
        mStructTree->addField( e_base ,
                               ( "DWORD SizeOfHeapCommit" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER32 , SizeOfHeapCommit ) ) ,
                               sizeof( peFile.getOptionHeader32( )->SizeOfHeapCommit ) ,
                               tr( "SizeOfHeapCommit" ) ,
                               (int)peFile.getOptionHeader32( )->SizeOfHeapCommit ,
                               optionHeaderFiele
                               );
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        mStructTree->addField( e_base ,
                               ( "ULONGLONG SizeOfHeapCommit" ) ,
                               parentBase + ( fieldOffset( IMAGE_OPTIONAL_HEADER64 , SizeOfHeapCommit ) ) ,
                               sizeof( peFile.getOptionHeader64( )->SizeOfHeapCommit ) ,
                               tr( "SizeOfHeapCommit" ) ,
                               (int)peFile.getOptionHeader64( )->SizeOfHeapCommit ,
                               optionHeaderFiele
                               );
    }


    
    

    mStructTree->addField( e_base ,
                           ( "DWORD LoaderFlags" ) ,
                           parentBase + ( OPTHDROFFSET( LoaderFlags ) ) ,
                           sizeof( peFile.getOptionHeader32( )->LoaderFlags ) ,
                           tr( "LoaderFlags" ) ,
                           (int)peFile.getOptionHeader32( )->LoaderFlags ,
                           optionHeaderFiele
                           );
    mStructTree->addField( e_base ,
                           ( "DWORD NumberOfRvaAndSizes" ) ,
                           parentBase + ( OPTHDROFFSET( NumberOfRvaAndSizes ) ) ,
                           sizeof( peFile.getOptionHeader32( )->NumberOfRvaAndSizes ) ,
                           tr( "NumberOfRvaAndSizes" ) ,
                           (int)peFile.getOptionHeader32( )->NumberOfRvaAndSizes ,
                           optionHeaderFiele
                           );

    parent = mStructTree->addField( e_base ,
                                    ( "IMAGE_DATA_DIRECTORY DataDirectory[]" ) ,
                                    parentBase + ( OPTHDROFFSET( DataDirectory ) ) ,
                                    sizeof( peFile.getOptionHeader32( )->DataDirectory ) ,
                                    tr( "data directory ,like import table , export table" ) ,
                                    0 ,
                                    optionHeaderFiele
                                    );

    
    tableToken.mFont.setItalic( true );

    // 解析数据目录表
    parentBase += ( OPTHDROFFSET( DataDirectory ) );
    auto addDataDir = [ &parentBase , &parent , &bIs32PeFile , &peFile , &tableToken ]( TypeTree* mStructTree , 
                                                                                        const QString& name , 
                                                                                        int index ,
                                                                                        pe_DataDirectroy* pDataDirectory ) {
        Field* table = nullptr;
        table = mStructTree->addField( e_base ,
                                       name ,
                                       parentBase + sizeof( IMAGE_DATA_DIRECTORY ) * index ,
                                       sizeof( IMAGE_DATA_DIRECTORY ) ,
                                       "" ,
                                       0 ,
                                       parent ,
                                       &tableToken
                                       );  
           
        mStructTree->addField( e_base ,
                               ( "VirtualAddress" ) ,
                               parentBase + sizeof( IMAGE_DATA_DIRECTORY ) * index ,
                               sizeof( DWORD ) ,
                               tr( "table VirtualAddress" ) ,
                               (int)pDataDirectory->VirtualAddress ,
                               table ,
                               0 ,
                               peFile.RVAToOfs( pDataDirectory->VirtualAddress ) ,
                               pDataDirectory->Size
                               );

        mStructTree->addField( e_base ,
                               ("Size") ,
                               parentBase + sizeof( IMAGE_DATA_DIRECTORY ) * index + sizeof( DWORD ) ,
                               sizeof( DWORD ) ,
                               tr( "size of table" ) ,
                               (int)pDataDirectory->Size ,
                               table
                               );
    };

    
    pe_DataDirectroy* pDataDirectory;
    if( bIs32PeFile ) {
        pDataDirectory = peFile.getOptionHeader32( )->DataDirectory ;
    }
    else if( peFile.getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        pDataDirectory = peFile.getOptionHeader64( )->DataDirectory;
    }

    for( int i = 0; i < 16; ++i ) {

        addDataDir( mStructTree , name[ i ] , i , pDataDirectory + i );
    }


   // 解析区段表
    pSectionHeader = peFile.getSectionHeader( );
    parentBase = (int)pSectionHeader - (int)peFile.getDosHeader( );

    auto addSection = [ &parentBase , &parent , &peFile , &tableToken ]( TypeTree* mStructTree , pe_SectionHeader* pSec , int nIndex ) {

        Field* misc = nullptr;
        // 添加区段表
        parent = mStructTree->addField( e_struct ,
                                        QString( "Section :%1" ).arg( (char*)pSec->Name ) ,
                                        parentBase + nIndex * IMAGE_SIZEOF_SECTION_HEADER ,
                                        IMAGE_SIZEOF_SECTION_HEADER ,
                                        "" ,
                                        0 , 0 , &tableToken
                                        );

        // 添加区段表字段
        int nBaseOffset = parentBase + nIndex * IMAGE_SIZEOF_SECTION_HEADER;;
        mStructTree->addField( e_base ,
                               ( "BYTE Name[] " ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , Name ) ,
                               sizeof( pSec->Name ) ,
                               tr( "Section name" ),
                               QString( "%1" ).arg( (char*)pSec->Name ) ,
                               parent
                               );

        misc = mStructTree->addField( e_union ,
                                      ( "union Misc" ) ,
                                      nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , Name ) ,
                                      sizeof( pSec->Name ) ,
                                      tr( "size" ) ,
                                      0 ,
                                      parent
                                      );
        mStructTree->addField( e_union ,
                               ( "DWORD PhysicalAddress" ) ,
                               nBaseOffset + sizeof( pSec->Name ) ,
                               sizeof( pSec->Misc.PhysicalAddress ) ,
                               ""  ,
                               (int)pSec->Misc.PhysicalAddress ,
                               misc
                               );

        mStructTree->addField( e_union ,
                               ( "DWORD VirtualSize" ) ,
                               nBaseOffset + sizeof( pSec->Name ) ,
                               sizeof( pSec->Misc.VirtualSize ) ,
                               tr( "size of section data in disk" ) ,
                               (int)pSec->Misc.VirtualSize ,
                               misc
                               );
        mStructTree->addField( e_base,
                               ( "DWORD VirtualAddress" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , VirtualAddress ) ,
                               sizeof( pSec->VirtualAddress ) ,
                               tr( "Section data virtual address" ) ,
                               (int)pSec->VirtualAddress ,
                               parent,0,peFile.RVAToOfs(pSec->VirtualAddress),
                               pSec->SizeOfRawData
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD SizeOfRawData" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , SizeOfRawData ) ,
                               sizeof( pSec->SizeOfRawData ) ,
                               tr( "size of section data(fileAligment)" ) ,
                               (int)pSec->SizeOfRawData ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD PointerToRawData" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , PointerToRawData ) ,
                               sizeof( pSec->PointerToRawData ) ,
                               tr( "section datea address in file(file offset)" ) ,
                               (int)pSec->PointerToRawData ,
                               parent
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD PointerToRelocations" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , PointerToRelocations ) ,
                               sizeof( pSec->PointerToRelocations ) ,
                               tr( "relocation table address(invalid)" ) ,
                               (int)pSec->PointerToRelocations ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( " DWORD PointerToLinenumbers" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , PointerToLinenumbers ) ,
                               sizeof( pSec->PointerToLinenumbers ) ,
                               tr( "line number table address(invalid)" ) ,
                               (int)pSec->PointerToLinenumbers ,
                               parent
                               );
        mStructTree->addField( e_base ,
                               ( "WORD  NumberOfRelocations" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , NumberOfRelocations ) ,
                               sizeof( pSec->NumberOfRelocations ) ,
                               tr( "size of relocation(invalid)" ) ,
                               (int)pSec->NumberOfRelocations ,
                               parent
                               );
        mStructTree->addField( e_base ,
                               ( "WORD  NumberOfLinenumbers" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , NumberOfLinenumbers ) ,
                               sizeof( pSec->NumberOfLinenumbers ) ,
                               tr( "NumberOfLinenumbers(invalid)" ) ,
                               (int)pSec->NumberOfLinenumbers ,
                               parent
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD Characteristics" ) ,
                               nBaseOffset + fieldOffset( IMAGE_SECTION_HEADER , Characteristics ) ,
                               sizeof( pSec->Characteristics ) ,
                               tr( "section Characteristics in memory" ) ,
                               (int)pSec->Characteristics ,
                               parent
                               );
    };
  
    for( int i = 0; i < peFile.getFileHeader( )->NumberOfSections; ++i ) {
        addSection( mStructTree , pSectionHeader + i , i );
    }

    
    sectionDataToken.mFont.setItalic( true );

    // 添加区段数据
    auto addSectionData = [ &parentBase , &sectionDataToken]( TypeTree* mStructTree , pe_SectionHeader* pSec) {
        mStructTree->addField( e_base ,
                               QString( "%1 section Data" ).arg( (char*)pSec->Name ) ,
                               pSec->PointerToRawData,
                               pSec->SizeOfRawData ,
                               tr( "section data" ) ,
                               (int)pSec->SizeOfRawData,
                               0,&sectionDataToken
                               );
    };

    
    for( int i = 0; i < peFile.getFileHeader( )->NumberOfSections; ++i ) {
        sectionDataToken.mFontColor = sectionDatacolor[ i % 5 ];
        addSectionData( mStructTree , pSectionHeader + i);
    }


    mStructTree->repain( );


    // 遍历导出表
    pe_ExportTable* exportTable = peFile.getExportTable( );
    parentBase = (int)exportTable - (int)peFile.getDosHeader( );

    // 解析地址表
    auto addExportTableAddressTable = [ &temp , &peFile ]( TypeTree* mStructTree , int funRva , int offset , int nSize , const QString& name , int nRvaDataSize ) {

        mStructTree->addField( e_base ,
                               name ,
                               offset ,
                               nSize ,
                               "" ,
                               funRva ,
                               temp , 0 ,
                               peFile.RVAToOfs( funRva ) ,
                               nRvaDataSize
                               );
    };

    if( exportTable != nullptr ) {

        parent = mStructTree->addField( e_struct ,
                                        ( "IMAGE_EXPORT_DIRECTORY" ) ,
                                        parentBase ,
                                        sizeof( IMAGE_EXPORT_DIRECTORY ) ,
                                        tr( "import funtion table" ) ,
                                        0 , 0 , &tableToken
                                        );

        mStructTree->addField( e_base ,
                               ( "DWORD Characteristics" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , Characteristics ) ,
                               sizeof( exportTable->Characteristics ) ,
                               tr( "unused" ) ,
                               (int)exportTable->Characteristics ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD TimeDateStamp" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , TimeDateStamp ) ,
                               sizeof( exportTable->TimeDateStamp ) ,
                               tr( "TimeDateStamp" ) ,
                               (int)exportTable->TimeDateStamp ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "WORD MajorVersion" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , MajorVersion ) ,
                               sizeof( exportTable->MajorVersion ) ,
                               tr( "unused" ) ,
                               exportTable->MajorVersion ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "WORD MinorVersion" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , MinorVersion ) ,
                               sizeof( exportTable->MinorVersion ) ,
                               tr( "unused" ) ,
                               exportTable->MinorVersion ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD Name" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , Name ) ,
                               sizeof( exportTable->Name ) ,
                               tr( "Dll name" ) ,
                               (char*)( peFile.RVAToOfs( exportTable->Name ) + (int)peFile.getDosHeader( ) ) ,
                               parent,0,peFile.RVAToOfs(exportTable->Name),
                               sizeof(exportTable->Name)*3
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD Base" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , Base ) ,
                               sizeof( exportTable->Base ) ,
                               tr( "base of export function order" ) ,
                               (int)exportTable->Base ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD NumberOfFunctions" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , NumberOfFunctions ) ,
                               sizeof( exportTable->NumberOfFunctions ) ,
                               tr( "total of export function" ) ,
                               (int)exportTable->NumberOfFunctions ,
                               parent
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD NumberOfNames" ) ,
                               parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , NumberOfNames ) ,
                               sizeof( exportTable->NumberOfNames ) ,
                               tr( "total of function by name export" ) ,
                               (int)exportTable->NumberOfNames ,
                               parent
                               );

        temp = mStructTree->addField( e_base ,
                                      ( "DWORD AddressOfFunctions" ) ,
                                      parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , AddressOfFunctions ) ,
                                      sizeof( exportTable->AddressOfFunctions ) ,
                                      tr( "address of function address table (RVA)" ) ,
                                      (int)exportTable->AddressOfFunctions ,
                                      parent , 0 , peFile.RVAToOfs( exportTable->AddressOfFunctions )
                                      );
        

        // 得到地址表的地址
        addressTableOffset = peFile.RVAToOfs( exportTable->AddressOfFunctions );
        DWORD* pAddressTable = (DWORD*)( addressTableOffset + peFile.fileBase( ) );
        string funcutionName;
        for( DWORD i = 0; i < exportTable->NumberOfFunctions; ++i ) {

            // 找到函数名
            addExportTableAddressTable( mStructTree ,
                                        pAddressTable[ i ] ,
                                        addressTableOffset + i*sizeof( DWORD ) ,
                                        sizeof(DWORD),
                                        tr( "function address(RVA)" ),sizeof(DWORD) );
        }



        temp = mStructTree->addField( e_base ,
                                      ( "DWORD AddressOfNames" ) ,
                                      parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , AddressOfNames ) ,
                                      sizeof( exportTable->AddressOfNames ) ,
                                      tr( "name table address(RVA)" ) ,
                                      (int)exportTable->AddressOfNames ,
                                      parent , 0 , peFile.RVAToOfs( exportTable->AddressOfNames ),
                                      exportTable->NumberOfNames * sizeof(DWORD)
                                      );
        // 解析名称表
        DWORD  nameOffset = peFile.RVAToOfs( exportTable->AddressOfNames );
        DWORD* pNameTable = (DWORD*)( nameOffset + peFile.fileBase( ) );
        for( DWORD i = 0; i < exportTable->NumberOfFunctions; ++i ) {

            if( !peFile.getExportTableFunctionName( i , funcutionName ) ) {
                funcutionName = "--";
            }

            addExportTableAddressTable( mStructTree ,
                                        pNameTable[ i ] ,
                                        nameOffset + i*sizeof( DWORD ) ,
                                        sizeof( DWORD ) ,
                                        funcutionName.c_str( ) ,
                                        funcutionName.size( )
                                        );
        }


        temp = mStructTree->addField( e_base ,
                                      ( "DWORD AddressOfNameOrdinals" ) ,
                                      parentBase + fieldOffset( IMAGE_EXPORT_DIRECTORY , AddressOfNameOrdinals ) ,
                                      sizeof( exportTable->AddressOfNameOrdinals ) ,
                                      tr( "ordinals table address(RVA)" ) ,
                                      (int)exportTable->AddressOfNameOrdinals ,
                                      parent , 0 , 
                                      peFile.RVAToOfs( exportTable->AddressOfNameOrdinals ),
                                      ( exportTable->NumberOfFunctions - exportTable->NumberOfNames )*sizeof(DWORD)
                                      );
        // 解析序号表
        DWORD  orderOffset = peFile.RVAToOfs( exportTable->AddressOfNameOrdinals);
        WORD* pOrderTable = (WORD*)( orderOffset + peFile.fileBase( ) );
        for( DWORD i = 0; i < exportTable->NumberOfFunctions; ++i ) {

            addExportTableAddressTable( mStructTree ,
                                        pOrderTable[ i ] ,
                                        orderOffset + i*sizeof( WORD ) ,
                                        sizeof( WORD ) ,
                                        tr( "Ordinals" ) ,
                                        sizeof( WORD ) );
        }

    }


    mStructTree->repain( );

    // 解析导入表
    pe_ImportTable* importTable = peFile.getImportTable( );
    if( importTable != nullptr ) {

        parentBase = (int)importTable - peFile.fileBase( );

        parent = mStructTree->addField( e_struct ,
                                        ( "IMAGE_IMPORT_DESCRIPTOR" ) ,
                                        parentBase + 0 ,
                                        sizeof( IMAGE_IMPORT_DESCRIPTOR ) ,
                                        tr( "Import table" ) ,
                                        0 , 0 , &tableToken
                                        );

        temp = mStructTree->addField( e_union,
                                      ( "DUMMYUNIONNAME" ) ,
                                      parentBase + 0 ,//偏移
                                      fieldOffset( IMAGE_IMPORT_DESCRIPTOR , TimeDateStamp ) ,//大小
                                      tr( "(INT)" ) ,//注释
                                      0 ,
                                      parent
                                      );
        mStructTree->addField( e_base ,
                               ( "DWORD Characteristics" ) ,
                               parentBase + fieldOffset( IMAGE_IMPORT_DESCRIPTOR , Characteristics ) ,
                               sizeof( importTable->Characteristics ) ,
                               ""  ,
                               (int)importTable->Characteristics ,
                               temp
                               );
        temp = mStructTree->addField( e_base ,
                                      ( "DWORD OriginalFirstThunk" ) ,
                                      parentBase + fieldOffset( IMAGE_IMPORT_DESCRIPTOR , OriginalFirstThunk ) ,
                                      sizeof( importTable->OriginalFirstThunk ) ,
                                      tr( "INT address(RVA)" ) ,
                                      (int)importTable->Characteristics ,
                                      temp ,
                                      0 ,
                                      peFile.RVAToOfs( importTable->Characteristics ) ,
                                      peFile.getImportIATSize( )
                                      );

        // 解析INT
        IMAGE_THUNK_DATA* pINT = (IMAGE_THUNK_DATA*)( peFile.RVAToOfs( importTable->FirstThunk ) + peFile.fileBase( ) );
      
        DWORD dwINTOffset = (int)pINT - peFile.fileBase( );
       

        auto addINT = [ &temp , &peFile ]( TypeTree* mStructTree , int rva , int offset , int nSize , const QString& name ) {

            Field* temp2 = mStructTree->addField( e_union,
                                                  name ,
                                                  offset ,
                                                  nSize ,
                                                  "" ,
                                                  0 ,
                                                  temp);
            
            // 添加IMAGE_THUNK_DATA
            temp2 = mStructTree->addField( e_base ,
                                           ( "AddressOfData" ) ,
                                           offset ,
                                           sizeof( DWORD ) ,
                                           tr( "Ordinal or IMAGE_IMPORT_BY_NAME struct RVA" ) ,
                                           rva ,
                                           temp2 ,
                                           0 ,
                                           peFile.RVAToOfs( rva ) ,
                                           sizeof( IMAGE_IMPORT_BY_NAME )
                                           );

            if( !(rva & 0x80000000) ) {

                rva &= 0x7FFFFFFF;
                IMAGE_IMPORT_BY_NAME* pIIBN = (IMAGE_IMPORT_BY_NAME*)( peFile.RVAToOfs( rva ) + peFile.fileBase( ) );

                temp2 = mStructTree->addField( e_base ,
                                               ( "IMAGE_IMPORT_BY_NAME" ) ,
                                               peFile.RVAToOfs( rva & 0x7FFFFFFF ) ,
                                               strlen( (char*)pIIBN->Name ) + sizeof( IMAGE_IMPORT_BY_NAME ) - 1 ,
                                               tr( "Name struct rva" ) ,
                                               0 ,
                                               temp2 
                                               );

                mStructTree->addField( e_base ,
                                       ( "WORD Hint" ) ,
                                       peFile.RVAToOfs( rva & 0x7FFFFFFF ) ,
                                       sizeof( WORD ) ,
                                       "" ,
                                       pIIBN->Hint ,
                                       temp2
                                       );
                mStructTree->addField( e_base ,
                                       ( "CHAR   Name[1]" ) ,
                                       peFile.RVAToOfs( rva & 0x7FFFFFFF ) + sizeof( WORD ) ,
                                       strlen( (char*)pIIBN->Name ) ,
                                       tr( "name" ) ,
                                       (char*)pIIBN->Name,
                                       temp2
                                       );
            }
        };

        while( *(PDWORD)pINT ) {

            addINT( mStructTree ,
                    pINT->u1.AddressOfData ,
                    dwINTOffset ,
                    sizeof( pINT->u1.AddressOfData ) ,
                    tr( "IMAGE_THUNK_DATA" ) 
                    );
           
            dwINTOffset += sizeof( pINT->u1.AddressOfData );
            ++pINT;
        }

        

        mStructTree->addField( e_base ,
                               ( " DWORD TimeDateStamp;" ) ,
                               parentBase + fieldOffset( IMAGE_IMPORT_DESCRIPTOR , TimeDateStamp ) ,
                               sizeof( importTable->TimeDateStamp ) ,
                               tr( "TimeDateStamp" ) ,
                               (int)importTable->TimeDateStamp ,
                               parent
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD ForwarderChain" ) ,
                               parentBase + fieldOffset( IMAGE_IMPORT_DESCRIPTOR , ForwarderChain ) ,
                               sizeof( importTable->ForwarderChain ) ,
                               tr( "ForwarderChain" ) ,
                               (int)importTable->ForwarderChain ,
                               parent
                               );

        pDllName = (char*)( peFile.RVAToOfs( importTable->Name ) + peFile );
        mStructTree->addField( e_base ,
                               ( "DWORD Name" ) ,
                               parentBase + fieldOffset( IMAGE_IMPORT_DESCRIPTOR , Name ) ,
                               sizeof( importTable->Name ) ,
                               tr( "import DLL Name" ) ,
                               pDllName ,
                               parent ,
                               0 ,
                               peFile.RVAToOfs( importTable->Name ) ,
                               strlen( pDllName )
                               );

        temp = mStructTree->addField( e_base ,
                                      ( "DWORD FirstThunk" ) ,
                                      parentBase + fieldOffset( IMAGE_IMPORT_DESCRIPTOR , FirstThunk ) ,
                                      sizeof( importTable->FirstThunk ) ,
                                      tr( "IAT address(RVA)" ) ,
                                      (int)importTable->FirstThunk ,
                                      parent ,
                                      0 ,
                                      peFile.RVAToOfs( importTable->FirstThunk ) ,
                                      peFile.getImportIATSize( )
                                      );
        // 遍历IAT

        DWORD dwIATOffset = peFile.RVAToOfs( importTable->FirstThunk );
        IMAGE_THUNK_DATA32* pIAT = (IMAGE_THUNK_DATA32*)( dwIATOffset + peFile.fileBase( ) );
        
        while( *(PDWORD)pIAT ) {

            addINT( mStructTree ,
                    pIAT->u1.AddressOfData ,
                    dwIATOffset ,
                    sizeof( pIAT->u1.AddressOfData ) ,
                    tr( "IMAGE_THUNK_DATA" ) 
                    );

            dwIATOffset += sizeof( pIAT->u1.AddressOfData );
            ++pIAT;
        }
    }

    // 解析重定位表
    pe_Relacation* pRecation = peFile.getRelacationBlack( );
    parentBase = (DWORD)pRecation - peFile;

    auto addRelactionBlack = [ &parent , &peFile , &bIs32PeFile ]( TypeTree* mStructTree , int offset , pe_Relacation *pRelTab ) {

        Field* temp2 = mStructTree->addField( e_struct ,
                                              ( "IMAGE_BASE_RELOCATION" ) ,
                                              offset ,
                                              pRelTab->SizeOfBlock ,
                                              tr( "recation black" ) ,
                                              0 ,
                                              parent
                                              );

        mStructTree->addField( e_base ,
                               ( "DWORD VirtualAddress" ) ,
                               offset + fieldOffset( IMAGE_BASE_RELOCATION , VirtualAddress ) ,
                               sizeof( pRelTab->VirtualAddress ) ,
                               tr( "recation black start RVA" ) ,
                               (int)pRelTab->VirtualAddress ,
                               temp2 ,
                               NULL ,
                               peFile.RVAToOfs( pRelTab->VirtualAddress ) ,
                               sizeof( pRelTab->VirtualAddress )
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD SizeOfBlock" ) ,
                               offset + fieldOffset( IMAGE_BASE_RELOCATION , SizeOfBlock ) ,
                               sizeof( pRelTab->SizeOfBlock ) ,
                               tr( "all the block size" ) ,
                               (int)pRelTab->SizeOfBlock ,
                               temp2 
                               );

       
        // 解析Block
        Typeoffset* pOffset = (Typeoffset*)( pRelTab + 1 );
        DWORD       dwSize = pRelTab->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION );
        mStructTree->addField( e_base ,
                               ( "WORD TypeOffset[]" ) ,
                               (int)pOffset - peFile ,
                               dwSize ,
                               tr( "all the block size" ) ,
                               (int)pRelTab->SizeOfBlock ,
                               temp2 ,
                               0 ,
                               (int)pOffset - peFile ,
                               dwSize ,
                               pRelTab->VirtualAddress
                               );
        

    };

    if( pRecation != NULL ) {

        parent = mStructTree->addField( e_struct ,
                                        ( "Relacation table" ) ,
                                        parentBase ,
                                        peFile.getDataDirectory( )[ 5 ].Size ,
                                        tr( "IMAGE_BASE_RELOCATION Array" ) ,
                                        0 ,
                                        NULL ,
                                        &tableToken
                                        );
        while( pRecation->SizeOfBlock != 0 ) {

            addRelactionBlack( mStructTree , (DWORD)pRecation - peFile , pRecation );

            pRecation = (pe_Relacation*)( (DWORD)pRecation + pRecation->SizeOfBlock );
        }
    }
        
    
    // 解析资源表

    // 资源目录
    pe_ResourcesDirectory   *pResDirFirst , *pResDirSecond , *pResDirThird;

    // 资源目录入口
    pe_ResourcesDirectoryEntry  *pResDirEntryFirst , *pResDirEntrySecond , *pResDirEntryThrid;

    // 资源数据入口
    pe_ResourcesDataEntry   *pResDataEntry;

    // 添加资源目录
    auto addResourcesDirectory = [ &peFile ]( TypeTree* mStructTree , 
                                              pe_ResourcesDirectory* pDir,
                                              int offset , 
                                              const QString& comment ,
                                              Field* parent ) -> Field* // 返回节点
    {

        Field* temp = mStructTree->addField( e_struct ,
                                             ( "IMAGE_RESOURCE_DIRECTORY" ) ,
                                             offset ,
                                             sizeof( IMAGE_RESOURCE_DIRECTORY ) ,
                                             comment ,
                                             0 ,
                                             parent
                                             );
        mStructTree->addField( e_base,
                               ( "DWORD Characteristics" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DIRECTORY , Characteristics ) ,
                               sizeof( pDir->Characteristics ) ,
                               tr("invalid"),
                               (int)pDir->Characteristics,
                               temp
                               );


        mStructTree->addField( e_base ,
                               ( "DWORD TimeDateStamp" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DIRECTORY , TimeDateStamp ) ,
                               sizeof( pDir->TimeDateStamp ) ,
                               tr( "invalid" ) ,
                               (int)pDir->TimeDateStamp ,
                               temp
                               );
        mStructTree->addField( e_base ,
                               ( "WORD MajorVersion" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DIRECTORY , MajorVersion ) ,
                               sizeof( pDir->MajorVersion ) ,
                               tr( "invalid" ) ,
                               (int)pDir->MajorVersion ,
                               temp
                               );
        mStructTree->addField( e_base ,
                               ( "WORD MinorVersion" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DIRECTORY , MinorVersion ) ,
                               sizeof( pDir->MinorVersion ) ,
                               tr( "invalid" ) ,
                               (int)pDir->MinorVersion ,
                               temp
                               );
        mStructTree->addField( e_base ,
                               ( "WORD NumberOfNamedEntries" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DIRECTORY , NumberOfNamedEntries ) ,
                               sizeof( pDir->NumberOfNamedEntries ) ,
                               tr( "Number Of Named Entries" ) ,
                               (int)pDir->NumberOfNamedEntries ,
                               temp
                               );
        mStructTree->addField( e_base ,
                               ( "WORD NumberOfIdEntries" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DIRECTORY , NumberOfIdEntries ) ,
                               sizeof( pDir->NumberOfIdEntries ) ,
                               tr( "Number Of Id Entries" ) ,
                               (int)pDir->NumberOfIdEntries ,
                               temp
                               );
        return temp;
    };

    // 添加资源入口
    auto addResourcesDirectoryEntry = [ &peFile,&pResDirFirst ]( TypeTree* mStructTree , 
                                                   pe_ResourcesDirectoryEntry* pEntry , 
                                                   int offset , 
                                                   QString comment ,
                                                   Field* parent,
                                                   int entryType)->Field* //entryType-> 0:none,1:Type,2:id
    {
        wchar_t buff[ 120 ] ;
        // 解析出资源的类型
        if( pEntry->NameIsString ) { // 资源类型是自定义类型(是字符串)

            PIMAGE_RESOURCE_DIR_STRING_U pString = 0;
            pString = (PIMAGE_RESOURCE_DIR_STRING_U)(pEntry->NameOffset + (DWORD)pResDirFirst);
            wcsncpy( buff , pString->NameString , pString->Length );
            buff[ pString->Length ] = 0;
            if( entryType == 1 ) { // 资源类型
                comment = QString( "Type:<%1>" ).arg( QString::fromUtf16( reinterpret_cast<const unsigned short *>( buff ) ) );
            }
            else if( entryType == 2 ) { // 资源ID
                comment = QString( "ID:<%1>" ).arg( QString::fromUtf16( reinterpret_cast<const unsigned short *>( buff ) ) );
            }
        }
        else { // 资源类型是标准类型或整型ID

            static QString type[ ] =
            {
                tr( "" ) ,				// 0
                tr( "Cursor" ) ,				// 1 
                tr( "Bitmap" ) ,				// 2 RT_BITMAP
                tr( "ICON" ) ,			    // 3 RT_ICON
                tr( "Menu" ) ,				// 4 RT_MENU
                tr( "Dialog" ) ,			// 5 RT_DIALOG      
                tr( "String" ) ,			// 6 RT_STRING      
                tr( "FontDir" ) ,			// 7 RT_FONTDIR     
                tr( "Font" ) ,				// 8 RT_FONT        
                tr( "Accelerator" ) ,			// 9 RT_ACCELERATOR 
                tr( "RCData" ) ,		// 10 RT_RCDATA      
                tr( "MessageTable" ) ,			// 11 RT_MESSAGETABLE
                tr( "CurSorGroup" ) ,			// 12 
                tr( "" ) ,				// 13 
                tr( "ICON Group" ) ,			// 14 
                "" ,								// 15
                tr( "Version" ) ,			// 16
                tr( "Dialog Include" ) ,	// 17 #define RT_DLGINCLUDE   MAKEINTRESOURCE(17)
                "" ,								// 18 #define RT_PLUGPLAY     MAKEINTRESOURCE(19)
                "" ,								// 19 #define RT_VXD          MAKEINTRESOURCE(20)
                "" ,								// 20 #define RT_ANICURSOR    MAKEINTRESOURCE(21)
                "" ,								// 21 #define RT_ANIICON      MAKEINTRESOURCE(22)
                "" ,									// 22 
                tr( "html" ) ,						    // 23 #define RT_HTML         MAKEINTRESOURCE(23)
                tr( "ManifestFile" )			// 24 RT_MANIFEST
            };

            if( entryType == 1 ) {
                comment = QString( "Type:[%1]" ).arg( type[ pEntry->Id ] );
            }
            else if( entryType == 2 ) {
                comment = QString( "ID:[%1]" ).arg( pEntry->Id );
            }
        }



        // 解析出资源的ID
        Field* temp = mStructTree->addField( e_struct ,
                                             ( "IMAGE_RESOURCE_DIRECTORY_ENTRY" ) ,
                                             offset ,
                                             sizeof( IMAGE_RESOURCE_DIRECTORY_ENTRY ) ,
                                             comment ,
                                             0 ,
                                             parent
                                             );

        Field* union1 = mStructTree->addField( e_union ,
                                               ( "unio NameOrId" ) ,
                                               offset ,
                                               sizeof( DWORD ) ,
                                               tr("name or id information") ,
                                               0 ,
                                               temp
                                               );
        Field* struct1 = mStructTree->addField( e_struct ,
                                                ( "struct NameOffset" ) ,
                                                offset ,
                                                sizeof( DWORD ) ,
                                                tr( "name offset" ) ,
                                                0 ,
                                                union1
                                                );
        mStructTree->addField( e_base ,
                               ( "DWORD NameOffset : 31" ) ,
                               offset ,
                               sizeof( DWORD ) ,
                               tr( "name offset" ) ,
                               (int)pEntry->NameOffset ,
                               struct1
                               );
        mStructTree->addField( e_base,
                               ( "DWORD NameIsString : 1" ) ,
                               offset ,
                               sizeof( DWORD ) ,
                               tr( "1 -> NameOffset is valid, otherwise Nameoffset is invalid" ) ,
                               (int)pEntry->NameIsString ,
                               struct1
                               );

        //-------------------------------------------------------------//

        union1 = mStructTree->addField( e_union ,
                                        ( "union Directory Or Data" ) ,
                                        offset + sizeof( DWORD ) ,
                                        sizeof( DWORD ) ,
                                        tr( "offset , maybe offset to Directory, maybe offset to Data" ) ,
                                        0 ,
                                        temp
                                        );
        mStructTree->addField( e_base ,
                               ( "DWORD OffsetToData" ) ,
                               offset ,
                               sizeof( DWORD ) ,
                               "" ,
                               (int)pEntry->OffsetToData ,
                               union1
                               );

        struct1 = mStructTree->addField( e_struct ,
                                         ( "struct OffsetToDirectory" ) ,
                                         offset ,
                                         sizeof( DWORD ) ,
                                         tr( "if DataIsDirectory is 1,then OffsetToDirectory is invalid" ) ,
                                         0 ,
                                         union1
                                         );
        mStructTree->addField( e_base ,
                               ( "DWORD OffsetToDirectory : 31" ) ,
                               offset ,
                               sizeof( DWORD ) ,
                               "" ,
                               (int)pEntry->OffsetToDirectory ,
                               struct1
                               );

        mStructTree->addField( e_base ,
                               ( "DWORD DataIsDirectory : 1" ) ,
                               offset ,
                               sizeof( DWORD ) ,
                               "" ,
                               (int)pEntry->DataIsDirectory ,
                               struct1
                               );
        return temp;
    };

    auto addResourcesData = [ &peFile ]( TypeTree* mStructTree ,
                                         pe_ResourcesDataEntry* pEntry ,
                                         int offset ,
                                         const QString& comment ,
                                         Field* parent ) {

        Field* temp = mStructTree->addField( e_struct ,
                                             ( "IMAGE_RESOURCE_DATA_ENTRY" ) ,
                                             offset ,
                                             sizeof( IMAGE_RESOURCE_DATA_ENTRY ) ,
                                             comment ,
                                             0 ,
                                             parent );
        mStructTree->addField( e_base ,
                               ( "DWORD OffsetToData" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DATA_ENTRY , OffsetToData ) ,
                               sizeof( pEntry->OffsetToData ) ,
                               tr( "resources data address(RVA)" ) ,
                               (int)pEntry->OffsetToData ,
                               temp ,
                               0 ,
                               peFile.RVAToOfs( pEntry->OffsetToData ) ,
                               pEntry->Size
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD Size" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DATA_ENTRY , Size ) ,
                               sizeof( pEntry->Size ) ,
                               tr( "resources size" ) ,
                               (int)pEntry->Size ,
                               temp
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD CodePage" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DATA_ENTRY , CodePage ) ,
                               sizeof( pEntry->CodePage ) ,
                               0 ,
                               (int)pEntry->CodePage ,
                               temp
                               );
        mStructTree->addField( e_base ,
                               ( "DWORD Reserved" ) ,
                               offset + fieldOffset( IMAGE_RESOURCE_DATA_ENTRY , Reserved ) ,
                               sizeof( pEntry->Reserved ) ,
                               0 ,
                               (int)pEntry->Reserved ,
                               temp
                               );
    };
    // 添加资源数据
    
    pResDirFirst = peFile.getResourcesTable( );

    Field* dirParent0 , *dirParent1 , *dirParent2 , *dirParent3 , *dirParent4 , *dirParent5;
    if( pResDirFirst != NULL ) {

        
        dirParent0 = addResourcesDirectory( mStructTree ,
                                            pResDirFirst ,
                                            (int)pResDirFirst - peFile ,
                                            tr( "First Directroy,Root dirctory" ) ,
                                            NULL );

        // 遍历第一层资源目录入口
        pResDirEntryFirst = (pe_ResourcesDirectoryEntry*)( pResDirFirst + 1 );
        for( int i = 0; i < pResDirFirst->NumberOfIdEntries + pResDirFirst->NumberOfNamedEntries; ++i ) {

            dirParent1 = addResourcesDirectoryEntry( mStructTree ,
                                                     pResDirEntryFirst ,
                                                     (int)pResDirEntryFirst - peFile ,
                                                     tr( "First Entry,save some resource type" ) ,
                                                     dirParent0,
                                                     1
                                                     );

            if( pResDirEntryFirst->DataIsDirectory ) {

                // 获取第二层资源的目录
                pResDirSecond = (pe_ResourcesDirectory *)( pResDirEntryFirst->OffsetToDirectory + (DWORD)pResDirFirst );
                pResDirEntrySecond = (pe_ResourcesDirectoryEntry*)( pResDirSecond + 1 );

                dirParent2 = addResourcesDirectory( mStructTree ,
                                                    pResDirSecond ,
                                                    (int)pResDirSecond - peFile ,
                                                    tr( "Second Directroy,same resources type directory" ) ,
                                                    dirParent1 );

                // 遍历第二层资源目录入口
                for( int j = 0 ; j < pResDirSecond->NumberOfIdEntries+pResDirSecond->NumberOfNamedEntries; j++ ) {

                    dirParent3 = addResourcesDirectoryEntry( mStructTree ,
                                                             pResDirEntrySecond ,
                                                             (int)pResDirEntrySecond - peFile ,
                                                             tr( "Secound Entry,resources id" ) ,
                                                             dirParent2 ,
                                                             2
                                                             );


                    if( pResDirEntrySecond->DataIsDirectory ) {

                        // 获取第三层资源的目录
                        pResDirThird = (pe_ResourcesDirectory*)( pResDirEntrySecond->OffsetToDirectory + (DWORD)pResDirFirst );

                        dirParent4 = addResourcesDirectory( mStructTree ,
                                                            pResDirThird ,
                                                            (int)pResDirThird - peFile ,
                                                            tr( "Thrid Directroy,keep architecture" ) ,
                                                            dirParent3 );


                        pResDirEntryThrid = (pe_ResourcesDirectoryEntry*)( pResDirThird + 1 );

                        dirParent5 = addResourcesDirectoryEntry( mStructTree ,
                                                                 pResDirEntryThrid ,
                                                                 (int)pResDirEntryThrid - peFile ,
                                                                 tr( "Thrid Entry,save resources data entry" ) ,
                                                                 dirParent4 ,
                                                                 0
                                                                 );


                        if( !pResDirEntryThrid->DataIsDirectory ) {

                            // 获取数据目录入口
                            pResDataEntry = (pe_ResDataEntry*)( pResDirEntryThrid->OffsetToData + (DWORD)pResDirFirst );
                            addResourcesData( mStructTree ,
                                              pResDataEntry ,
                                              (int)pResDataEntry - peFile ,
                                              tr( "Data Entry, save resources information" ) ,
                                              dirParent5
                                              );
                        }
                    }


                    ++pResDirEntrySecond;
                }


            }
            ++pResDirEntryFirst;
        }

    }

}

void MainWindow::onEditHexTextColumn( int nLine , int nRow )
{
    QString buff;
    buff.sprintf("line:%d,row:%d",nLine,nRow);
    mLinePosition->setText(buff);
}

void MainWindow::onEditStringColumn( int nLine , int nRow )
{
    QString buff;
    buff.sprintf("line:%d,row:%d",nLine,nRow);
    mLinePosition->setText(buff);
}


