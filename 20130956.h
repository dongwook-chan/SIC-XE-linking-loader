#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#include <dirent.h>
#include <sys/stat.h>



// 1주차

/* ========== 명령문 저장 ========== */
#define MAXCMD 80	// 명령문 최대 길이

enum INITBUF { NOINIT, INIT, FREE };	// initBufBase의 인자

// 명령문 저장용 구조체
typedef struct _cmdNode {
	char *cmd;              // 명령문

	struct _cmdNode *link;  // 다음 구조체
}cmdNode;

cmdNode *showRootCmd(void);                             // 명령문 linkedlist의 첫 노드 주소값을 반환한다.
cmdNode *modCurCmd(cmdNode *newCmd);                    // 현재 linkedlist상 노드 주소값을 반환한다.
char *initBufBase(short int mode);						// buffer의 초기화/주소값 반환
char *modCurBuf(char *newBuf);							// 현재 버퍼의 문자를 가르키는 주소값(커서) 반환/수정
void createNewCmd(cmdNode **newcmd, char *buf);         // 명령문 linkedlist에 새로운 노드를 생성
cmdNode *addCmd(char *buf);                             // 생성된 노드를 linkedlist에 추가
void freeCmds();                                        // 명령문 저장에 사용된 모든 동적 할당을 해제한다.


/* ========== 명령어 추출 및 처리 ========== */
#define EOB '\n'	// End Of Buffer
#define MAXCMDWORD 10	// 최대 명령어 길이(opcodelist)
#define MAXADDROP 4	// 최대 주소 길이(FFFF)
#define MAXVALOP 2	// 최대값(FF)
#define MAXHEX 8	// unsinged long int로 받을 수 있는 hex는 최대 8자리

enum CMDSTAT{NOCHANGE, VALID, INVALID, QUIT};	// modCmdStatus의 인자
enum ORDINAL{FIRST, SECOND, LAST, VAR};	// getOperand의 인자
enum CMDERR{OPERAND, CMDWORD, EXCESS, ADDRBOUND, ADDRORDER, MNEMONIC, NO_OP, EXTENSION, NEGATIVE_OPERAND, WRONG_EXTENSION};	// cmdError의 인자

void getCmdWord(char *cmdWord);	// 명령문에서 명령어 추출
short int modCmdStatus(short int newStatus);	// 현재 명령문의 상태 반환/수정
short int cmdError(short int errCode);	// 에러 코드에 맞는 예외처리
short int isCmdEnd(void);	// 명령문(cmd)의 끝(end)에 도달했는지 확인
unsigned long int getOperand(short int ordinal, short int maxLen, short int allowNeg);
char *getMnemonic(void);	// 명령문에서 mnemonic 인자를 추출한다.
short int procCmdWord(char *cmdWord);	// 명령어(cmdWord)에 따른 처리(proc)


/* ========== 1. Shell 관련 명령어 ========== */
void help();	// 명령어 리스트 화면에 출력
void dir(void);	// 현재 디렉토리 내 오브젝트 목록 출력
void history(void);	// linked list에 저장된 명령어 목록 출력


/* ========== 2. 메모리 관련 명령어 ========== */
#define MEMSIZE 16*65536	// 메모리 사이즈

unsigned char *memBase(void);	// 메모리 초기화 및 주소 반환
int modLastAddr(int newAddr);	// dump된 마지막 주소를 반환/수정
void dump(int start, int end);	// 메모리 내용 출력
void edit(int addr, int value);	// 주어진 주소인자의 값을 값인자로 갱신
void fill(int start, int end, short int value);	// 메모리의 start부터 end까지를 value로 갱신
void reset(void);	// 메모리 전체를 0으로 변경


/* ========== 3. OPCODE TABLE 관련 명령어 ========== */
#define HASHSIZE 20	// 해시 테이블 사이즈
#define MAXOPCODE 2	// opcode 최대 길이
#define MAXMNEMONIC 6	// mnemoninc 최대 길이

// opcode를 저장할 구조체
typedef struct _opcodeNode {
	char *code;	// opcode
	char *mnemonic;	// mnemonic
	short int format;	// 형식

	struct _opcodeNode *link;	// 다음 opcode
}opcodeNode;

opcodeNode **hashTable(void);	// 해시 테이블 초기화 및 주소 반환
void hashOpcode(opcodeNode *hashTable[HASHSIZE]);	// opCode를 해싱한다.
short int opcode(char *mnemonic, short int *form);	// mnemonic 인자에 상응하는 opcode 출력
void opcodelist(opcodeNode *hashTable[HASHSIZE]);	// opcode 정보가 저장된 해시 테이블 출력
void freeHashTable(opcodeNode **hashTable, short int maxLen); // hash table free



// 2주차
/* ========== 1. shell 관련 명령어 ========== */
enum FILETYPE{ORDINARY, ASM};
char *getFileName(short int *fileType);	// 명령문에서 파일명을 추출해낸다.
void type(char *fileName);	// 파일의 내용을 출력한다.


/* ========== 2. SIC/XE 어셈블러 명령 ========== */
#define SYMTABSIZE 60	// ASCII table에서 A부터 z까지 들어갈 수 있도록
#define DELIMETER 7	// isspace에서 space로 간주되는 6개 문자 + newline + 쉼표
#define MAXATTNO 7
#define MAXRECSIZE 4
#define MAXLINE 80
#define INTERMEDIATE "temp.txt"
#define MAXADDRFLAG 7
#define MAXOBJFLAG 1
#define ADDED -1
#define NOT_FOUND -1
#define XBPE (xbpe - 2)
#define MAX_TXT_REC 60

enum SYM_ERR_MODE{PROC_ERR, READ_CNT, INIT_CNT};
enum SYM_ERR_CODE { DUPLICATE, UNKNOWN, ATTNAME, OUT_OF_BOUND, REG, PLUS, OP_VAL ,OPCODE_TABLE, DIREC_SYNTAX, DEF_REC, UNKNOWN_SYM, WRONG_REC_LEN, OPCODE_FORM_MISMATCH, MOD_REC_SIGN, REF_REC};
enum RECORD_TYPE{INSTRUCT, DIRECTIVE, COMMENT, ERR};	// 종류 늘어나면 pass2의 recType != ERR 조건 수정할 것!
enum INSTRUCT_FIELD { SYMBOL, OPERATION, OPERAND1, OPERAND2 };
enum DIRECTIVE_FIELD { /*SYMBOL, */DIRECT = 1, VALUE };
enum DIRECTIVE_CODE {START, END, BYTE, WORD, RESB, RESW, BASE, NOBASE, LTORG, EQU, ORG, NONE};
enum ADDRFLAG{N, I, X, B, P, E};
enum OBJFLAG{BASE_REL};
enum RANGE { PC_LOWER = -2048, PC_UPPER = 2047, BASE_LOWER = 0, BASE_UPPER = 4095, EXD_LOWER = -524288, EXD_UPPER = 524287};

typedef struct _symNode {
	int loc;
	char *sym;
	struct _symNode *link;
}symNode;

short int symErr(short int mode, short int errCode, int lineCnt);	// sybol 관련 명령문 출력, error counter가 쌓이면 pass의 main loop을 중지한다.
int addSym(short int pass, int lineCnt, int loc, char *sym);		// symbol table에 symbol을 추가한다.
void symbol();		// symbol table을 출력한다.
void freeSymbol();	// symbol table의 메모리 할당을 해제한다.
short int directive(char *direct);	// directive 목록을 찾아서 목록이 있으면 directive code를 그렇지 않다면 -1을 반환
char *tok(char *newSrc, short int *comma);	// strtok와 동일, comma는 쉼표 문법 검사용 인자
void tokenize(short int pass, char *line, int lineCnt, int *loc, short int *recType, char **record, short int *directCode, short int *form, short int *addrFlag);	// assembly line을 받아 symbol, operation, operand 등으로 구분해주며, 문법을 체크한다.
short int pass1(char *fileName, int *progLen);	// pass1
short int getReg(int lineCnt, char *reg);	// register 이름을 담은 문자열을 인자로 받아, 실제 존재하는 레지스터면 상응하는 코드를, 그렇지 않으면 -1을 반환
char *genObjCode(int lineCnt, int loc, char **record, int opAddr, short int *addrFlag, int *objFlag, int *offset);	// tokenize 함수의 결과물을 받아 objCode를 생성, 반환한다.
short int pass2(char *fileName, int progLen);	// pass1이 성공한 경우 pass2
short int assemble(char *fileName);	// "assemble"명령어에 의해 호출되며 pass 전 초기화 및 pass1, pass2 사이의 처리 담당



// 3주차
#define MAXADDRLEN 5
#define MAXADDRNUM 3
#define MAXOBJLINE 80
#define MAXESTABSIZE 50
#define MAXDEFPERLINE 5
#define ESTAB_DUP -1
#define OPTABSIZE 64
#define REFTABSIZE 50
#define MAXREFPERLINE 12
#define MAXREFPERLINE_INDEXED 9
#define BPTABSIZE 50

#define INT_CMP(x, y) ((x)>(y))?1:(((x)==(y))?0:-1)

enum SIGN {FORBID_NEGATIVE, ALLOW_NEGATIVE};
enum MODE {MNEMONIC_MODE, ADDRESS_MODE};
enum ADDR_STAT {FILENAME, DOT_EXTENSION};
enum EXTSYM_TYPE {PROG, EXTSYM};
enum PASS_NUM {PASS1 = 1, PASS2};
enum REG {A_, X_, L_, B_, S_, T_, F_, PC = 8, SW};
enum MNEMONIC {ADD, CLEAR, COMP, DIV, J, LD, MUL, OR, RSUB, SHIFTL, SHIFTR, SSK, ST, SUB, TIX};
enum OPERAND_TYPE {MEM_REF, REG_REF};
enum OBJ_ERR{REL_ADDR, UNKNOWN_MNEMONIC};
enum PROC_STAT{NEW, CONTINUE};
enum RUN_MODE{PROC, STAT};
enum REG_PRINT_MODE{BP, EXIT};

typedef struct addrNode_ {
	char *addr;

	struct addrNode_ *link;
}addrNode;

typedef struct extSymNode_ {
	short int type;
	char *name;
	int address;
	unsigned long int length;
	
	struct extSymNode_ *link;
	struct extSymNode_ *mapLink;
}extSymNode;

typedef struct opNode_ {
	short int code;	// opcode
	char *mnemonic;	// mnemonic
	short int format;	// 형식

	struct opNode_ *link;
}opNode;

typedef struct refNode_ {
	short int index;
	char *symbol;

	struct refNode_ *link;
}refNode;

typedef struct bpNode_ {
	int address;
	short int chk;

	struct bpNode_ *link;
	struct bpNode_ *mapLink;
}bpNode;

void hashOpTab(opNode *hashTable[OPTABSIZE]);
char *getOpcode(short int opCode, short int *form);
void freeHashTable2(opNode **hashTable, short int maxLen);

char *getAddr(short int ordinal, short int maxlen);
void addAddr(short int init, short int *addrNum);
void freeAddr(void);

int addExtSym(short int init, short int pass, short int type, char *name, unsigned long int address, int length);
void printExtSym();
void printMap();

char *procHeadRec(short int pass, int *startAddr, char *line, int csaddr, int *cslth);
short int procDefRec(char *line, int startAddr, int csaddr);
short int procRefRec(char *line, short int *isIndexed);
void freeRefSym();
short int loadPass1(void);

void multiEdit(int startAddr, int csaddr, int addr, unsigned int value, int len);


short int procTxtRec(char *line, int startAddr, int csaddr);
short int procModRec(char *line, int startAddr, int csaddr, short int isIndexed, char *progName);
short int loadPass2(int *execAddr);
void loader(int *progLen);

short int run(short int mode, int progLen);
void printBp();
void freeHashTableBp(bpNode **hashTable, short int maxLen);
short int objErr(short int mode, short int errCode);
void freeEst();
int addBp(short int init, short int pass, unsigned long int address, int startAddr, int progLen);