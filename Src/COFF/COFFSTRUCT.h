#pragma once



typedef	unsigned int	uint,*puint , dword , *pdword;
typedef unsigned char	byte,*pbyte;
typedef unsigned short	ushort,*pushort , word ,*pword;

// wFlags:
// 0x0001 	F_RELFLG 
// 无重定位信息标记.这个标记指出COFF文件中没有重定位信息.通常在目标文件中这个标记们为0,在可执行文件中为1  
// 0x0002 	F_EXEC 
// 可执行标记。这个标记指出 COFF 文件中所有符号已经解析， COFF 文件应该被认为是可执行文件。  
// 0x0004 	F_LNNO 
// 文件中所有行号已经被去掉。  0x0008 F_LSYMS 文件中的符号信息已经被去掉。  
// 0x0100 	F_AR32WR 
// 标记指出文件是32位的小尾端存储方式 COFF文件。
#pragma pack(1)
typedef struct COFF_HEADER
{
    WORD    wMacine;    /* 平台名 */
    WORD    numberOfSection;   /* 区段个数 */
	DWORD	uTimeDataStamp;   /* 时间戳 */
    DWORD	pointerToSymbol;   /* 符号表的文件偏移 */
    DWORD	numberOfSymbol;    /* 符号表的个数 */
	WORD    wOptHeaSize;   /* 扩展头大小 */
    WORD    wFlags;    /* 属性 */
}COFF_HEADER,*PCOFF_HEADER;



// uFlags:
// 值名称说明
// 0x0020	STYP_TEXT
// 正文段标识，说明该段是代码。
// 0x0040	STYP_DATA
// 数据段标识，有些标识的段将用来保存已初始化数据。
// 0x0080	STYP_BSS
// 有这个标识段也是用来保存数据，不过这里的数据是未初始化数据。
// 注意，在BSS段中，ulVSize、ulVAddr、ulSize、ulSecOffset、ulRelOffset、ulLNOffset、ulNumRel、ulNumLN的值都为0。
// （上表只是部分值，其它值在PE格式中介绍，后同）
#pragma pack(1)
typedef struct COFF_SEC_HEA
{                     /* modified COFF*/
    char    szName[ 8 ];      /* section name */
    DWORD   virtualSize;        /* physical address */
    DWORD   virtualAddress;          /* virtual address */
    DWORD   sizeofRawData;         /* size of section */
    DWORD   pointerToRawData;       /* fileptr to raw data*/
    DWORD   pointerToRelaction;       /* fileptr to reloc */
    DWORD   pointerToLinenumber;      /* fileptr to lineno */
    WORD    numberOfRelcation;       /* reloc count */
    WORD    numberOfLinenumber;        /* line number count */
    DWORD   uFlags;        /* flags */
}COFF_SEC_HEA , *PCOFF_SEC_HEA;



#define REL_I386_DIR32  0x06 // 32位寻址的直接地址 
#define REL_I386_REL32	0x14 // 32位寻址的相对地址

#pragma pack(1)
typedef struct COFF_REL
{
    DWORD	address;		// 定位偏移
    DWORD	symbolIndex;	// 重定位的符号
	WORD	wType;		// 定位类型
} COFF_REL,*PCOFF_REL;




#define SIZEOFREL		10
#define NEXTRELOC(pReloc) (PCOFF_REL)(((uint)(pReloc))+10)


#define SYM_CLASS_EXTERNAL	0x02
#define SYM_CLASS_STATIC	0x03
#pragma pack(1)
typedef struct
{
	union
	{
		char cName[8];				// 符号名称
		struct
		{
			DWORD Zero;	    // 字符串表标识
            DWORD Offset;	// 字符串偏移
		} s;
	} e;
    DWORD   value;				// 符号值
	short   iSection;				// 符号所在段
    WORD    Type;				// 符号类型
	BYTE    Cass;				// 符号存储类型
    BYTE    numberOfAnnex;				// 符号附加记录数
} COFF_SYMBOL , *PCOFF_SYMBOL;

#define SIZEOFSYMBOL 18
#define NEXTSYMBOL(pSymbol)  (PCOFF_SYML)(((DWORD)(pSymbol)) + SIZEOFSYMBOL)


#pragma pack(1)
#define  OFFSETTOSTRING(CoffHeader) (COFF_STRING_TABLE*)((CoffHeader)->pointerToSymbol + SIZEOFSYMBOL * (CoffHeader)->numberOfSymbol + (int)CoffHeader)
typedef struct
{
    DWORD   dwSize;
    char    string[ 1 ];
}COFF_STRING_TABLE;


const char* getSymbolType( const COFF_HEADER* pHead , int nIndex )
{
    COFF_SYMBOL *pSymbol = (COFF_SYMBOL*)( pHead->pointerToSymbol + (int)pHead );

    if( pSymbol[ nIndex ].Cass == SYM_CLASS_EXTERNAL )
        return "Extern Symbol";
    else if( pSymbol[ nIndex ].Cass == SYM_CLASS_EXTERNAL )
        return "Local Symbol";
    return "";
}

const char*  getSymbolName(const COFF_HEADER* pHead, DWORD nIndex ) 
{
    if( nIndex >= pHead->numberOfSymbol )
        return nullptr;

    COFF_SYMBOL *pSymbol = (COFF_SYMBOL*)( pHead->pointerToSymbol + (int)pHead );
    COFF_STRING_TABLE* pString = OFFSETTOSTRING( pHead );
    int n = sizeof( COFF_SYMBOL );
    n = SIZEOFSYMBOL;


    char* pName = nullptr;

    // 获取符号名
    if( pSymbol[nIndex].e.s.Zero == 0 ) {
        pName = &pString->string[ pSymbol[ nIndex ].e.s.Offset ];
    }
    else {
        pName = pSymbol[ nIndex ].e.cName;
    }

    return pName;

}