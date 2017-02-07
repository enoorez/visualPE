#pragma once
#include "qstring.h"
#include "qcolor.h"
#include "Src\HexTextEditor\tokenlist.h"
#include "qvariant.h"

class QTreeWidget;
class HexTextEditor;
class QTreeWidgetItem;



typedef enum FieldType
{
    e_base ,  // 基本数据类型
    e_struct, // 结构体
    e_union , //联合体
}FieldType;



class Field
{
public:
    FieldType           type;
    Field*              parent;   // 父节点,结构体和联合体时拥有

    QString             name;   //名称
    QString             comment;//注释
    QTreeWidgetItem*    item;   // TreeWidget部件使用到的节点
    TokenList::Token    token;  // HexTextEditor部件使用到的着色规则

    // 当Field是一个父节点时, 以下的值不使用.
    QVariant            value; // 字段的值

public:
    Field( FieldType    type ,
           const QString& name ,
           const QString& comment ,
           const QVariant& value ,
           TokenList::Token&token ,
           QTreeWidgetItem* item = nullptr ,
           Field* parent = nullptr );

    ~Field( );


    Field* createField( FieldType type ,
                        const QString& name ,
                        const QString& comment ,
                        const QVariant& value ,
                        TokenList::Token&token ,
                        QTreeWidgetItem* item = nullptr ,
                        Field* parent = nullptr );

    void    itemOnTextEditRangle(int& index,int& nLen );
};


// 结构树
// 将结构体插入到树中
class TypeTree
{
public:
    typedef struct USERDATAONITEM
    {
        int type;//0:普通, 1:资源,0x200以上时:展开重定位的typeoffset
        int rva;
        int size;
    }USERDATAONITEM;


public:
    TypeTree( QTreeWidget* tree, HexTextEditor* edit);
    ~TypeTree( );

    Field*   addField( FieldType type ,
                       const QString& name , // 字段名
                       int nIndex ,          // 字段在缓冲区内的偏移
                       int nSize ,           // 字段占的字节数
                       const QString& comment , // 字段的注释
                       const QVariant& value ,    // 字段的值
                       Field* parent = nullptr ,
                       TokenList::Token* pToken = nullptr,
                       int userData = -1 ,
                       int userDataSize = 1,
                       int userDataType = 0
                       );

    Field*  findType( FieldType type , const QString& name );
    Field*  findToken( int line , int row );

    void    selectOnHexEditor( const Field* field );
    void    selectOnTreeWidget( const Field* field );

    void    clear( );
    void    repain( );
protected:
    QList<Field*>           mFieldList;
    QList<USERDATAONITEM*>  mUserDataList;

    QTreeWidget*            mTree;
    HexTextEditor*          mEdit;
};

