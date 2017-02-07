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
    e_base ,  // ������������
    e_struct, // �ṹ��
    e_union , //������
}FieldType;



class Field
{
public:
    FieldType           type;
    Field*              parent;   // ���ڵ�,�ṹ���������ʱӵ��

    QString             name;   //����
    QString             comment;//ע��
    QTreeWidgetItem*    item;   // TreeWidget����ʹ�õ��Ľڵ�
    TokenList::Token    token;  // HexTextEditor����ʹ�õ�����ɫ����

    // ��Field��һ�����ڵ�ʱ, ���µ�ֵ��ʹ��.
    QVariant            value; // �ֶε�ֵ

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


// �ṹ��
// ���ṹ����뵽����
class TypeTree
{
public:
    typedef struct USERDATAONITEM
    {
        int type;//0:��ͨ, 1:��Դ,0x200����ʱ:չ���ض�λ��typeoffset
        int rva;
        int size;
    }USERDATAONITEM;


public:
    TypeTree( QTreeWidget* tree, HexTextEditor* edit);
    ~TypeTree( );

    Field*   addField( FieldType type ,
                       const QString& name , // �ֶ���
                       int nIndex ,          // �ֶ��ڻ������ڵ�ƫ��
                       int nSize ,           // �ֶ�ռ���ֽ���
                       const QString& comment , // �ֶε�ע��
                       const QVariant& value ,    // �ֶε�ֵ
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

