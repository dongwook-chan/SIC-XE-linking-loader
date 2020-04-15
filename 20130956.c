#include "20130956.h"

symNode *symTab[SYMTABSIZE] = { 0 };	// symbol table, 모든 멤버를 0으로 초기화
int progAddr = 0;		// loader 또는 run 명령어를 수행할 때 시작하는 주소를 지정
						// sicsim이 시작되면 default로 progaddr는 0x00 주소로 지정
addrNode *rootAddr;
extSymNode *esTab[MAXESTABSIZE] = { 0 };
extSymNode *mapRoot = NULL;
opNode *opTab[OPTABSIZE];
refNode *refTab[REFTABSIZE] = { 0 };
bpNode *bpTab[BPTABSIZE] = { 0 };
bpNode *bpRoot = NULL;

int main() {
	char *buf;	// 입력 받은 명령문를 저장할 buffer
	char cmdWord[MAXCMDWORD + 1];	// 명령어를 저장할 변수
	memBase();	// 메모리 초기화
	hashOpcode(hashTable());	// opcode.txt를 기반으로 hash table 생성
	hashOpTab(opTab);
	
	while (1) {
		/* 초기화 */
		buf = initBufBase(INIT);	// buffer를 초기화하고,
		modCurBuf(NULL);			// current buffer 커서를 초기화	

		/* 셸 */
		printf("sicsim> ");
		
		/* 명령문 입력받기 */
		fgets(buf, MAXCMD, stdin);      // buffer에 명령문 받기

		/* 명령문 분석 및 처리 */
		modCmdStatus(VALID);	// 명령문 상태를 '유효함'을 초기화
		getCmdWord(cmdWord);	// 명령문에서 명령어 추출
		procCmdWord(cmdWord);	// 명령어별 처리(명령문 유효성 검사) 및 명령어 함수 호출
		if (modCmdStatus(NOCHANGE) == INVALID) {	// 명령문의 상태가 '유효하지 않음'이면
			initBufBase(FREE);	// 명령문을 history에 저장하지 않고 buffer를 free
			continue;
		}
		else if (modCmdStatus(NOCHANGE) == QUIT) 	// 명령문의 상태가 '종료'이면
			break;

		/* 명령문 histroy에 추가 */
		addCmd(buf);	// 유효한 명령문이면, buffer를 통째로 linked list의 멤버로 할당하기

		/* 명령어 후처리 */
		if (strncmp(cmdWord, "hi", 2) == 0)	// 명령어가 history였을 경우,
			history();	// 명령문이 추가된 후 명령어 함수 history 호출
	}

	free(memBase());	// memory free
	freeHashTable(hashTable(), HASHSIZE);	// hash table을 free
	freeHashTable2(opTab, OPTABSIZE);
	freeHashTableBp(bpTab, BPTABSIZE);
	freeAddr();
	addAddr(1, NULL);
	freeEst();
	addExtSym(1, 0, 0, NULL, 0, 0);
	freeRefSym();

	freeCmds();	// history에 저장된 명령문들을 free
	// 각종 어셈블러 테이블 free(혹은 해당 함수 내에서 free)
	
	return 0;
}


// 1주차
/* ========== 명령문 저장 ========== */
// 명령문 linkedlist의 첫 노드 주소값을 반환한다.
cmdNode *showRootCmd(void) {
	static cmdNode rootCmd;	// 명령문 linked list의 첫 노드
							// (첫 노드 역할만 수행하며, 명령문이 구조체 멤버로 할당되지 않는다.)
	return &rootCmd;
}

// 현재 linkedlist상 노드 주소값을 반환한다.
cmdNode *modCurCmd(cmdNode *newCmd) {	// 새로운 노드 인자
	static cmdNode *curCmd = NULL;

	if (curCmd == NULL)					// 처음 호출된 경우
		return curCmd = showRootCmd();	//		현재 노드를 첫 노드로 갱신, 반환

	else if (newCmd != NULL)			// 인자가 NULL이 아닐 경우
		curCmd = newCmd;				//		현재 노드를 인자로 갱신, 반환

	return curCmd;		// 인자가 NULL일 경우 현재 노드를 수정 없이 반환
}

// buffer의 초기화/주소값 반환
char *initBufBase(short int mode) {	// 모드 인자
	static char *bufBase;	// buffer

	if (mode == INIT)			// 초기화 모드일 경우
		return bufBase = (char *)malloc(sizeof(char)*(MAXCMD + 1));	// buffer에 동적 메모리 할당
																	//		유효한 명령문이 담긴 버퍼는 명령문 linked list 노드의 멤버로 할당되며,(createNewCmd)
																	//		유효한 명령문이 아닌 버퍼는 free된다.(main)
	else if (mode == NOINIT)	// 초기화하지 않을 경우
		return bufBase;			//		buffer 주소값 반환

	else if (mode == FREE) {	// free할 경우
		free(bufBase);			//		buffer free
		return NULL;
	}

	return bufBase;
}

// 현재 버퍼의 문자를 가르키는 주소값(커서) 반환/수정
char *modCurBuf(char *newBuf) {	// 새로운 커서 인자
	static char *curBuf = NULL;

	if (curBuf == NULL)		// 첫 호출인 경우
		return curBuf = initBufBase(NOINIT);	// buffer의 주소값으로 초기화, 반환

	else if (newBuf != NULL)	// 인자가 NULL이 아닌 경우
		curBuf = newBuf;		//		인자로 커서 갱신, 반환

	return curBuf;	// 인자가 NULL인 경우 현재 커서 주소값 반환
}

// 명령문 linkedlist에 새로운 노드를 생성
void createNewCmd(cmdNode **newcmd, char *buf) {	// 새로운 노드 인자, buffer 인자
	*newcmd = (cmdNode *)malloc(sizeof(cmdNode));	// 새로운 노드를 위한 동적 메모리 할당

	(*newcmd)->cmd = buf;	// 명령문 멤버에 buffer의 내용 할당(통째로)
	(*newcmd)->link = NULL;	// link 멤버는 비활성화
}

// 생성된 노드를 linkedlist에 추가
cmdNode *addCmd(char *buf) {	// buffer 인자
	cmdNode *curCmd = modCurCmd(NULL);      // 현재 노드를 받아오기

	createNewCmd(&(curCmd->link), buf);		// 새로운 노드 생성
	return modCurCmd(curCmd->link);			// 추가된 노드를 현재 노드로 갱신
}

// 명령문 linkedlist(history)의 메모리 할당 해제
void freeCmds() {
	cmdNode *curCmd = showRootCmd();
	cmdNode *nextCmd = curCmd->link;

	for (curCmd = nextCmd; curCmd; curCmd = nextCmd) {	// 첫 노드부터 각 노드를 방문하면서
		nextCmd = curCmd->link;		// 노드가 사라지기 전 link 정보 임시 저장
		free(curCmd->cmd);			// 명령문 멤버 우선 free
		free(curCmd);				// 노드 free
	}
}

/* ========== 명령어 추출 및 처리 ========== */
// 명령문에서 명령어 추출
void getCmdWord(char *cmdWord) {	// 명령어를 저장할 인자
	char *curBuf = initBufBase(NOINIT);	// buffer의 주소값
	short int chCnt = 0;				// 명령어의 문자수

	while (*curBuf != EOB && isspace(*curBuf))		// 명령어 앞의 공백문자 skip
		++curBuf; 

	while (*curBuf != EOB && !isspace(*curBuf)) {		// 명령어를 cmdWord에 copy
		*cmdWord = *curBuf;
		++cmdWord, ++curBuf, ++chCnt;
		if (chCnt == MAXCMDWORD)	// 가장 긴 명령어(opcodelist)의 길이에 도달하면
			break;					// copy를 중단한다.
	}

	*cmdWord = '\0';	// 명령어에 NULL문자 append
	modCurBuf(curBuf);	// 현재 버퍼(명령문) 커서 갱신
						// (명령어 바로 다음 문자를 가르키도록)
}

// 현재 명령문의 상태 반환/수정
short int modCmdStatus(short int newStatus) {	// 새로운 상태
	static short int cmdStatus;	// 현재 명령문의 상태

	if (newStatus != NOCHANGE)	// 인자가 NOCHANGE가 아니면
		return cmdStatus = newStatus;	// 새로운 상태로 현재 상태 갱신

	return cmdStatus;	// 인자가 NOCHANGE이면 수정 없이 현재 상태 반환
}

// 에러 코드에 맞는 예외처리
short int cmdError(short int errCode) {	// 에러코드
	if (modCmdStatus(NOCHANGE) == INVALID)	// error의 중복 출력을 방지한다.(한 번에 하나의 오류만 출력)
		return 1;

	modCmdStatus(INVALID);	// 에러가 발생했으므로 현재 명령문의 상태를 '유효하지 않음'으로 수정

	switch (errCode) {
	case OPERAND:	// 인자 및 명령문 형식 에러
		fprintf(stderr, "error: invalid delimiter or missing operand\n");
		fprintf(stderr, "Use comma for delimiter.\n");
		fprintf(stderr, "address operand\t: 00000 ~ FFFFF\n");
		fprintf(stderr, "value operand\t: 00 ~ FF\n"); break;
	case CMDWORD:	// 명령문 에러
		fprintf(stderr, "error: invalid command word\n");
		fprintf(stderr, "Use lowercase for command word\n"); break;
	case EXCESS:	// 인자가 최대길이를 초과하거나 요구되는 것보다 많은 경우, 유효하지 않은 문자의 경우
		fprintf(stderr, "error: excessive or invalid operand(s)\n");
		fprintf(stderr, "Cut operand(s) short to satisfy the boundary,\n");
		fprintf(stderr, "excluding invalid characters,\n"); break;
	case ADDRBOUND:	// 주소 인자가 범위를 벗어난 경우
		fprintf(stderr, "error: address out of bound\n");
		fprintf(stderr, "address operand\t: 00000 ~ FFFFF\n"); break;
	case ADDRORDER:	// 주소 인자의 순서 에러(대소관계 에러)
		fprintf(stderr, "error: address out of order\n");
		fprintf(stderr, "Two addresses must be in nondecreasing order."); break;
	case MNEMONIC:	// MNEMONIC 인자 에러
		fprintf(stderr, "error: invalid mnemonic\n");
		fprintf(stderr, "Use uppercase for mnemonic\n");
		fprintf(stderr, "Refer to opcodelist for valid mnemonic.\n"); break;
	case NO_OP:	// 인자가 없거나 음수
		fprintf(stderr, "error: missing operand or invalid character\n");
		fprintf(stderr, "Use hex for address and value operand.\n");
		fprintf(stderr, "Use char for mnemic operand.\n"); break;
	case EXTENSION:
		fprintf(stderr, "error: assemble requires file extension \".asm\"\n"); break;
	case NEGATIVE_OPERAND:
		fprintf(stderr, "error: operands must be positive\n"); break;
	case WRONG_EXTENSION:
		fprintf(stderr, "error: extension must be .obj\n"); break;
	}

	fprintf(stderr, "Refer to h[elp] for valid command line.\n");

	return 1;
}

// 명령문(cmd)의 끝(end)에 도달했는지 확인
short int isCmdEnd() {
	char *curBuf = modCurBuf(NULL);	// 현재 버퍼 커서
									// max까지만 받았는데 문자가 남아있다.

	while (*curBuf != EOB && isspace(*curBuf)) {		// 명령어 뒤의 공백문자 skip
		++curBuf;
	}

	if (*curBuf != EOB)	// 공백문자 뒤 공백문자가 아닌데 버퍼 종결문자(EOB)가 아닌 경우
		return 0;		//		불필요한 문자가 있으므로 end가 아님

	return 1;	// 검사를 통과했으면 end가 맞음
}

// 명령문에서 숫자 인자(start, end, address, value)를 추출한다.
unsigned long int getOperand(short int ordinal, short int maxLen, short int allowNeg) {	// 몇번째 인자인지, 인자의 최대 길이
	char *curBuf = modCurBuf(NULL);	// buffer에서 현재 커서 위치
	char *operand = (char *)malloc(sizeof(char)*(maxLen + 1));	// 인자를 받을 변수
	char *opPtr = operand;	// 인자 문자열 탐색용 포인터
	short int chCnt = 0;	// 인자의 길이
	int opHex;	// hex로 변환된 인자

	if (modCmdStatus(NOCHANGE) == INVALID)	// 이미 유효하지 않음이 판명된 명령문은 추출하지 않는다.
		return -1;

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 앞의 공백문자 skip
		++curBuf;

	if (allowNeg == FORBID_NEGATIVE && *curBuf == '-') {		// 음수를 받으면 오류
		cmdError(NEGATIVE_OPERAND);
		return -1;
	}

	while (*curBuf != EOB && isxdigit(*curBuf)) {	// 인자 받기
		*opPtr = *curBuf;
		++opPtr; ++curBuf; ++chCnt;
		if (chCnt == maxLen)
			break;
	}

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 뒤 공백문자 skip
		++curBuf;

	// 인자 유효성 검사
	//		첫번째 인자일 수도 있고 마지막 인자일 수도 있는(VAR) 인자는
	//		유효성 검사 면제(procCmd에서 별도 검사 진행)
	if (operand == opPtr) {	// 인자가 없거나 16진수가 아니라면
		cmdError(NO_OP);	// error
		return -1;
	}

	if (ordinal != LAST && ordinal != VAR) {	// 마지막 인자가 아닌데
		if (*curBuf != ',') {		 // ','로 끝나지 않으면
			cmdError(OPERAND);		// error
			return -1;
		}
		else	// 마지막 인자가 아니면서 ','로 끝나면
			++curBuf;	// ',' 다음으로 커서 이동
	}

	modCurBuf(curBuf);	// 인자 추출을 위한 커서 이동이 끝났으면 갱신한다.

	if (ordinal == LAST && ordinal != VAR)	// 마지막 인자인데
		if (!isCmdEnd()) {		// 인자가 남아 있다면
			cmdError(EXCESS);	// error
			return -1;
		}

	*opPtr = '\0';	// 인자에 null 문자 append
	opHex = strtol(operand, NULL, 16);
	free(operand);
	return opHex;	// 문자열로 받은 인자를 16진수로 반환
}

char *getMnemonic(void) {
	char *curBuf = modCurBuf(NULL);	// buffer에서 현재 커서 위치
	char *mnemonic = (char *)malloc(sizeof(char)*(MAXMNEMONIC + 1));	// 인자를 받을 변수
	char *mnemPtr = mnemonic;	// 인자 문자열 탐색용 포인터
	short int chCnt = 0;	// 인자의 길이

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 앞의 공백문자 skip
		++curBuf;

	while (*curBuf != EOB && isalpha(*curBuf)) {	// 인자 받기
		*mnemPtr = *curBuf;
		++mnemPtr; ++curBuf; ++chCnt;
		if (chCnt == MAXMNEMONIC)	// 인자 길이의 최댓값에 도달하면
			break;	// copy를 중단한다.
	}

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 뒤 공백문자 skip
		++curBuf;

	modCurBuf(curBuf);	// 인자 추출을 위한 커서 이동이 끝났으면 갱신한다.

	if (mnemonic == mnemPtr) {	// 인자가 없거나 알파벳이 아니라면
		cmdError(NO_OP);	// error
		return NULL;
	}

	if (!isCmdEnd()) {	// mnemonic인자는 홀로 쓰이는 인자인데 인자가 남아 있다면
		cmdError(EXCESS);	// error
		return NULL;
	}

	*mnemPtr = '\0';	// 인자에 null 문자 append

	return mnemonic;
}

// 명령문에서 mnemonic 인자를 추출한다.
char *getAddr(short int ordinal, short int maxlen) {
	char *curBuf = modCurBuf(NULL);	// buffer에서 현재 커서 위치
	char *addr = (char *)malloc(sizeof(char)*(MAXCMD + 1));	// 인자를 받을 변수
	char *addrPtr = addr;	// 인자 문자열 탐색용 포인터
	short int chCnt = 0;	// 인자의 길이
	short int status = FILENAME;
	char extension[] = "obj";
	char *extPtr = extension;

	if (modCmdStatus(NOCHANGE) == INVALID)	// 이미 유효하지 않음이 판명된 명령문은 추출하지 않는다.
		return NULL;

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 앞의 공백문자 skip
		++curBuf;

	while (*curBuf != EOB && (isalnum(*curBuf)|| *curBuf == '.')) {	// 인자 받기
		if (*curBuf == '.') {
			if (status == FILENAME) {
				status = DOT_EXTENSION;
				++curBuf;
				continue;
			}
			else {
				cmdError(WRONG_EXTENSION);
				return NULL;
			}
		}

		if (status == FILENAME) {
			*addrPtr = *curBuf;
			++addrPtr; ++curBuf; ++chCnt;
		}
		else if (status == DOT_EXTENSION) {
			if (*extPtr == '\0') {
				cmdError(EXCESS);
				return NULL;
			}
			if (*curBuf == *extPtr) {
				++extPtr; ++curBuf;
			}
			else {
				cmdError(WRONG_EXTENSION);
				return NULL;
			}
		}
		if (chCnt == maxlen)	// 인자 길이의 최댓값에 도달하면
			break;	// copy를 중단한다.
	}

	if (*extPtr != '\0') {
		cmdError(WRONG_EXTENSION);
		return NULL;
	}

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 뒤 공백문자 skip
		++curBuf;

	if (addr == addrPtr) {	// 인자가 없거나 알파벳이 아니라면
		cmdError(NO_OP);	// error
		return NULL;
	}

	if (ordinal != LAST && ordinal != VAR) {	// 마지막 인자가 아닌데
		if (*curBuf != ',') {		 // ','로 끝나지 않으면
			cmdError(OPERAND);		// error
			return NULL;
		}
		else	// 마지막 인자가 아니면서 ','로 끝나면
			++curBuf;	// ',' 다음으로 커서 이동
	}

	modCurBuf(curBuf);	// 인자 추출을 위한 커서 이동이 끝났으면 갱신한다.

	if (ordinal == LAST && ordinal != VAR)	// 마지막 인자인데
		if (!isCmdEnd()) {		// 인자가 남아 있다면
			cmdError(EXCESS);	// error
			return NULL;
		}
	
	*addrPtr = '\0';	// 인자에 null 문자 append

	return addr;
}

// 명령어(cmdWord)에 따른 처리(proc)
short int procCmdWord(char *cmdWord) {
	static int progLen;
	static int startAddr;
	// 인자가 없는 명령어의 경우 명령문이 끝났는지 확인(isCmdEnd)하고,
	//		끝났으면 명령어별 함수 호출

	// 인자가 있는 명령어는 개수만큼 인자를 명령문에서 추출(getOperand, getMnemonic)하고,
	//		끝났으면 명령어별 함수 호출

	if (strcmp(cmdWord, "h") == 0 || strcmp(cmdWord, "help") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		help();
	}
	else if (strcmp(cmdWord, "d") == 0 || strcmp(cmdWord, "dir") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		//dir();
	}
	else if (strcmp(cmdWord, "q") == 0 || strcmp(cmdWord, "quit") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		modCmdStatus(QUIT);
	}
	else if (strcmp(cmdWord, "hi") == 0 || strcmp(cmdWord, "history") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		// 명령문의 유효성 검사가 끝나고 명령문이 linkedlist에 추가된 뒤 history 함수 별도 호출
	}
	else if (strcmp(cmdWord, "du") == 0 || strcmp(cmdWord, "dump") == 0) {
		int start;
		unsigned long int end;

		if (isCmdEnd()) {	// 명령문이 끝났으면, 인자가 없는 경우이므로
			start = modLastAddr(-1);	// 마지막으로 dump된 주소 + 1을 start에 할당
			end = start + 0xA0 - 1;		// default로 그로부터 0xA0 - 1 까지의 주소를 end에 할당
		}
		else {	// 명령문이 끝나지 않았다면, 인자가 1개 이상이다.
			char *curBuf;	// 현재 버퍼 커서
			start = getOperand(VAR, MAXHEX, FORBID_NEGATIVE);	// 명령문에서 1번째 인자 추출
			if (*(curBuf = modCurBuf(NULL)) != ',' && isCmdEnd()) {	// 인자 다음 ,가 있고, 명령문의 끝이라면 인자가 1개인 경우
				end = start + 0xA0 - 1;	// default로 start로부터 0xA0 - 1 까지의 주소를 end에 할당
			}
			else if (*curBuf == ',') {	// 인자 다음 ,가 있고 명령문이 끝이 아니면 인자가 2개인 경우
				modCurBuf(curBuf + 1);	// , 다음으로 커서 이동
				end = getOperand(LAST, MAXHEX, FORBID_NEGATIVE);	// 2번째 인자 추출
				if (!(start <= end))	// 인자간 대소관계 검사
					return cmdError(ADDRORDER);
			}
			else
				return cmdError(OPERAND);

			if (end >= MEMSIZE)		// end가 out of bound이면
				end = MEMSIZE - 1;	//		최댓값으로 설정

			if (!((0 <= start && start < MEMSIZE)))	// start가 out of bound이면
				return cmdError(ADDRBOUND);	// 예외처리
		}
		if (modCmdStatus(NOCHANGE) == VALID) {	// 위 검사들을 통과했다면
			dump(start, end);	// 명령어 함수(dump) 호출
		}
	}
	else if (strcmp(cmdWord, "e") == 0 || strcmp(cmdWord, "edit") == 0) {
		int addrOp;	// 주소 인자
		short int valOp;	// 값 인자

		addrOp = getOperand(FIRST, MAXHEX, FORBID_NEGATIVE);	// 명령문에서 주소 인자 추출
		valOp = getOperand(LAST, MAXVALOP, ALLOW_NEGATIVE);	// 명령문에서 값 인자 추출

		if (!((0 <= addrOp && addrOp < MEMSIZE)))	// start가 out of bound이면
			return cmdError(ADDRBOUND);	// 예외처리

		if (modCmdStatus(NOCHANGE) == VALID)	// 유효한 명령문이라면
			edit(addrOp, valOp);	// 명령어 함수(edit) 호출
	}
	else if (strcmp(cmdWord, "f") == 0 || strcmp(cmdWord, "fill") == 0) {
		int start;
		unsigned long int end;	// 주소 인자 2개
		short int valOp;	// 값 인자

		start = getOperand(FIRST, MAXHEX, FORBID_NEGATIVE);	// 명령문에서 1번째 주소인자 추출
		end = getOperand(SECOND, MAXHEX, FORBID_NEGATIVE);	// 명령문에서 2번째 주소인자 추출
		valOp = getOperand(LAST, MAXVALOP, ALLOW_NEGATIVE);		// 명령문에서 값 인자 추출

		if (!(start <= end))	// 인자간 대소관계 검사
			return cmdError(ADDRORDER);

		if (end >= MEMSIZE)		// end가 out of bound이면
			end = MEMSIZE - 1;	//		최댓값으로 설정

		if (!((0 <= start && start < MEMSIZE)))	// start가 out of bound이면
			return cmdError(ADDRBOUND);	// 예외처리

		if (modCmdStatus(NOCHANGE) == VALID)	// 유효한 명령문이라면
			fill(start, end, valOp);	// 명령어 함수(fill) 호출
	}
	else if (strcmp(cmdWord, "reset") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		reset();
	}
	else if (strcmp(cmdWord, "opcode") == 0) {
		char *mnemonic = getMnemonic();	// 명령문에서 mnemonic 인자 추출

		if (modCmdStatus(NOCHANGE) == VALID)	// 유효한 명령문이라면
			if (opcode(mnemonic, NULL) == NOT_FOUND)	// 명령어 함수(opcode) 호출
				cmdError(MNEMONIC);

		free(mnemonic);
	}
	else if (strcmp(cmdWord, "opcodelist") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		opcodelist(hashTable());
	}
	else if (strcmp(cmdWord, "assemble") == 0) {
		short int fileType;		// 어떤 종류의 파일인가
		char *fileName = getFileName(&fileType);	// 명령문에서 fileName 인자 추출

		if (modCmdStatus(NOCHANGE) == VALID)	// 유효한 명령문이라면
			assemble(fileName);	// 명령어 함수(assemble) 호출

		free(fileName);
	}
	else if (strcmp(cmdWord, "type") == 0) {
		short int fileType;		// 어떤 종류의 파일인가
		char *fileName = getFileName(&fileType);	// 명령문에서 fileName 인자 추출

		if (modCmdStatus(NOCHANGE) == VALID)	// 유효한 명령문이라면
			type(fileName);	// 명령어 함수(assemble) 호출

		free(fileName);
	}
	else if (strcmp(cmdWord, "symbol") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		symbol();
	}
	else if (strcmp(cmdWord, "progaddr") == 0) {
		int addr = getOperand(LAST, MAXADDRLEN, FORBID_NEGATIVE);	// 명령문에서 mnemonic 인자 추출

		if (run(STAT, -1) == CONTINUE) {
			fprintf(stderr, "can not modify progaddr while program is running\n");
			fprintf(stderr, "terminate program in order to modify\n");
			return 0;
		}

		if (modCmdStatus(NOCHANGE) == VALID)	// 유효한 명령문이라면
			progAddr = addr;

		printf("%x\n", addr);
	}
	else if (strcmp(cmdWord, "loader") == 0) {
		short int addrNum = 0;

		if (isCmdEnd()) {	// 명령문이 끝났으면, 인자가 없는 경우이므로
			return cmdError(NO_OP);
		}
		else {	// 명령문이 끝나지 않았다면, 인자가 1개 이상이다.
			char *curBuf;	// 현재 버퍼 커서

			addAddr(0, &addrNum);

			if (*(curBuf = modCurBuf(NULL)) != ',' && isCmdEnd())	// 인자 다음 ,가 있고, 명령문의 끝이라면 인자가 1개인 경우
				;
			else if (*curBuf == ',') {	// 인자 다음 ,가 있고 명령문이 끝이 아니면 인자가 2개인 경우
				modCurBuf(curBuf + 1);
				
				addAddr(0, &addrNum);

				if (*(curBuf = modCurBuf(NULL)) != ',' && isCmdEnd())	// 인자 다음 ,가 있고, 명령문의 끝이라면 인자가 1개인 경우
					;
				else if (*curBuf == ',') {	// 인자 다음 ,가 있고 명령문이 끝이 아니면 인자가 2개인 경우
					modCurBuf(curBuf + 1);

					addAddr(0, &addrNum);
				}
				else
					return cmdError(OPERAND);
			}
			else
				return cmdError(OPERAND);
		}

		loader(&progLen);

		if (progLen > 0XFFFFF) {
			fprintf(stderr, "source file longer than available memory size\n");
			return -1;
		}

		startAddr = progAddr;
	}
	/*
	else if(strcmp(cmdWord, "multi")==0){
		int start;
		unsigned long int end, end2;	// 주소 인자 2개

		start = getOperand(FIRST, MAXHEX, FORBID_NEGATIVE);	// 명령문에서 1번째 주소인자 추출
		end = getOperand(SECOND, MAXHEX, FORBID_NEGATIVE);
		end2 = getOperand(LAST, MAXHEX, FORBID_NEGATIVE);
		

		multiEdit(0, 0, start, end, end2);
	}
	*/
	else if (strcmp(cmdWord, "bp") == 0) {
		if (isCmdEnd()) {	// 명령문이 끝났으면, 인자가 없는 경우이므로
			printBp();
		}
		else {
			char *curBuf = modCurBuf(NULL);
			for (; isspace(*curBuf); ++curBuf);

			if (strcmp(curBuf, "clear\n") == 0)
				freeHashTableBp(bpTab, BPTABSIZE);
			else if (isxdigit(*curBuf)) {
				int address = getOperand(LAST, MAXADDRLEN, FORBID_NEGATIVE);
				addBp(0, PASS1, address, startAddr, progLen);
			}
		}
	}
	else if (strcmp(cmdWord, "run") == 0) {
		if (!isCmdEnd())
			return cmdError(EXCESS);

		run(PROC, progLen);
	}
	else
		return cmdError(CMDWORD);

	return 0;
}


/* ========== 1. Shell 관련 명령어 ========== */
// 명령어 리스트 화면에 출력
void help() {
	puts("h[elp]");
	puts("d[ir]");
	puts("q[uit]");
	puts("hi[story]");
	puts("du[mp] [start, end]");
	puts("e[dit] address, value");
	puts("f[ill] start, end, value");
	puts("reset");
	puts("opcode mnemonic");
	puts("opcodelist");
	puts("assemble filename");
	puts("type filename");
	puts("symbol");
}

// 현재 디렉토리 내 오브젝트 목록 출력
/*
void dir(void) {
	DIR *dirStr = opendir(".");  // open directory stream
	struct dirent *dirObj;  // objects in the directory
	struct stat objInfo;    // stores information about an object

	if (dirStr)			// if the stream is not empty
		while ((dirObj = readdir(dirStr))) {
			stat(dirObj->d_name, &objInfo);

			if (strcmp(dirObj->d_name, ".") == 0 || strcmp(dirObj->d_name, "..") == 0)       // skip . and ..
				continue;

			printf("%s", dirObj->d_name);   // print object name

			if (S_ISDIR(objInfo.st_mode))    // if the object is directory
				putchar('/');
			else if ((stat(dirObj->d_name, &objInfo) == 0 && objInfo.st_mode& S_IXUSR))      // if the object is executable
				putchar('*');

			putchar('\t');
		}

	closedir(dirStr);

	putchar('\n');
}
*/

// linked list에 저장된 명령어 목록 출력
void history(void) {
	cmdNode *curCmd = showRootCmd()->link;	// 명령문이 저장된 첫 노드의 주소 할당
	int i;	// 명령문 인덱스 번호 출력용 변수

	if (curCmd->link == NULL)	// history가 비어있다면
		return;	// 아무것도 출력하지 않는다.

	for (i = 0; curCmd; curCmd = curCmd->link, ++i)	// linked list의 마지막 노드까지 탐색하여
		printf("%d\t%s", i + 1, curCmd->cmd);	// 명령문 인덱스 번호와 명령문을 출력
}

/* ========== 2. 메모리 관련 명령어 ========== */
// 메모리 초기화 및 주소 반환
unsigned char *memBase(void) {
	static unsigned char *mem = NULL;	// 메모리 포인터

	if (mem == NULL)	// 첫 호출인 경우
		mem = (unsigned char *)calloc(MEMSIZE, sizeof(unsigned char));	// 메모리에 MEMSIZE만큼의 메모리 할당 후 0으로 초기화

	return mem;	// 첫 호출이 아닌 경우 주소 반환
}

// dump된 마지막 주소를 반환/수정
int modLastAddr(int newAddr) {
	static int lastAddr = 0;	// dump된 마지막 주소

	if (newAddr >= 0)	// 인자가 0 이상이면
		lastAddr = newAddr;	// 새로운 주소로 갱신

	return lastAddr;	// 인자가 음수이면, 기존의 마지막 주소 반환
}

// 메모리 내용 출력
void dump(int start, int end) {
	int offset1 = start % 0x10;	// 1번째 인자로 인해 출력되지 않는 주소의 개수
	int offset2 = end % 0x10;	// 1번째 인자로 인해 출력되는 주소의 개수
	int lineStart = start - offset1;	// 매 줄의 시작 주소
	int lastLine = end - offset2;	// 마지막 줄의 시작 주소
	unsigned char *mem;	// 메모리 포인터
	short int i;	// 줄 내부 탐색용 인덱스

	if (lineStart == lastLine) {	// 1줄만 출력하는 경우
		// print first line(skip offset1)
		mem = memBase() + lineStart;
		printf("%.5X", lineStart);					// mem addr
		for (i = 0; i < offset1; ++i)		// mem content(hex)
			printf("   ");
		for (; i <= offset2; ++i)
			printf(" %.2X", mem[i]);
		for (; i < 0x10; ++i)
			printf("   ");
		printf(" ; ");								// delimiter
		for (i = 0; i < offset1; ++i)		// mem content(ASCII)
			putchar('.');
		for (; i <= offset2; ++i)
			if (0x20 <= mem[i] && mem[i] <= 0x7E)
				putchar(mem[i]);
			else
				putchar('.');
		for (; i < 0x10; ++i)
			putchar('.');
		putchar('\n');
	}
	else {	// 2줄 이상 출력하는 경우
		// print first line(skip offset1)
		mem = memBase() + lineStart;
		printf("%.5X", lineStart);					// mem addr
		for (i = 0; i < offset1; ++i)		// mem content(hex)
			printf("   ");
		for (; i < 0x10; ++i)
			printf(" %.2X", mem[i]);
		printf(" ; ");								// delimiter
		for (i = 0; i < offset1; ++i)		// mem content(ASCII)
			putchar('.');
		for (; i < 0x10; ++i)
			if (0x20 <= mem[i] && mem[i] <= 0x7E)
				putchar(mem[i]);
			else
				putchar('.');
		putchar('\n');

		// print intermediate line
		lineStart += 0x10;
		mem += 0x10;
		while (lineStart < lastLine) {
			printf("%.5X", lineStart);					// mem addr
			for (i = 0; i < 0x10; ++i)					// mem content(hex)
				printf(" %.2X", mem[i]);
			printf(" ; ");								// delimiter
			for (i = 0; i < 0x10; ++i)					// mem content(ASCII)
				if (0x20 <= mem[i] && mem[i] <= 0x7E)
					putchar(mem[i]);
				else
					putchar('.');
			putchar('\n');
			lineStart += 0x10;
			mem += 0x10;
		}

		// print last line(print only offset2)
		printf("%.5X", lineStart);					// mem addr
		for (i = 0; i <= offset2; ++i)			// mem content(hex)
			printf(" %.2X", mem[i]);
		for (; i < 0x10; ++i)
			printf("   ");
		printf(" ; ");								// delimiter
		for (i = 0; i <= offset2; ++i)		// mem content(ASCII)
			if (0x20 <= mem[i] && mem[i] <= 0x7E)
				putchar(mem[i]);
			else
				putchar('.');
		for (; i < 0x10; ++i)
			putchar('.');
		putchar('\n');
	}

	if(end == 0xFFFFF)	// 마지막으로 출력된 주소가 max 값이면 
		modLastAddr(0);	//		0으로 초기화
	else
		modLastAddr(end + 1);	// 아니면 마지막으로 출력된 주소 + 1으로 갱신
}

// 주어진 주소인자의 값을 값인자로 갱신
void edit(int addr, int value) {
	memBase()[addr] = value;
}

// 여러 칸에 걸쳐 edit
void multiEdit(int startAddr, int csaddr, int addr, unsigned int value, int len) {	// len in half byte
	int lsb;
	int opcode;
	unsigned char *mem;
	unsigned char *memPtr;
	short int isInst;
	/*
	if (value < 0) {
		value *= -1;
		value = ~value;
		++value;
	}
	*/
	// 상대적인 메모리 주소값 참조
	memPtr = mem = memBase() + csaddr + addr - startAddr;

	// halfbyte로 주어진 len이 홀수이면 첫 byte의 반만 수정하기 위해 선처리
	isInst = len % 2;
	if (isInst) {
		--len;
		++memPtr;
	}
	
	// 주어진 len만큼 처리
	for (memPtr += len / 2 - 1; memPtr >= mem + isInst; --memPtr) {
		lsb = value % 0X100;
		value /= 0X100;

		*memPtr = lsb;
	}

	// 홀수 len이 주어진 경우 첫 byte의 반만 후처리
	if (isInst) {
		opcode = *memPtr;
		value = opcode | value;
		*mem = value;
	}
}

// 메모리의 start부터 end까지를 value로 갱신
void fill(int start, int end, short int value) {
	memset(memBase() + start, value, end - start + 1);
}

// 메모리 전체를 0으로 변경
void reset(void) {
	memset(memBase(), 0, MEMSIZE);
}

/* ========== 3. OPCODE TABLE 관련 명령어 ========== */
// 해시 테이블 초기화 및 주소 반환
opcodeNode **hashTable(void) {
	static opcodeNode *hashTable[HASHSIZE] = { 0 };	// opcodeNode를 가르키는 포인터들의 배열
													//		0으로 초기화한다.
	return hashTable;
}

// opCode를 해싱한다.
void hashOpcode(opcodeNode *hashTable[HASHSIZE]) {
	FILE *fp = fopen("opcode.txt", "r");	// opcode.txt 열기
	char *arg1;	// opcode.txt의 1번째 attribute값
	char *arg2;	// opcode.txt의 2번째 attribute값
	char arg3[4];	// opcode.txt의 3번째 attribute값
	opcodeNode **tablePtr;	// hash table에 대한 포인터
	opcodeNode *opcode;	// opcode 1개를 저장할 구조체에 대한 포인터
	
	if (fp == NULL) {
		fprintf(stderr, "file open error\n");
		exit(EXIT_FAILURE);
	}

	while (!feof(fp)) {
		short int index;	// 해시 테이블 내 인덱스
		short int i;		// for loop용 인덱스

		tablePtr = hashTable;	// 테이블의 첫번째 버킷으로 포인터 초기화

		opcode = (opcodeNode *)malloc(sizeof(opcodeNode));	// opcode를 받을 구조체에 메모리 할당
		arg1 = (char *)malloc(sizeof(char)*(MAXOPCODE + 1));	// 1번째 attribute값을 받을 변수에 메모리 할당
		arg2 = (char *)malloc(sizeof(char)*(MAXMNEMONIC + 1));	// 2번째 attribute값을 받을 변수에 메모리 할당

		if (fscanf(fp, "%s%s%s", arg1, arg2, arg3) != 3)	// 3개 attribute값 모두 받지 못했다면
			break;	// EOF로 간주하고 해싱 종료
		
		for (i = 0, index = 0; arg2[i]; ++i)	// 2번째 인자의 모든 문자를 탐색
			index += arg2[i];	// 각 값을 합한다.
		tablePtr += index % 20;	// 합한 값을 해시 테이블 인덱스로 활용

		// opcode를 담을 구조체에 입력 받은 attribute값 할당
		opcode->code = arg1;
		opcode->mnemonic = arg2;
		opcode->format = arg3[0] - '0';
		opcode->link = NULL;

		if (*tablePtr != NULL)			// 버킷의 첫 노드가 아니면
			opcode->link = *tablePtr;	//		기존의 linked list를 append

		*tablePtr = opcode;	// 현재 노드를 버킷의 첫 노드로
	}	
	fclose(fp);
}

void hashOpTab(opNode *hashTable[OPTABSIZE]) {
	FILE *fp = fopen("opcode.txt", "r");	// opcode.txt 열기
	char *arg1;	// opcode.txt의 1번째 attribute값
	char *arg2;	// opcode.txt의 2번째 attribute값
	char arg3[4];	// opcode.txt의 3번째 attribute값
	opNode **tablePtr;	// hash table에 대한 포인터
	opNode *opcode;	// opcode 1개를 저장할 구조체에 대한 포인터

	if (fp == NULL) {
		fprintf(stderr, "file open error\n");
		exit(EXIT_FAILURE);
	}

	while (!feof(fp)) {
		short int index;	// 해시 테이블 내 인덱스

		tablePtr = hashTable;	// 테이블의 첫번째 버킷으로 포인터 초기화

		opcode = (opNode *)malloc(sizeof(opNode));	// opcode를 받을 구조체에 메모리 할당
		arg1 = (char *)malloc(sizeof(char)*(MAXOPCODE + 1));	// 1번째 attribute값을 받을 변수에 메모리 할당
		arg2 = (char *)malloc(sizeof(char)*(MAXMNEMONIC + 1));	// 2번째 attribute값을 받을 변수에 메모리 할당

		if (fscanf(fp, "%s%s%s", arg1, arg2, arg3) != 3)	// 3개 attribute값 모두 받지 못했다면
			break;	// EOF로 간주하고 해싱 종료

		index = strtol(arg1, NULL, 16);
		tablePtr += index % OPTABSIZE;	// 합한 값을 해시 테이블 인덱스로 활용
		free(arg1);

								// opcode를 담을 구조체에 입력 받은 attribute값 할당
		opcode->code = index;
		opcode->mnemonic = arg2;
		opcode->format = arg3[0] - '0';
		opcode->link = NULL;

		if (*tablePtr != NULL)			// 버킷의 첫 노드가 아니면
			opcode->link = *tablePtr;	//		기존의 linked list를 append

		*tablePtr = opcode;	// 현재 노드를 버킷의 첫 노드로
	}

	fclose(fp);
}

char *addRef(short int pass, short int index, char *sym) {
	refNode **bucket;	// hash table에 대한 포인터(버킷)
	refNode *tempSym, *prevSym;	// 버킷 내 탐색을 위한 커서
	refNode *symPtr;	// 심볼 구조체에 대한 포인터
	short int status;	// 비교 결과

	bucket = refTab + index % REFTABSIZE;	// 첫 글자의 ASCII 값을 해시 테이블 인덱스로 활용

	symPtr = (refNode *)malloc(sizeof(refNode));	// 심볼 구조체 포인터에 메모리 동적 할당

	symPtr->index = index;												// 구조체에 멤버 할당하기
	symPtr->symbol = sym;
	symPtr->link = NULL;

	if (*bucket == NULL) {// 버킷이 비어있을 경우
		if (pass == 1)
			*bucket = symPtr;//		버킷의 첫번째 symbol로 할당
	}
	else {					// 버킷에 기존 symbol이 존재할 경우
		prevSym = NULL;
		tempSym = *bucket;
		while (tempSym != NULL) {	// 버킷 내 다른 노드가 존재하면 정렬하기
			//status = strcmp(tempSym->sym, sym);
			status = (tempSym->index > index);

			if (status == 1)
				break;
			else if (status == 0) {
				free(symPtr);
				if(pass == 1)
					symErr(PROC_ERR, DUPLICATE, -1);
				return tempSym->symbol;
			}

			prevSym = tempSym;
			tempSym = tempSym->link;
		}

		if (pass == 1) {
			if (tempSym != *bucket) {	// symbol이 두번째 노드 이후에 삽입되는 경우
				prevSym->link = symPtr;
				symPtr->link = tempSym;
			}
			else {	// symbol이 첫번째 노드에 삽입되는 경우
				symPtr->link = *bucket;
				*bucket = symPtr;
			}
		}
	}
	if (pass == 2) 
		symErr(PROC_ERR, UNKNOWN_SYM, -1);

	return NULL;	// symbol 추가 성공
}

void freeRefSym() {
	short int i;	// for loop용 인덱스
	refNode *symPtr, *nextPtr;

	for (i = 0; i < REFTABSIZE; ++i) {	// 해시 테이블 탐색
		if (refTab[i] != NULL) {	// 버킷에 노드가 있다면
			for (symPtr = refTab[i]; symPtr; symPtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = symPtr->link;
				free(symPtr->symbol);	// symbol free
				free(symPtr);	// 자료구조 free
			}
			refTab[i] = NULL;	// symTabl을 NULL값으로 초기화한다.
		}
	}
}

// mnemonic 인자에 상응하는 opcode 출력
short int opcode(char *mnemonic, short int *form) {
	short int index;	// 해시 테이블 내 인덱스
	short int i;	// for loop용 인덱스
	opcodeNode *opcodePtr;	// opcode를 저장할 구조체에 대한 포인터

	if (mnemonic == NULL)
		return -1;

	for (i = 0, index = 0; mnemonic[i]; ++i)	// mnemonic 각 문자 값을 더해
		index += mnemonic[i];					//		해시 테이블용 인덱스 계산
	opcodePtr = *(hashTable() + (index % 20));
	while (opcodePtr) {	// 해당 버킷 탐색
		if (strcmp(mnemonic, opcodePtr->mnemonic) == 0) {	// mnemonic이 일치하면
			if (form == NULL)
				printf("opcode is %s\n", opcodePtr->code);	// 출력
			else
				*form = opcodePtr->format;
			return strtol(opcodePtr->code, NULL, 16);
		}
		opcodePtr = opcodePtr->link;	// 버킷의 다음 opcode
	}
	return -1;
}

char *getOpcode(short int opCode, short int *form) {
	short int index;	// 해시 테이블 내 인덱스
	opNode *opcodePtr;	// opcode를 저장할 구조체에 대한 포인터
	short int cmpRet;

	if (opCode < 0)
		return NULL;

	index = opCode;
	opcodePtr = *(opTab + (index % OPTABSIZE));
	while (opcodePtr) {	// 해당 버킷 탐색
		cmpRet = (opCode == opcodePtr->code);
		if (cmpRet) {	// mnemonic이 일치하면
			*form = opcodePtr->format;
			return opcodePtr->mnemonic;
		}
		opcodePtr = opcodePtr->link;	// 버킷의 다음 opcode
	}
	return NULL;
}

// opcode 정보가 저장된 해시 테이블 출력
void opcodelist(opcodeNode *hashTable[HASHSIZE]) {
	short int i;	// for loop용 인덱스
	opcodeNode *opcodePtr;	// opcode를 저장할 구조체에 대한 포인터

	for (i = 0; i < HASHSIZE; ++i) {	// 해시 테이블 탐색
		printf("%d : ", i);	// i번째 버킷 출력 중
		if (hashTable[i]) {	// 버킷에 노드가 있다면
			for (opcodePtr = hashTable[i]; opcodePtr->link; opcodePtr = opcodePtr->link)	// 버킷의 마지막 노드까지 탐색하여
				printf("[%s,%s] -> ", hashTable[i]->mnemonic, hashTable[i]->code);	// 내용 출력
			printf("[%s,%s]", hashTable[i]->mnemonic, hashTable[i]->code);
		}
		putchar('\n');
	}
}

// opcode 정보가 저장된 해시 테이블 free
void freeHashTable(opcodeNode **hashTable, short int maxLen) {
	short int i;	// for loop용 인덱스
	opcodeNode *opcodePtr, *nextPtr;	// opcode를 저장할 구조체에 대한 포인터

	for (i = 0; i < maxLen; ++i) {	// 해시 테이블 탐색
		if (hashTable[i])	// 버킷에 노드가 있다면
			for (opcodePtr = hashTable[i]; opcodePtr; opcodePtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = opcodePtr->link;
				free(opcodePtr->code);
				free(opcodePtr->mnemonic);
				free(opcodePtr);
			}
		hashTable[i] = NULL;
	}
}

void freeHashTable2(opNode **hashTable, short int maxLen) {
	short int i;	// for loop용 인덱스
	opNode *opcodePtr, *nextPtr;	// opcode를 저장할 구조체에 대한 포인터

	for (i = 0; i < maxLen; ++i) {	// 해시 테이블 탐색
		if (hashTable[i])	// 버킷에 노드가 있다면
			for (opcodePtr = hashTable[i]; opcodePtr; opcodePtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = opcodePtr->link;
				free(opcodePtr->mnemonic);
				free(opcodePtr);
			}
		hashTable[i] = NULL;
	}
}

void activateBp(bpNode **hashTable, short int maxLen) {
	short int i;	// for loop용 인덱스
	bpNode *opcodePtr, *nextPtr;	// opcode를 저장할 구조체에 대한 포인터

	for (i = 0; i < maxLen; ++i) {	// 해시 테이블 탐색
		if (hashTable[i]) {	// 버킷에 노드가 있다면
			for (opcodePtr = hashTable[i]; opcodePtr; opcodePtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = opcodePtr->link;
				opcodePtr->chk = 0;
			}
		}
	}
}

void freeHashTableBp(bpNode **hashTable, short int maxLen) {
	short int i;	// for loop용 인덱스
	bpNode *opcodePtr, *nextPtr;	// opcode를 저장할 구조체에 대한 포인터

	for (i = 0; i < maxLen; ++i) {	// 해시 테이블 탐색
		if (hashTable[i]) {	// 버킷에 노드가 있다면
			for (opcodePtr = hashTable[i]; opcodePtr; opcodePtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = opcodePtr->link;
				free(opcodePtr);

			}
		}
		hashTable[i] = NULL;
	}

	printf("[ok] clear all breakpoints\n");
	addBp(1, 0, 0, 0, 0);
	bpRoot = NULL;
}

// 2주차
/* ========== 1. shell 관련 명령어 ========== */
// 명령문에서 파일명을 추출해낸다.
char *getFileName(short int *fileType) {
	char *curBuf = modCurBuf(NULL);	// buffer에서 현재 커서 위치
	char *fileName = (char *)malloc(sizeof(char)*(MAXCMD + 1));	// 인자를 받을 변수
	char *filePtr = fileName;	// 인자 문자열 탐색용 포인터
	short int chCnt = 0;	// 인자의 길이

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 앞의 공백문자 skip
		++curBuf;

	while (*curBuf != EOB && !isspace(*curBuf)) {	// 인자 받기
		*filePtr = *curBuf;
		++filePtr; ++curBuf; ++chCnt;
		if (chCnt == MAXCMD)	// 인자 길이의 최댓값에 도달하면
			break;	// copy를 중단한다.
	}

	while (*curBuf != EOB && isspace(*curBuf)) 		// 인자 뒤 공백문자 skip
		++curBuf;

	modCurBuf(curBuf);	// 인자 추출을 위한 커서 이동이 끝났으면 갱신한다.

	if (fileName == filePtr) {	// 인자가 없거나 alnum이 아니라면
		cmdError(NO_OP);	// error
		return NULL;
	}

	if (!isCmdEnd()) {	// fileName인자는 홀로 쓰이는 인자인데 인자가 남아 있다면
		cmdError(EXCESS);	// error
		return NULL;
	}

	*filePtr = '\0';	// 인자에 null 문자 append
	
	*fileType = (strcmp(filePtr - 4, ".asm")==0)?1:0;	// 확장자가 asm이면 filetype에 1 아니면 0 할당
	
	return fileName;
}

// 파일의 내용을 출력한다.
void type(char *fileName) {
	FILE *fp = fopen(fileName, "r");
	int c;

	if (fp == NULL) {	// 파일 열기 오류
		fprintf(stderr, "error: file open error\n");
		modCmdStatus(INVALID);
		fclose(fp);
		return;
	}

	while ((c = fgetc(fp)) != EOF)	// EOF까지 문자를 불러들여
		putchar(c);	// 출력한다.

	putchar('\n');
	fclose(fp);
	return;
}

// sybol 관련 명령문 출력, error counter가 쌓이면 pass의 main loop을 중지한다.
short int symErr(short int mode, short int errCode, int lineCnt) {
	static short int errCnt = 0;

	if (mode == INIT_CNT) {			// assemble 시작 전 error counter를 초기화한다.
		errCnt = 0;
		return 0;
	}
	else if (mode == READ_CNT) {	// error counter를 반환하며, pass 내부에서 하나 이상일 경우 main loop을 종료한다.
		return errCnt;
	}
	else if (mode == PROC_ERR) {	// 에러코드에 상응하는 오류문 출력
		++errCnt;

		if(lineCnt >= 0)
			fprintf(stderr, "error on line %d: ", lineCnt);
		switch (errCode) {
		case DUPLICATE:
			fprintf(stderr, "symbol duplication\n"); break;
		case UNKNOWN:
			fprintf(stderr, "neither operation, directive nor comment.\n");
			fprintf(stderr, "Choose 1 among the above\n"); break;
		case ATTNAME:
			fprintf(stderr, "symbol or operation code name error\n");
			fprintf(stderr, "Symbol must start with alphabet letter.\n");
			fprintf(stderr, "Operation code must consist of uppercase letter.\n"); break;
		case OUT_OF_BOUND:
			fprintf(stderr, "address out of bound\n"); break;
		case REG:
			fprintf(stderr, "invalid register\n");
			fprintf(stderr, "Choose among the following;\n");
			fprintf(stderr, "A, X, L, B, S, T, F, PC, SW\n"); break;
		case PLUS:
			fprintf(stderr, "'+' can be prefix only to operation\n"); break;
		case OP_VAL:
			fprintf(stderr, "invalid operand or value\n"); break;
		case OPCODE_TABLE:
			fprintf(stderr, "wrong syntax for the operation\n");
			fprintf(stderr, "Consult opcode table for valid syntax.\n"); break;
		case DIREC_SYNTAX:
			fprintf(stderr, "wrong syntax for the directive\n"); break;
		case DEF_REC:
			fprintf(stderr, "definition record must consist of multiples of following:\n");
			fprintf(stderr, "1. 6 letters includig white space for symbol name\n");
			fprintf(stderr, "2. 6 digits indicating the \n");  break;
		case UNKNOWN_SYM:
			fprintf(stderr, "encountered symbol not included in ESTAB\n"); break;
		case WRONG_REC_LEN:
			fprintf(stderr, "value of record length field doesn't match actual record length\n"); break;
		case OPCODE_FORM_MISMATCH:
			fprintf(stderr, "opcode doesn't match format indicated by x bit\n"); break;
		case MOD_REC_SIGN:
			fprintf(stderr, "modification record must have 10th column value as sign\n"); break;
		case REF_REC:
			fprintf(stderr, "invalid reference record\n"); break;
		}
		return errCnt;
	}
	return 0;
}

// symtab에 sym인자가 있는지 확인하고 있다면 location을, 없다면 -1 반환, 없다면 새로 symtab에 추가
int addSym(short int pass, int lineCnt, int loc, char *sym) {
	symNode **bucket;	// hash table에 대한 포인터(버킷)
	symNode *tempSym, *prevSym;	// 버킷 내 탐색을 위한 커서
	symNode *symPtr;	// 심볼 구조체에 대한 포인터
	short int status;	// 비교 결과

	if (!('A' <= sym[0] && sym[0] <= 'z'))
		return -1;

	bucket = symTab + sym[0] - 'A';	// 첫 글자의 ASCII 값을 해시 테이블 인덱스로 활용

	symPtr = (symNode *)malloc(sizeof(symNode));	// 심볼 구조체 포인터에 메모리 동적 할당

	// 구조체에 멤버 할당하기
	symPtr->loc = loc;
	symPtr->sym = sym;
	symPtr->link = NULL;

	if (*bucket == NULL) {// 버킷이 비어있을 경우
		if (pass == 1)
			*bucket = symPtr;//		버킷의 첫번째 symbol로 할당
	}
	else {					// 버킷에 기존 symbol이 존재할 경우
		prevSym = NULL;
		tempSym = *bucket;
		while (tempSym != NULL) {	// 버킷 내 다른 노드가 존재하면 정렬하기
			status = strcmp(tempSym->sym, sym);

			if (status > 0)
				break;
			else if (status == 0) {
				free(symPtr);
				if (pass == 1)	// pass1인데 중복되는 노드 발견
					symErr(PROC_ERR, DUPLICATE, -1);
				return tempSym->loc;
			}

			prevSym = tempSym;
			tempSym = tempSym->link;
		}

		if (pass == 1) {
			if (tempSym != *bucket) {	// symbol이 두번째 노드 이후에 삽입되는 경우
				prevSym->link = symPtr;
				symPtr->link = tempSym;
			}
			else {	// symbol이 첫번째 노드에 삽입되는 경우
				symPtr->link = *bucket;
				*bucket = symPtr;
			}
		}
	}
	if(pass == 2)
		symErr(PROC_ERR, UNKNOWN_SYM, -1);

	return -1;	// symbol 추가 성공
}

// symbol table을 출력한다.
void symbol() {
	short int i;	// for loop용 인덱스
	symNode *symPtr;

	for (i = 0; i < SYMTABSIZE; ++i) {	// 해시 테이블 탐색
		if (symTab[i])	// 버킷에 노드가 있다면
			for (symPtr = symTab[i]; symPtr; symPtr = symPtr->link)	// 버킷의 마지막 노드까지 탐색하여
				printf("\t%s\t%.4X\n", symPtr->sym, symPtr->loc);	// 내용 출력
	}
}

// symbol table의 메모리 할당을 해제한다.
void freeSymbol() {
	short int i;	// for loop용 인덱스
	symNode *symPtr, *nextPtr;

	for (i = 0; i < SYMTABSIZE; ++i) {	// 해시 테이블 탐색
		if (symTab[i] != NULL) {	// 버킷에 노드가 있다면
			for (symPtr = symTab[i]; symPtr; symPtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = symPtr->link;
				free(symPtr->sym);	// symbol free
				free(symPtr);	// 자료구조 free
			}
			symTab[i] = NULL;	// symTabl을 NULL값으로 초기화한다.
		}
	}
}

// symbol table의 메모리 할당을 해제한다.
void freeEst() {
	short int i;	// for loop용 인덱스
	extSymNode *symPtr, *nextPtr;

	for (i = 0; i < MAXESTABSIZE; ++i) {	// 해시 테이블 탐색
		if (esTab[i] != NULL) {	// 버킷에 노드가 있다면
			for (symPtr = esTab[i]; symPtr; symPtr = nextPtr) {	// 버킷의 마지막 노드까지 탐색하여
				nextPtr = symPtr->link;
				free(symPtr->name);	// symbol free
				free(symPtr);	// 자료구조 free
			}
		}
		esTab[i] = NULL;	// symTabl을 NULL값으로 초기화한다.
	}
}

// directive 목록을 찾아서 목록이 있으면 directive code를 그렇지 않다면 -1을 반환
short int directive(char *direct) {
	static char *dirTab[] = { "START", "END", "BYTE", "WORD", "RESB", "RESW", "BASE", "NOBASE", "LTORG", "EQU", "ORG", "\0" };
	short int i;

	if (direct == NULL)
		return -1;

	for (i = 0; dirTab[i][0] != '\0'; ++i)
		if (strcmp(direct, dirTab[i]) == 0)	// directive 목록에 존재한다면
			return i;	// 해당 코드를 반환

	return -1;	// 아니라면 -1을 반환
}

// strtok와 동일, comma는 쉼표 문법 검사용 인자
char *tok(char *newSrc, short int *comma) {
	static char *srcRoot;
	static int start = 0, end = 0;
	static int i;
	short int tempComma = 0;
	
	char *ret;

	// 새로운 문자열 받으면 srcRoot을 해당 문자열의 주소로 갱신하여 언제든 접근할 수 있도록 한다.
	if (newSrc != NULL) {
		srcRoot = newSrc;
		i = start = end = 0;
	}

	// 전처리(word 전 공백 및 쉼표 skip)
	for (; srcRoot[i] && (isspace(srcRoot[i]) || (tempComma = srcRoot[i] == ',')); ++i)
		if (tempComma == 1)
			*comma = 1;
	if(srcRoot[i])	start = i;
	else return NULL;

	// 중처리(word 처리)
	for (; srcRoot[i] && (!isspace(srcRoot[i]) && !(tempComma = srcRoot[i] == ',')); ++i)
		if (tempComma == 1)
			*comma = 1;
	end = i-1;

	ret = (char *)calloc(end - start + 2, sizeof(char));
	strncpy(ret, srcRoot + start, end - start + 1);

	return ret;
}

// token에다가 순서대로 저장 line에다 의미순 대로 저장
//		tokenize 이후에도 line 필요하면 temp 변수 선언 필요
//		token은 선언시 0으로 초기화
void tokenize(short int pass, char *line, int lineCnt, int *loc, short int *recType, char **record, short int *directCode, short int *form, short int *addrFlag) {
	char *tempLine;
	char *token[MAXATTNO + 1] = { 0 };	// token은 line에서 등장하는 순서대로 저장, record는 역할 순서대로 저장
	char *tempToken;
	short int tknCtr;
	short int operationOffset = 0;
	short int operandOffset = 0;
	short int comma = 0;
	short int commaTime = 0;
	short int isComma = 0;

	*directCode = NONE;

	// line을 tempLine에 복사
	tempLine = (char *)malloc(sizeof(char)*(strlen(line) + 1));
	strcpy(tempLine, line);

	// pass 2인 경우, 첫번째와 두번쨰 애트리뷰트가 각각 line counter, location이다.
	// location만 갱신하고 둘 다 pass에서 사용되지 않으므로 free한다.
	if (pass == 2) {
		tok(tempLine, &comma);	// lineCnt column 버리기
		tempToken = tok(NULL, &comma);
		if (tempToken[0] == '.') {		// comment인 경우
			*recType = COMMENT;
			return;
		}
		else if (tempToken && strcmp(tempToken, "BASE") == 0)	// BASE directive인 경우
			token[0] = tempToken;
		else {
			*loc = strtol(tempToken, NULL, 16);	// location 갱신
			token[0] = tok(NULL, &comma);
		}
	}
	else
		token[0] = tok(tempLine, &comma);

	for (tknCtr = 1; tknCtr < MAXATTNO; ++tknCtr) {
		token[tknCtr] = tok(NULL, &comma);
		if (comma == 1 && commaTime == 0)
			commaTime = tknCtr;
	}

	// determine recType
	// find mnemonic
	addrFlag[E] = 0;
	for (tknCtr = 0; tknCtr < MAXATTNO; ++tknCtr) {
		tempToken = token[tknCtr];
		if (tempToken != NULL && tempToken[0] == '+') {	// 첫 글자가 +라면 두번째 글자부터 operation인지 검사할 수 있도록 offset을 설정한다.
			operationOffset = 1;	// offset 설정
			addrFlag[E] = 1;	// +가 operation 앞에 있으면 extended
		}
		if (opcode(tempToken + operationOffset, form) >= 0)	// OPERATION
			break;
		else
			//err 첫 글자가 +가 될 수 없음
			addrFlag[E] = 0;	// OPERATION 아니면 E addrFlag 다시 reset
	}

	if (tknCtr < MAXATTNO) {	// operation이 존재하면
		*recType = INSTRUCT;	// 이 레코드의 유형은 번역 대상인 instruction

		addrFlag[N] = addrFlag[I] = 1;	// simple addressing으로 초기화
		addrFlag[X] = 0;	// unindexed로 초기화

		record[OPERATION] = (char *)malloc(sizeof(char)*(strlen(token[tknCtr]) + 1));	// token[tknCtr]에서 operation이 발견됨
		strcpy(record[OPERATION], token[tknCtr] + operationOffset);	// record의 OPERATION 애트리뷰트에 복사

		if (tknCtr >= 1 && !isdigit(token[tknCtr - 1][0])) {	
			record[SYMBOL] = (char *)malloc(sizeof(char)*(strlen(token[tknCtr - 1]) + 1));	// operation 전에 불러들인 word가 있다면
			strcpy(record[SYMBOL], token[tknCtr - 1]);	// symbol이므로 record의 SYMBOL 애트리뷰트에 복사
		}

		if (tknCtr <= MAXATTNO - 2 && token[tknCtr + 1] != NULL) {	// 만약 operation 다음에 불러온 word가 있다면
			// prefix 선처리: addressing 모드를 나타내는 특수문자가 있다면 다음 문자부터 처리할 수 있도록
			if (token[tknCtr + 1][0] == '@') {
				operandOffset = 1;
				addrFlag[N] = 1; addrFlag[I] = 0;
			}
			else if (token[tknCtr + 1][0] == '#') {
				operandOffset = 1;
				addrFlag[N] = 0; addrFlag[I] = 1;
			}

			record[OPERAND1] = (char *)malloc(sizeof(char)*(strlen(token[tknCtr + 1]) + 1));	// 첫번째 operand를
			strcpy(record[OPERAND1], token[tknCtr + 1] + operandOffset);	// record의 OPERAND1 애트리뷰트에 복사
		}

		if (tknCtr <= MAXATTNO - 3 && token[tknCtr + 2] != NULL) {
			if (token[tknCtr + 2][0] == 'X')
				addrFlag[X] = 1;
			else {
				record[OPERAND2] = (char *)malloc(sizeof(char)*(strlen(token[tknCtr + 2]) + 1));	// 두번째 operand가 있다면
				strcpy(record[OPERAND2], token[tknCtr + 2]);	// record의 OPERAND2 애트리뷰트에 복사
				if (commaTime == tknCtr + 2)
					isComma = 1;
			}
		}
	}
	// comment라면
	else if ((token[0] && token[0][0] == '.') || (token[1] && token[1][0] == '.') || (token[2] && token[2][0] == '.'))
		*recType = COMMENT;
	// directive라면
	else {
		*recType = DIRECTIVE;
		for (tknCtr = 0; tknCtr < MAXATTNO; ++tknCtr)
			if (token[tknCtr] && (*directCode = directive(token[tknCtr])) != -1) {
				if (tknCtr <= MAXATTNO - 2 && token[tknCtr + 1] != NULL) {
					record[VALUE] = (char *)malloc(sizeof(char)*(strlen(token[tknCtr + 1]) + 1));	// directive의 oeprand를 저장
					strcpy(record[VALUE], token[tknCtr + 1]);
				}
				if (tknCtr >= 1 && !isdigit(token[tknCtr - 1][0])) {
					record[SYMBOL] = (char *)malloc(sizeof(char)*(strlen(token[tknCtr - 1]) + 1)); // directive 전 word인 sybol을 저장
					strcpy(record[SYMBOL], token[tknCtr - 1]);
				}
				break;
			}

		// word의 개수는 최대치를 넘을 수 없다.
		if (tknCtr >= MAXATTNO) {
			*recType = ERR;
			*directCode = -1;

			symErr(PROC_ERR, UNKNOWN, lineCnt);
			return;
		}
	}

	// instruction인 경우
	if (*recType == INSTRUCT) {
		// 1형식의 oeprand 검사
		if (*form == 1) {
			if (record[OPERAND1] != NULL || record[OPERAND2] != NULL) {
				symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
			}
		}
		// 2형식의 operand 검사
		else if (*form == 2) {
			if (strcmp(record[OPERATION], "TIXR") == 0) {
				if (getReg(lineCnt, record[OPERAND1]) == -1) {	// 첫번째 operand가 레지스터 목록에 없다면
					symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;	// 문법 오류
				}
			}
			else if (strcmp(record[OPERATION], "CLEAR") != 0 && (record[OPERATION][strlen(record[OPERATION]) - 1] == 'R' || strcmp(record[OPERATION], "RMO") == 0 || strcmp(record[OPERATION], "SHIFTL") == 0)) {
				if (record[OPERAND1] == NULL || record[OPERAND2] == NULL || isComma == 0) {
					symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
				}
				if (strncmp(record[OPERATION], "SHIFT", 5) == 0) {
					if (getReg(lineCnt, record[OPERAND1]) == -1) {
						symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
					}
				}
				else if (getReg(lineCnt, record[OPERAND1]) == -1 || getReg(lineCnt, record[OPERAND2]) == -1) {
					symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
				}
			}
			else {
				if (record[OPERAND1] == NULL) {
					symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
				}
			}
		}
		// 3형식과 4형식의 oeprand 검사
		else if (*form == 3 || *form == 4) {
			// "RSUB"은 3형식이지만 operand가 없어야 한다.
			if (strcmp(record[OPERATION], "RSUB") == 0) {
				if (record[OPERAND1] != NULL || record[OPERAND2] != NULL) {
					symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
				}
			}
			else {
				if (record[OPERAND1] == NULL) {
					symErr(PROC_ERR, OPCODE_TABLE, lineCnt); return;
				}
			}
		}
	}
	// directive인 경우
	else if (*recType == DIRECTIVE) {
		switch (*directCode) {
		case BASE: case ORG:	// operand인 value만을 가지는 경우
			if (record[SYMBOL] != NULL || record[VALUE] == NULL) {
				symErr(PROC_ERR, DIREC_SYNTAX, lineCnt); return;
			}break;
		case BYTE: case WORD: case EQU: case RESB: case RESW:	// value와 더불어 symbol도 가져야 하는 경우
			if (record[SYMBOL] == NULL || record[VALUE] == NULL) {
				symErr(PROC_ERR, DIREC_SYNTAX, lineCnt); return;
			}break;
		}
	}

	// 동적 메모리 해제
	for (tknCtr = 0; tknCtr < MAXATTNO; ++tknCtr)
		if (token[tknCtr] != NULL)
			free(token[tknCtr]);

	free(tempLine);
	return;
}

// pass1
short int pass1(char *fileName, int *progLen) {
	FILE *inFp, *imFp;

	// tokenize 용 인자
	char line[MAXLINE + 1];
	int lineCnt = 0;
	short int recType;
	char *record[MAXRECSIZE + 1] = { 0 };
	short int directCode;
	short int form;
	short int addrFlag[MAXADDRFLAG] = { 0 };	// n, i, x, b, p, e bit값을 저장하기 위한 배열

	int startAddr;
	int loc = 0;

	short int i;

	remove(INTERMEDIATE);

	inFp = fopen(fileName, "r");
	imFp = fopen(INTERMEDIATE, "w");

	// file open error
	if (inFp == NULL || imFp == NULL) {
		fprintf(stderr, " error: %s file open\n", (inFp == NULL)?"input":"intermediate");
		return 0;
	}

	/* read first input line */
	fgets(line, MAXLINE, inFp);
	lineCnt += 5;
	tokenize(1, line, lineCnt, NULL, &recType, record, &directCode, &form, addrFlag);

	if (directCode == START) {
		startAddr = strtol(record[VALUE], NULL, 16);	/* save #[OPERAND] as starting address */
		loc = startAddr;	/* initialize LOCCTR to starting address */

		fprintf(imFp, "%d\t%X\t%s", lineCnt, loc, line);	/* write line to intermediate file */

		/* record 동적 할당 해제 후 NULL 할당 */
		for (i = 0; i<MAXRECSIZE; ++i)
			if (record[i]) {
				if (i != SYMBOL)			// symbol이 존재하는 경우 attribute 통째로 symtab에 들어가기 때문에 free하지 말 것
					free(record[i]);
				record[i] = NULL;
			}

		/* read next input line */
		fgets(line, MAXLINE, inFp);
		lineCnt += 5;
		tokenize(1, line, lineCnt, NULL, &recType, record, &directCode, &form, addrFlag);
	}
	else
		loc = 0;	/* initialize LOCCTR to 0 */

	while (directCode != END) {
		if (symErr(READ_CNT, 0, 0) > 0) {
			fclose(inFp);
			fclose(imFp);
			return 0;
		}

		/* write line to intermediate file */
		if(recType == COMMENT || directCode == BASE)
			fprintf(imFp, "%d\t\t%s", lineCnt, line);	
		else
			fprintf(imFp, "%d\t%X\t%s", lineCnt, loc, line);	

		if (recType != COMMENT) {
			/* search symbol and add to symtab if valid */
			if (record[SYMBOL] != NULL && addSym(1, lineCnt, loc, record[SYMBOL]) >= 0)	// symbol이 존재하면 
				symErr(PROC_ERR, DUPLICATE, lineCnt);

			/* update LOCCTR(loc) */
			//@ 문법 검사 필요하지 않나? 문법 요소(operand)가 갖춰져야 아래 내용 valid
			if (recType == INSTRUCT) {	// 명령문은 형식에 따라서 사이즈 결정됨
				if (addrFlag[E] == 1)
					loc += 4;
				else if (form == 2)
					loc += 2;
				else
					loc += 3;
			}
			else if (recType == DIRECTIVE)	// directive는 할당되는 메모리에 따라서 사이즈 결정됨
				switch (directCode) {
				case WORD:
					loc += 3; break;
				case RESW:
					loc += 3 * strtol(record[VALUE], NULL, 10); break;
				case RESB:
					loc += strtol(record[VALUE], NULL, 10); break;
				case BYTE:
					switch (record[VALUE][0]) {
					case 'X':	//@ 검증 필요
						loc += (strlen(record[VALUE]) - 3) / 2; break;
					case 'C':
						loc += (strlen(record[VALUE]) - 3); break;
					default:	//@ 검증 필요
						loc += 3; break;
					}
				}
			/* set error flag (invalid operation code) */	//@
		}

		/* record 동적 할당 해제 후 NULL 할당 */
		for (i = 0; i<MAXRECSIZE; ++i)
			if (record[i] != NULL) {
				if (i != SYMBOL)			// symbol이 존재하는 경우 attribute 통째로 symtab에 들어가기 때문에 free하지 말 것
					free(record[i]);
				record[i] = NULL;
			}

		/* read next input line */
		fgets(line, MAXLINE, inFp);
		lineCnt += 5;
		tokenize(1, line, lineCnt, NULL, &recType, record, &directCode, &form, addrFlag);
	}

	fprintf(imFp, "%d\t%X\t%s", lineCnt, loc, line);	/* write line to intermediate file */

	/* record 동적 할당 해제 */
	for (i = 0; i<MAXRECSIZE; ++i)
		if (record[i]) {
			free(record[i]);
		}


	*progLen = loc - startAddr;	/* save (LOCCTR - startin address) as program length */

	fclose(inFp);
	fclose(imFp);

	return 1;
}

// register 이름을 담은 문자열을 인자로 받아, 실제 존재하는 레지스터면 상응하는 코드를, 그렇지 않으면 -1을 반환
short int getReg(int lineCnt, char *reg) {
	if (reg[0] != 'S' && reg[1] == 'P' && reg[1] != '\0') {
		symErr(PROC_ERR, REG, lineCnt);
		return -1;
	}

	switch (reg[0]) {
	case 'A':
		return 0;
	case 'X':
		return 1;
	case 'L':
		return 2;
	case 'B':
		return 3;
	case 'S':
		if (reg[1] != '\0') {
			if (reg[1] == 'W') {
				return 9;
			}
			else {
				symErr(PROC_ERR, REG, lineCnt);
				return -1;
			}
		}
		return 4;
	case 'T':
		return 5;
	case 'F':
		return 6;
	case 'P':
		if (reg[1] != 'C') {
			symErr(PROC_ERR, REG, lineCnt);
			return -1;
		}
		return 8;
	}
	return -1;
}

// tokenize 함수의 결과물을 받아 objCode를 생성, 반환한다.
char *genObjCode(int lineCnt, int loc, char **record, int opAddr, short int *addrFlag, int *objFlag, int *offset) {
	// instruction format:
	//       first seg   | second seg | third seg
	//	  1st byte,  2nd |     3rd    |  4th~		
	//		opCode, n, i | x, b, p, e | opAddr
	short int opCode;
	short int form;
	char ni[3];
	char xbpe[5];
	char *seg[3] = { 0 };
	char *obj;

	int pc, base;
	int disp;

	short int i;

	// prepare opCode
	opCode = opcode(record[OPERATION], &form);

	// prepare opAddr & set B, P flag
	if (record[OPERAND1] != NULL) {
		if (addrFlag[E] == 0) {
			addrFlag[B] = 0;
			if (opAddr == -1) {
				addrFlag[P] = 0;
				opAddr = strtol(record[OPERAND1], NULL, 10);
			}
			else
				addrFlag[P] = 1;
		}
		else {
			addrFlag[B] = addrFlag[P] = 0;
			if (opAddr == -1)
				opAddr = strtol(record[OPERAND1], NULL, 10);
		}
	}

	// prepare n, i
	ni[N] = addrFlag[N] + '0'; ni[I] = addrFlag[I] + '0';

	// prepare x, b, p, e
	XBPE[X] = addrFlag[X] + '0'; XBPE[B] = addrFlag[B] + '0'; XBPE[P] = addrFlag[P] + '0'; XBPE[E] = addrFlag[E] + '0';

	// prepare format
	if (form == 3 && addrFlag[E] == 1)
		form = 4;
	
	// prepare obj
	seg[0] = (char *)malloc(sizeof(char) * 3);
	seg[1] = (char *)malloc(sizeof(char) * 2);
	seg[2] = (char *)malloc(sizeof(char) * 6);

	// 예외적인 경우 먼저 처리: RSUB, 2형식
	if (record[OPERAND1] == NULL) {
		obj = (char *)malloc(sizeof(char)*((XBPE[E] == '1') ? 9 : 7));
		snprintf(obj, (XBPE[E] == '1') ? 9 : 7, (XBPE[E] == '1') ? "%.2X000000" : "%.2X0000", (unsigned int)(opCode + strtol(ni, NULL, 2)));
		return obj;
	}
	else if (form == 2) {
		obj = (char *)malloc(sizeof(char) * 5);
		snprintf(obj, 3, "%.2X", opCode);
		if (record[OPERAND2] != NULL) {
			snprintf(obj + 2, 2, "%.1X", getReg(lineCnt, record[OPERAND1]));
			snprintf(obj + 3, 2, "%.1X", getReg(lineCnt, record[OPERAND2]));
		}
		else
			snprintf(obj + 2, 3, "%.1X0", getReg(lineCnt, record[OPERAND1]));
		return obj;
	}

	// direct addressing
	if (XBPE[B] == '0' && XBPE[P] == '0') {
		if (opAddr < 0) {
			symErr(PROC_ERR, OP_VAL, lineCnt);
			return NULL;
		}

		// 바로 object code를 만들어 반환
		snprintf(seg[0], 3, "%.2X", (unsigned int)(opCode + strtol(ni, NULL, 2)));
		snprintf(seg[1], 2, "%.1X", (unsigned int)strtol(xbpe, NULL, 2));
		snprintf(seg[2], (XBPE[E] == '1') ? 6 : 4, (XBPE[E] == '1')?"%.5X":"%.3X", (unsigned int)opAddr);
	}
	else {
		// 형식별 pc 계산
		if (form == 2)
			pc = loc + 2;
		else if (XBPE[E] == '1')
			pc = loc + 4;
		else
			pc = loc + 3;
		
		// BASE directive에 의해 지시된 base값 가져오기
		base = objFlag[BASE_REL];

		// 첫번째 segment: operation code + n, i 레지스터 값
		snprintf(seg[0], 3, "%.2X", (unsigned int)(opCode + strtol(ni, NULL, 2)));

		// pc relative인 경우 displacement 계산: Target Address - pc
		disp = opAddr - pc;

		if (PC_LOWER <= disp && disp <= PC_UPPER) {	// PC relative의 표현 범위 내라면
			// PC relative
			snprintf(seg[1], 2,  "%.1X", (unsigned int)strtol(xbpe, NULL, 2));
			snprintf(seg[2], (XBPE[E] == '1') ? 6 : 4, (XBPE[E] == '1') ? "%.5X" : "%.3X", ((unsigned int)disp) % ((XBPE[E] == '1') ? 1<<20: 1<<12));	// 2's complement
		}
		else{
			disp = opAddr - base;

			if (BASE_LOWER <= disp && disp <= BASE_UPPER) {	// base relative의 표현 범위 내라면
				//base relative
				XBPE[B] = '1'; XBPE[P] = '0';

				snprintf(seg[1], 2, "%.1X", (unsigned int)strtol(xbpe, NULL, 2));
				snprintf(seg[2], (XBPE[E] == '1') ? 6 : 4, (XBPE[E] == '1') ? "%.5X" : "%.3X", (unsigned int)disp);
			}
			else {	// 두 방법 모두 안 되면 extended로
				disp = opAddr - pc;			
					
				if (EXD_LOWER <= disp && disp <= EXD_UPPER) {// extended 조건 범위
					// extended(원래는 아니었는데)
					XBPE[B] = 0; XBPE[P] = 1;
					XBPE[E] = '1';

					++pc;	// extended에 맞게 pc를 수정한다.
					*offset = *offset + 1;	// location 수정용 offset을 calling function에 반환한다.
					
					snprintf(seg[1], 2, "%.1X", (unsigned int)strtol(xbpe, NULL, 2));
					snprintf(seg[2], 6, "%.5X", disp);
				}
				else {
					symErr(PROC_ERR, OUT_OF_BOUND, lineCnt);
				}
			}	
		}
	}

	// segment들을 합쳐서 object code 완성하기
	obj = (char *)malloc(sizeof(char)*((XBPE[E] == '1') ? 9: 7));
	obj[0] = '\0';
	for (i = 0; i < 3; ++i) {
		if (seg[i] != NULL) {
			strcat(obj, seg[i]);
			free(seg[i]);
		}
	}

	// 수정한 flag 값들을 calling fuction에 적용하기
	addrFlag[X] = XBPE[X] - '0'; addrFlag[B] = XBPE[B] - '0'; addrFlag[P] = XBPE[P] - '0'; addrFlag[E] = XBPE[E] - '0';

	return obj;
}

// pass1이 성공한 경우 pass2
short int pass2(char *fileName, int progLen) {
	FILE *imFp, *lstFp, *objFp;
	
	short int fileNameLen;
	char *lstFileName;
	char *objFileName;
	int startAddr;

	// tokenize 용 인자
	char line[MAXLINE + 1];
	int lineCnt = 0;
	int loc;
	short int recType;
	char *record[MAXATTNO] = { 0 };
	short int directCode;
	short int form;
	short int addrFlag[MAXADDRFLAG] = { 0 };	// n, i, x, b, p, e bit값을 저장하기 위한 배열
	int objFlag[MAXOBJFLAG];
	int objColCnt = 0;
	int objColIncre;
	int startLoc;
	int endLoc;
	char txtRecBuf[MAX_TXT_REC + 1] = { 0 };

	// obj 계산용
	int opAddr;
	char *valHex = NULL;
	char oneHex[3];
	int offset = 0;
	char *objOperation = NULL;

	short int i;
	
	fileNameLen = strlen(fileName);

	lstFileName = (char *)malloc(sizeof(char)*(fileNameLen + 1));
	objFileName = (char *)malloc(sizeof(char)*(fileNameLen + 1));

	strcpy(lstFileName, fileName);
	strcpy(objFileName, fileName);

	remove(lstFileName);
	remove(objFileName);

	imFp = fopen(INTERMEDIATE, "r");
	lstFp = fopen(strcat(lstFileName, ".lst"), "w");
	objFp = fopen(strcat(objFileName, ".obj"), "w");

	if (imFp == NULL || lstFp == NULL || objFp == NULL) {
		fprintf(stderr, " error: %s file open\n", (imFp == NULL) ? "intermediate" : ((lstFp == NULL) ? "list" : "object"));
		return 0;
	}

	// file open error 처리

	/* read first input line */
	fgets(line, MAXLINE, imFp);
	line[strlen(line) - 1] = '\0';
	lineCnt += 5;
	tokenize(2, line, lineCnt, &loc, &recType, record, &directCode, &form, addrFlag);

	startAddr = loc;

	/* write Header record to object program */
	fprintf(objFp, "H%-6.6s%.6X%.6X\n", record[SYMBOL], (unsigned int)strtol(record[VALUE], NULL, 16), (unsigned int)progLen);

	if (directCode == START) {
		fprintf(lstFp, "%s\n", line);	/* write listing line */
		
		/* record 동적 할당 해제 후 NULL 할당 */
		for (i = 0; i<MAXRECSIZE; ++i)
			if (record[i] != NULL) {
				if (i != SYMBOL)			// symbol이 존재하는 경우 attribute 통째로 symtab에 들어가기 때문에 free하지 말 것
					free(record[i]);
				record[i] = NULL;
			}

		/* read next input line */
		fgets(line, MAXLINE, imFp);
		line[strlen(line) - 1] = '\0';
		lineCnt += 5;
		tokenize(2, line, lineCnt, &loc, &recType, record, &directCode, &form, addrFlag);
	}

	

	/* initialize first text record */

	while (directCode != END) {
		if (symErr(READ_CNT, 0, 0) > 0) {
			fclose(imFp);
			fclose(lstFp);
			fclose(objFp);
			return 0;
		}

		if (recType != COMMENT) {
			if (recType != ERR) {	/* search OPTAB for OPCODE -> if found */
				if (record[OPERAND1] != NULL && (opAddr = addSym(2, lineCnt, loc, record[OPERAND1])) >= 0);	/* search SYMTAB for OPERAND -> if found store symbol value as operand address */
				/* else store 0 as operand address & set error flag */
				else opAddr = -1;

				// instruction일 경우 objOperation에서 object code 생성
				if (recType == INSTRUCT) {
					objOperation = genObjCode(lineCnt, loc, record, opAddr, addrFlag, objFlag, &offset);
				}
				// directive일 경우 value에 맞게 valHex에 object code 생성
				else if (directCode == BYTE || directCode == WORD) {
					switch (record[VALUE][0]) {
					case 'X':
						record[VALUE][strlen(record[VALUE]) - 1] = '\0';
						valHex = (char *)malloc(sizeof(char) * (strlen(record[VALUE] + 1)));
						valHex[0] = 0;
						strcpy(valHex, record[VALUE] + 2); break;
					case 'C':
						record[VALUE][strlen(record[VALUE]) - 1] = '\0';
						valHex = (char *)malloc(sizeof(char) * 7);
						valHex[0] = '\0';
						for (i = 2; record[VALUE][i]; ++i) {
							snprintf(oneHex, 3, "%.2X", (unsigned int)record[VALUE][i]);
							strcat(valHex, oneHex);
						}	break;
					}
				}
				// base일 경우 value를 base flag 변수에 할당
				else if (directCode == BASE) {
					objFlag[BASE_REL] = addSym(2, lineCnt, loc, record[VALUE]);
				}

			}
			// obj code를 파일에 출력
			if (valHex != NULL || objOperation != NULL || directCode == RESB || directCode == RESW) {
				objColIncre = 0;	

				// 현제 object code의 길이를 objCOlIncre; object code 증분에 할당한다.
				if (objOperation != NULL && recType == INSTRUCT) {
					objColIncre = strlen(objOperation);
				}
				else if (valHex != NULL && recType == DIRECTIVE){
					objColIncre = strlen(valHex);
				}

				// 버퍼가 꽉 찼으면
				if (txtRecBuf[0] != '\0' && (objColCnt + objColIncre > MAX_TXT_REC || directCode == RESB || directCode == RESW)) {
					// 초기화
					objColCnt = 0;
					endLoc = loc;
					fprintf(objFp, "%.2X%s\n", endLoc - startLoc, txtRecBuf);
					txtRecBuf[0] = '\0';
				}

				
				if (directCode != RESB && directCode != RESW) {
					// 버퍼가 비었으면 text 레코드의 앞 부분을 작성한다.
					if (objColCnt == 0) {
						startLoc = loc;
						txtRecBuf[0] = '\0';
						fprintf(objFp, "T%.6X", startLoc);
					}

					// 현재 object code만큼 버퍼에 추가한다.
					objColCnt += objColIncre;
					
					if (recType == INSTRUCT) {
						strcat(txtRecBuf, objOperation);
					}
					else {
						strcat(txtRecBuf, valHex);
					}
				}
			}
		}
		/* write listing line */
		
		if (recType == INSTRUCT && objOperation != NULL) {
			fprintf(lstFp, ((form == 2 && record[OPERAND2] == NULL) || record[OPERAND1] == NULL)?"%s\t\t%s\n":"%s\t%s\n", line, objOperation);
			free(objOperation);
			objOperation = NULL;
		}
		else if (recType == DIRECTIVE && valHex != NULL) {
			fprintf(lstFp, "%s\t%s\n", line, valHex);
			free(valHex);
			valHex = NULL;
		}else
			fprintf(lstFp, "%s\n", line);

		/* record 동적 할당 해제 후 NULL 할당 */
		for (i = 0; i<MAXRECSIZE; ++i)
			if (record[i] != NULL) {
				if (i != SYMBOL)			// symbol이 존재하는 경우 attribute 통째로 symtab에 들어가기 때문에 free하지 말 것
					free(record[i]);
				record[i] = NULL;
			}

		/* read next input line */
		fgets(line, MAXLINE, imFp);
		line[strlen(line) - 1] = '\0';
		lineCnt += 5;
		tokenize(2, line, lineCnt, &loc, &recType, record, &directCode, &form, addrFlag);


	}

	endLoc = loc;
	fprintf(objFp, "%.2X%s\n", endLoc - startLoc, txtRecBuf);

	/* write End record to object program */
	fprintf(objFp, "E%.6X", startAddr);

	/* write last lsting line */
	fprintf(lstFp, "%s\n", line);

	fclose(imFp);
	fclose(lstFp);
	fclose(objFp);

	return 1;
}

// "assemble"명령어에 의해 호출되며 pass 전 초기화 및 pass1, pass2 사이의 처리 담당
short int assemble(char *fileName) {
	int progLen;

	// 확장자가 asm이 아닌 파일은 처리하지 않는다.
	if (strcmp(fileName + strlen(fileName) - 4, ".asm") != 0) {
		cmdError(EXTENSION);
		return 0;
	}

	freeSymbol();	//  symbol table 초기화
	symErr(INIT_CNT, 0, 0);	// symbol error counter 초기화

	// pass1이 성공적이라면
	if (pass1(fileName, &progLen) == 0)
		return 0;

	fileName[strlen(fileName) - 4] = '\0';	// 확장자를 제외한 파일명을 pass2에 넘겨준다.
	
	// pass2를 실행한다.
	pass2(fileName, progLen);

	return 1;
}



// 3주차
/* ========== 1. 주소 지정 명령어 ========== */
// 명령문에서 파일명을 추출해낸다.
void addAddr(short int init, short int *addrNum) {
	static addrNode *curAddr = NULL, *prevAddr = NULL;

	if (init == 1) {
		prevAddr = curAddr = NULL;
		return;
	}

	if (*addrNum >= MAXADDRNUM) {
		cmdError(EXCESS);
		return;
	}

	++(*addrNum);
	
	curAddr = (addrNode *)malloc(sizeof(addrNode));
	if (*addrNum == 1)
		rootAddr = curAddr;
	curAddr->addr = getAddr((*addrNum != MAXADDRNUM)?VAR:LAST, MAXLINE);	// 명령문에서 1번째 인자 추출
	curAddr->link = NULL;
	if(prevAddr != NULL)
		prevAddr->link = curAddr;
	prevAddr = curAddr;
}

void freeAddr(void) {
	addrNode *prevAddr, *curAddr = rootAddr;

	for (curAddr = rootAddr; curAddr; curAddr = prevAddr) {
		prevAddr = curAddr->link;
		if(curAddr->addr != NULL)
			free(curAddr->addr);
		free(curAddr);
	}

	rootAddr = NULL;
}

int addExtSym(short int init, short int pass, short int type, char *name, unsigned long int address, int length) {
	static extSymNode *mapPrev = NULL;
	extSymNode **bucket;
	extSymNode *tempSym, *prevSym;
	extSymNode *symPtr;
	short int i;
	int sum = 0;
	short int cmpRet;

	if (init == 1) {
		mapPrev = NULL;
		return -1;
	}

	for (i = 0; i < strlen(name); ++i)
		sum += name[i];

	bucket = esTab + sum % MAXESTABSIZE;

	symPtr = (extSymNode *)calloc(1, sizeof(extSymNode));

	if (mapPrev == NULL)
		mapRoot = symPtr;

	symPtr->type = type;
	symPtr->name = name;
	symPtr->address = address;
	symPtr->length = length;
	symPtr->link = NULL;
	symPtr->mapLink = NULL;

	if (*bucket == NULL) {
		if (pass == 1)
			*bucket = symPtr;
	}
	else {					// 버킷에 기존 symbol이 존재할 경우
		prevSym = NULL;
		tempSym = *bucket;
		while (tempSym != NULL) {	// 버킷 내 다른 노드가 존재하면 정렬하기
			cmpRet = strcmp(tempSym->name, name);

			if (cmpRet > 0)
				break;
			else if (cmpRet == 0) {
				free(symPtr);
				if(pass == 1)	// pass1인데 중복되는 노드 발견
					symErr(PROC_ERR, DUPLICATE, -1);
				return tempSym->address;
			}

			prevSym = tempSym;
			tempSym = tempSym->link;
		}

		if (pass == 1) {
			if (tempSym != *bucket) {	// symbol이 두번째 노드 이후에 삽입되는 경우
				prevSym->link = symPtr;
				symPtr->link = tempSym;
			}
			else {	// symbol이 첫번째 노드에 삽입되는 경우
				symPtr->link = *bucket;
				*bucket = symPtr;
			}
		}
	}

	switch (pass) {
	case 1:
		if (mapPrev != NULL)
			mapPrev->mapLink = symPtr;
		mapPrev = symPtr; break;
	case 2:		// pass2인데 일치하는 노드 찾지 못함
			symErr(PROC_ERR, UNKNOWN_SYM, -1); break;
	}
	return -1;
}

void printExtSym() {
	short int i;	// for loop용 인덱스
	extSymNode *symPtr;
	int lenSum = 0;
	puts("control\tsymbol\taddress\tlength");
	puts("section\tname");
	puts("-------------------------------------");

	for (i = 0; i < MAXESTABSIZE; ++i) {	// 해시 테이블 탐색
		if (esTab[i] != NULL)	// 버킷에 노드가 있다면
			for (symPtr = esTab[i]; symPtr; symPtr = symPtr->link) 	// 버킷의 마지막 노드까지 탐색하여
				switch (symPtr->type) {
				case PROG:
					printf("%s\t\t%x\t%x\n", symPtr->name, (unsigned int)(symPtr->address), (unsigned int)(symPtr->length));
					lenSum += symPtr->length; break;
				case EXTSYM:
					printf("\t%s\t%x\n", symPtr->name, (unsigned int)(symPtr->address)); break;
				}
	}
	printf("\t\ttotal length\t%.4X\n", lenSum);
}

// ESTAB을 입력된 순서대로 출력한다.
void printMap() {
	extSymNode *symPtr;	// 노드를 가리킬 포인터
	int lenSum = 0;		// 프로그램 길이가 저장될 변수
	puts("control\tsymbol\taddress\tlength");
	puts("section\tname");
	puts("-------------------------------------");

	// root 노드를 받아서 NULL이 나올 때까지 탐색
	for (symPtr = mapRoot; symPtr; symPtr = symPtr->mapLink) {	
		switch (symPtr->type) {
		case PROG:
			printf("%s\t\t%.4X\t%.4X\n", symPtr->name, (unsigned int)(symPtr->address), (unsigned int)(symPtr->length));
			lenSum += symPtr->length; break;	// program에 대한 ESTAB 항목이라면 length field를 합산한다.
		case EXTSYM:
			printf("\t%s\t%.4X\n", symPtr->name, (unsigned int)(symPtr->address)); break;
		}
	}
	printf("\t\t\ttotal length\t%.4X\n", lenSum);
}

// head record 처리: ESTAB혹은 REFTAB에 입력하기
char *procHeadRec(short int pass, int *startAddr, char *line, int csaddr, int *cslth) {
	char tmp;
	short int i;
	char *name = (char *)calloc(7, sizeof(char));

	for (i = 5; line[i] == ' '; --i);	/* Header Record의 공백 문자 제거 */

	// 프로그램 이름 추출
	tmp = line[i + 1];
	line[i + 1] = '\0';

	strcpy(name, line);

	line[i + 1] = tmp;

	// 시작 주소 추출
	tmp = line[12];
	line[12] = '\0';

	*startAddr = strtol(line + 6, NULL, 16);

	line[12] = tmp;

	// 프로그램 길이 추출
	line[18] = '\0';

	*cslth = strtol(line + 12, NULL, 16);

	// pass 별로 해당하는 table에 추가
	switch (pass) {
	case 1:
		addExtSym(0, pass, PROG, name, csaddr, *cslth); break;
	case 2:
		addRef(PASS1, 1, name); break;
	}

	return name;
}

// definition record 처리: ESTAB에 입력
short int procDefRec(char *line, int startAddr, int csaddr) {
	short int defCtr;
	char *linePtr = line;
	short int i;
	int addr;
	char tmp;
	char *name;

	// definition record 내 12칸씩 불러들인다.
	for (defCtr = 0; defCtr < MAXDEFPERLINE; defCtr++, linePtr += 12) {
		if (symErr(READ_CNT, 0, 0) > 0)
			return 0;

		// 첫 6칸에서 NULL 포인터가 있으면 end of line이므로 return
		for (i = 0; i < 6; ++i)
			if (linePtr[i] == '\n' || linePtr[i] == '\0')
				return 0;

		// external symbol의 이름
		name = (char *)calloc(7, sizeof(char));

		// 주소 추출
		tmp = linePtr[12];
		linePtr[12] = '\0';

		addr = strtol(linePtr + 6, NULL, 16);

		linePtr[12] = tmp;

		for (i = 5; linePtr[i] == ' '; --i);	/* Header Record의 공백 문자 제거 */
		
		// 이름 추출
		tmp = linePtr[i + 1];
		linePtr[i + 1] = '\0';

		strcpy(name, linePtr);

		linePtr[i + 1] = tmp;

		// ESTAB에 입력
		addExtSym(0, 1, EXTSYM, name, csaddr + addr - startAddr, -1);	// search
	}
	return 1;
}

// linking loader의 pass1
//		강의자료의 psuedo code와 최대한 유사하게 작성하기 위해 노력했다.
short int loadPass1(void) {
	unsigned int csAddr;
	addrNode *addrPtr;
	FILE *inFp;
	char *addr;
	char *fileName;
	char line[MAXOBJLINE + 1];
	int startAddr;
	int cslth;

	csAddr = progAddr;	// get PROGADDR from operatin system
						// set CSADDR to PROGADDR (for first control section)
	for (addrPtr = rootAddr; addrPtr; addrPtr = addrPtr->link) {	// while not end of input
		addr = addrPtr->addr;
		fileName = (char *)calloc((strlen(addr) + 1), sizeof(char));
		strcpy(fileName, addr);

		inFp = fopen(strcat(fileName, ".obj"), "r");

		if (inFp == NULL) {
			fprintf(stderr, "file open error\n");
			fclose(inFp);
			return 0;
		}

		fgets(line, MAXLINE, inFp);	// read next input record (Header record for control section)
		procHeadRec(PASS1, &startAddr, line + 1, csAddr, &cslth);	// set cslth to control section length, search ESTAB for control section name
												// search ESTAB for control section name
		fgets(line, MAXLINE, inFp);	// read next input record
		while (line[0] != 'E') {	// while record type != 'E' do
			if (symErr(READ_CNT, 0, 0) > 0) {
				fclose(inFp);
				return 0;
			}

			if (line[0] == 'D')			// if record type = 'D' then do
				procDefRec(line + 1, startAddr, csAddr);	// for each symbol in the record do

			fgets(line, MAXLINE, inFp);	// read next input record
		}
		csAddr += cslth;
		fclose(inFp);
		//printMap();
	}
	return 1;
}

// text record 처리: memory에 올리기
short int procTxtRec(char *line, int startAddr, int csaddr) {
	char tmp;
	int lineAddr;
	short int lineLen;
	int recByte;
	short int opCode;
	char *linePtr;
	short int xbpe;
	short int extended;
	short int form;
	int value;

	// 해당 text record(line)의 시작주소 추출
	tmp = line[6];
	line[6] = '\0';

	lineAddr = strtol(line, NULL, 16);

	line[6] = tmp;

	// line의 길이 추출
	tmp = line[8];
	line[8] = '\0';

	lineLen = strtol(line + 6, NULL, 16);

	line[8] = tmp;

	// record를 한 byte씩 탐색하며 record에 올리기
	for (recByte = 0; recByte < lineLen; recByte += form) {
		linePtr = line + recByte * 2 + 8;

		// opcode 추출
		tmp = linePtr[2];
		linePtr[2] = '\0';

		opCode = strtol(linePtr, NULL, 16);

		opCode = opCode & 0XFC;

		// opcode로부터 형식 추출
		if (getOpcode(opCode, &form) == NULL) {	// 만약 opcode가 아니라면
			linePtr[2] = tmp;
			// 2칸씩(1byte씩) 읽어들여 메모리에 차례로 올린다.
			for (; *linePtr != '\n'; linePtr += 2, recByte++) {
				if (*(linePtr + 1) == '\n')
					return 1;

				//추출
				tmp = linePtr[2];
				linePtr[2] = '\0';

				//치환
				value = strtol(linePtr, NULL, 16);

				//메모리에 올리기
				edit(csaddr + lineAddr - startAddr + recByte, value);

				linePtr[2] = tmp;
			}
			return 1;
		}

		linePtr[2] = tmp;

		if (form == 3) {
			tmp = linePtr[3];
			linePtr[3] = '\0';

			xbpe = strtol(linePtr + 2, NULL, 16);

			linePtr[3] = tmp;

			// e bit 추출
			extended = (xbpe & 0X01);
		}

		// 3형식과 3형식을 구분한다.
		if (form == 3 && extended) {
			form = 4;
		}

		// 형식의 크기만큼 추출
		tmp = linePtr[form * 2];
		linePtr[form * 2] = '\0';

		// 치환
		value = strtol(linePtr, NULL, 16);

		// 메모리에 올리기
		multiEdit(startAddr, csaddr, lineAddr + recByte, value, form * 2);

		linePtr[form * 2] = tmp;
	}
	return 1;
}

// modification record 추출
short int procModRec(char *line, int startAddr, int csaddr, short int isIndexed, char *progName) {
	char tmp;
	int modeeAddr;
	short int len;
	char sign;
	char *key;
	int augend;
	int addend;
	int replacement;
	unsigned char *mem;
	short int isInst;
	short int i;

	// 수정 대상 주소 추출
	tmp = line[6];
	line[6] = '\0';

	modeeAddr = strtol(line, NULL, 16);

	line[6] = tmp;

	// sign 추출
	sign = line[8];

	line[8] = '\0';

	// length 추출
	len = strtol(line + 6, NULL, 16);

	isInst = len % 2;

	// sign이 없는 경우 프로그램이 하나인 경우이므로
	if (sign == '\n' || sign == '\0' || sign == '\r' || sign == '\v') {
		key = progName;	// 프로그램의 주소를
		sign = '+';		// 더해준다.
	}
	else {	// sign이 있는 경우
		// index 혹은 external symbol 이름을 추출
		key = line + 9;
		for (i = 9; line[i] != '\n' && line[i] != '\0' && line[i] != '\v' && line[i] != '\r'; ++i);
		line[i] = '\0';
	}

	mem = memBase();

	augend = 0;
	
	/* mod rec의 주소 field 상 메모리 참조하여 수정 대상 값(augend)를 불러온다 */
	if (isInst) 
		augend = mem[csaddr + modeeAddr - startAddr] & 0X3;	

	// 피가수 계산(수정 대상 메모리 참조)
	for (i = 0; i < len / 2; ++i) {
		augend = augend << 8;
		augend = augend | mem[csaddr + modeeAddr - startAddr + isInst + i];
	}

	/* mod 피연산자의 주소를 ESTAB에서 fetch */
	if (isIndexed)
		key = addRef(PASS2, strtol(key, NULL, 16), NULL);

	// 가수 계산(수정할 값 계산)
	addend = addExtSym(0, 2, EXTSYM, key, 0, 0);

	// sign에 따라 더하고 뺀 결과 계산
	replacement = (sign == '+') ? augend + addend : augend - addend;

	// 결과를 메모리에 올리기(수정 대상을 덮어씌우기)
	multiEdit(startAddr, csaddr, modeeAddr, replacement, len);	//@ 무조건 3?

	return 1;
}

// reference record 처리
short int procRefRec(char *line, short int *isIndexed) {	// reference record를 R 다음 문자부터 line으로 받는다.
	short int refCtr;
	char *linePtr = line;
	short int i;
	char tmp;
	char *name;
	short int increment;
	short int index;
	short int maxLen;
	short int symLen;

	// 첫 문자가 숫자라면 인덱스를 활용한 refrence이고
	if (isdigit(*linePtr))
		*isIndexed = 1;
	else // 아니면 external symbol의 이름을 기준으로 modify 한다.
		*isIndexed = 0;

	// external symbol만 있는 경우 6자리, index를 활용하면 index를 위한 2자리가 추가되어 8자리를 increment하며 탐색한다.
	switch (*isIndexed) {
	case 1:
		increment = 8;
		maxLen = MAXREFPERLINE_INDEXED; break;
	case 0:
		increment = 6; 
		maxLen = MAXREFPERLINE; break;
	}

	for (refCtr = 0; refCtr < maxLen; ++refCtr, linePtr += increment) {
		if (symErr(READ_CNT, 0, 0) > 0)
			return 0;

		// end of line이면 return
		for (i = 0; isspace(linePtr[i]); ++i);
		if (linePtr[i] == '\0' || linePtr[i] == '\0')
			return 1;

		// external symbol의 이름을 할당할 변수
		name = (char *)calloc(7, sizeof(char));

		// 인덱스 추출
		tmp = linePtr[2];
		linePtr[2] = '\0';

		index = strtol(linePtr, NULL, 16);

		linePtr[2] = tmp;

		// 이름 추출
		tmp = linePtr[8];
		linePtr[8] = '\0';

		if ((symLen = strlen(linePtr + 2)) != 6)
			i = symLen;
		else
			i = 7;



		linePtr[8] = tmp;

		for (; linePtr[i] == ' '; --i);	/* 공백 문자 제거 */

		tmp = linePtr[i + 1];
		linePtr[i + 1] = '\0';

		strcpy(name, linePtr + 2);

		for (i = 0; i < strlen(name); ++i)
			if (name[i] == '\r' || name[i] == '\n') {
				name[i] = '\0';
				symLen = 1;
			}

		linePtr[i + 1] = tmp;

		// 이름과 인덱스를 reftab에 입력
		addRef(PASS1, index, name);

		if (symLen != 6)
			return 1;
	}
	return 1;
}

// symbol table을 출력한다.
void printRef() {
	short int i;	// for loop용 인덱스
	refNode *symPtr;

	printf("print ref\n");

	for (i = 0; i < REFTABSIZE; ++i) {	// 해시 테이블 탐색
		if (refTab[i])	// 버킷에 노드가 있다면
			for (symPtr = refTab[i]; symPtr; symPtr = symPtr->link)	// 버킷의 마지막 노드까지 탐색하여
				printf("\t%s\t%.4X\n", symPtr->symbol, symPtr->index);	// 내용 출력
	}
}

// linking loader의 pass2
//		강의자료의 psuedo code와 최대한 유사하게 작성하기 위해 노력했다.
short int loadPass2(int *execAddr) {
	unsigned int csAddr;
	addrNode *addrPtr;
	FILE *inFp;
	char *addr;
	char *fileName;
	char line[MAXOBJLINE + 1];
	int startAddr;
	int cslth;
	short int isIndexed = 0;
	char *progName;

	csAddr = progAddr;		// set CSADDR to PROGADDR
	*execAddr = progAddr;	// set EXECADDR to PROGADDR

	for (addrPtr = rootAddr; addrPtr; addrPtr = addrPtr->link) {	// while not end of input
		addr = addrPtr->addr;
		fileName = (char *)calloc((strlen(addr) + 1), sizeof(char));
		strcpy(fileName, addr);

		inFp = fopen(strcat(fileName, ".obj"), "r");

		if (inFp == NULL) {
			fprintf(stderr, "file open error\n");
			fclose(inFp);
			return 0;
		}

		fgets(line, MAXLINE, inFp);	// read next input record (Header record for control section)
		progName = procHeadRec(PASS2, &startAddr, line + 1, csAddr, &cslth);	// set cslth to control section length, search ESTAB for control section name

		fgets(line, MAXLINE, inFp);	// read next input record
		while (line[0] != 'E') {	// while record type != 'E' do
			if (symErr(READ_CNT, 0, 0) > 0) {
				fclose(inFp);
				return 0;
			}

			switch (line[0]) {
			case 'T':
				procTxtRec(line + 1, startAddr, csAddr); break;	// mov object code from record to location
			case 'M':
				procModRec(line + 1, startAddr, csAddr, isIndexed, progName); break;
			case 'R':
				procRefRec(line + 1, &isIndexed); break;
			}

			fgets(line, MAXLINE, inFp);
		}
		
		if (line[1] != '\0') {	// if an address is specified (in End record) then
			*execAddr = csAddr + strtol(line + 1, NULL, 16);	// set EXECADDR to (CSADDR + specified adress)
		}
		freeRefSym();

		csAddr += cslth;	// add CSLTH to CSADDR
		fclose(inFp);
	}
	return 1;
	// jump to location given by EXECADDR (to start execution of lad program)
}

// load pass1과 pass2를 수행하며, 초기화 및 중간 처리를 담당한다.
void loader(int *progLen) {
	int execAddr;
	extSymNode *symPtr;

	if (modCmdStatus(NOCHANGE) == INVALID) {	// shell 명령문 분석과정에서 문제가 발생했으면 load 수행하지 않음
		freeAddr();
		addAddr(1, NULL);
		return;
	}

	symErr(INIT_CNT, 0, 0);	// symbol error counter 초기화

	if (loadPass1() == 1 && symErr(READ_CNT, 0, 0) == 0) {
		//printExtSym();
		printMap();
		loadPass2(&execAddr);
	}

	// progLen 계산
	*progLen = 0;
	for (symPtr = mapRoot; symPtr; symPtr = symPtr->mapLink) {
		if (symPtr->type == PROG) {
			*progLen += symPtr->length;
		}
	}

	// 초기화
	freeAddr();		// addr linked list free
	addAddr(1, NULL);	// address linekd list 초기화
	freeEst();		// ESTAB free
	addExtSym(1, 0, 0, NULL, 0, 0);		// ESTAB 초기화
	freeRefSym();	// reftab free
	putchar('\n');
}

// obj파일을 읽는 동안 발생한 에럴르 출력한다.
short int objErr(short int mode, short int errCode) {
	static short int errCnt = 0;
	if (mode == INIT_CNT) {			// assemble 시작 전 error counter를 초기화한다.
		errCnt = 0;
		return 0;
	}
	else if (mode == READ_CNT) {	// error counter를 반환하며, pass 내부에서 하나 이상일 경우 main loop을 종료한다.
		return errCnt;
	}
	else if (mode == PROC_ERR) {	// 에러코드에 상응하는 오류문 출력
		++errCnt;
		fprintf(stderr, "Error occured during run.\n");

		switch (errCode) {
		case REL_ADDR:
			fprintf(stderr, "either b bit or p bit must be 1.\n"); break;
		}
		return errCnt;
	}
	return errCnt;
}

// 메모리상의 데이터에서 opcode를 추출한다.
short int extractOpcode(int loc, char **mnemonic, short int *form, int *reg) {
	unsigned char *mem = memBase();
	short int opCode;
	char *temp;

	// 첫 byte를 읽어와서 mask
	opCode = mem[loc];
	opCode &= 0XFC;

	// opcode에 상응하는 mnemonic 검색
	*mnemonic = (char *)calloc(MAXMNEMONIC, sizeof(char));
	temp = getOpcode(opCode, form);
	strcpy(*mnemonic, temp);
	
	// 3형식과 4형식 구분
	if (*form == 3 && (mem[loc + 1] / 0X10) & 0X1)
		*form = 4;

	// 형식 만큼 pc increment
	reg[PC] += *form;

	return opCode;
}

// 메모리상의 데이터를 읽어온다.
unsigned int getMem(int loc, short int form, short int len) {
	unsigned char *mem = memBase();
	short int i;
	unsigned int ret = 0;

	// format의 각 길이만큼 
	if (form != -1)
		switch(form) {
		case 0:	//SIC
			ret = mem[loc + 1] & 0X7F;
			len = 1;
		case 3:	//SIC/XE 3형식
			ret = mem[loc + 1] & 0XF;
			len = 1;	 break;
		case 4:	//SIC/XE 4형식
			ret = mem[loc + 1] & 0XF;
			len = 2;	break;
		}

	// 메모리 상의 data를 len만큼 byte단위로 읽어 합치기
	for (i = 0; i < len; ++i) {
		ret *= 0X100;
		ret |= mem[loc + ((form!=-1)?2:0) + i];
	}
	
	return ret;
}

// syntax표에 있는 만큼 가져오기
short int extractOperand(int loc, char *mnemonic, int len, int *reg, short int form, int **destReg, int **src, int *ta) {
	unsigned char *mem = memBase();

	short int ni;
	short int xbpe;

	short int r1, r2;
	//int disp;

	*ta = 0;	// target address

	// nibpxe 추출
	ni = mem[loc] & 0X3;
	xbpe = mem[loc + 1] / 0x10;

	if (form == 3 && ni == 0) {	// SIC
		**src = getMem(loc, form, -1);
		form = 0;
	}
	else{
		// evaluate target address
		*ta += getMem(loc, form, -1);
		if (form == 3 && *ta / 0X800) {
			*ta = 0XFFF - *ta + 1;
			*ta *= -1;
		}

		// SIC/XE
		switch (form) {
		case 2:	// 2형식 처리
			r1 = mem[loc + 1] / 0X10;
			r2 = mem[loc + 1] % 0X10;

			*destReg = reg + r1;
			free(*src);
			*src = reg + r2;
			return 1;
		case 3: case 4:	// 3, 4형식의 target address(ta) 계산
			if (xbpe & 0X8)
				*ta += reg[X_];

			switch (xbpe & 0X6) {
			case 0X4:
				*ta += reg[B_]; break;
			case 0X2:
				*ta += reg[PC]; break;
			}

		}
		// fetch operand, considering indirection level given by n, i bit
		switch (ni) {
		case 0X1:
			**src = *ta; break;
		case 0X2:
			if (mnemonic[0] == 'J') {
				**src = getMem(*ta, -1, len); break;
			}
			else {
				**src = getMem(*ta, -1, len);
				**src = getMem(**src, -1, len); break;
			}
		case 0X3:
			if (mnemonic[0] == 'J') {
				**src = *ta; break;
			}
			else {
				**src = getMem(*ta, -1, len); break;
			}
		}
	}
	return 1;
}

// 메모리 상에 올려진 object code를 분석한다.
short int procObj(int *loc, char *mnemonic, short int form, int *reg){
	short int mnemLen = strlen(mnemonic);
	int *destReg;
	int *src = calloc(1, sizeof(int));
	short int operandLen;
	unsigned char *mem = memBase();
	short int i;
	int ta;

	// 이번 프로젝트에서 제외되는 mnemonic의 경우 -1 반환
	switch (mnemonic[0]) {
	case 'H': case 'W':
		return -1;
	case 'S':
		switch (mnemonic[1]) {
		case 'H': case 'T': case 'U': break;
		default: return -1;
		} break;
	case 'T':
		switch (mnemonic[2]) {
		case '\0': case 'O':
			return -1;
		}
	}

	// 결과를 저장할 register와 operand로 참조할 memory 크기(byte) 결정
	destReg = reg + A_;
	operandLen = 3;

	if (mnemLen >= 3) {
		switch (mnemonic[mnemLen - 1]) {
		case 'A':
			destReg = reg + A_; /*set as default*/  break;
		case 'B':
			destReg = reg + B_; /*set as default*/	break;
		case 'H':
			destReg = reg + A_; operandLen = 1;	 break;
		case 'F':
			destReg = reg + F_; operandLen = 6; break;
		case 'L':
			destReg = reg + L_; /*set as default*/	break;
		case 'S':
			destReg = reg + S_; /*set as default*/	break;
		case 'W':
			destReg = reg + SW; /*set as default*/	break;
		case 'T':
			destReg = reg + T_; /*set as default*/	break;
		case 'X':
			destReg = reg + X_; /*set as default*/	break;
		}
	}

	// 결정된 크기만큼 mem을 참조, destination에 할당될 src와 ta를 확정한다.
	extractOperand(*loc, mnemonic, operandLen, reg, form, &destReg, &src, &ta);

	// 결정된 destination register에 계산된 src를 할당한다.
	if (strncmp(mnemonic, "ADD", 3) == 0) {
		*destReg += *src;
	}
	else if (strcmp(mnemonic, "AND") == 0) {
		*destReg &= *src;
	}
	else if (strcmp(mnemonic, "CLEAR") == 0) {
		*destReg = 0;
	}
	else if (strncmp(mnemonic, "COMP", 3) == 0) {
		reg[SW] = INT_CMP(*destReg, *src);
	}
	else if (strncmp(mnemonic, "DIV", 3) == 0) {
		*destReg /= *src;
	}
	else if (strcmp(mnemonic, "JSUB") == 0) {
		reg[L_] = reg[PC];
		*loc = reg[PC] = *src;
	}
	else if (strcmp(mnemonic, "J") == 0) {
		*loc = reg[PC] = *src;
	}
	else if (strncmp(mnemonic, "J", 1) == 0) {
		if (strcmp(mnemonic + 1, "EQ")==0 && reg[SW] == 0)
			*loc = reg[PC] = *src;
		else if (strcmp(mnemonic + 1, "GT")==0 && reg[SW] == 1)
			*loc = reg[PC] = *src;
		else if (strcmp(mnemonic + 1, "LT")==0 && reg[SW] == -1)
			*loc = reg[PC] = *src;
	}
	else if (strncmp(mnemonic, "LD", 2) == 0) {
		*destReg = *src;
	}
	else if (strncmp(mnemonic, "MUL", 3) == 0) {
		*destReg *= *src;
	}
	else if (strcmp(mnemonic, "OR") == 0) {
		*destReg |= *src;
	}
	else if (strcmp(mnemonic, "RMO") == 0) {
		*destReg = *src;
	}
	else if (strcmp(mnemonic, "RSUB") == 0) {
		*loc = reg[PC] = reg[L_];
	}
	else if (strncmp(mnemonic, "SHIFT", 5) == 0) {
		switch (mnemonic[5]) {
		case 'L':
			*destReg = *destReg << *src; break;
		case 'R':
			*destReg = *destReg >> *src; break;
		}
	}
	else if (strncmp(mnemonic, "ST", 2) == 0) {
		short int val = *destReg;
		for (i = 0; i < operandLen; ++i) {
			mem[ta + operandLen - 1 - i] = val & 0XFF;
			val /= 0X100;
		}
	}
	else if (strncmp(mnemonic, "SUB", 3) == 0) {
		*destReg -= *src;
	}
	else if (strncmp(mnemonic, "TIX", 3) == 0) {
		reg[X_] = reg[X_] + 1;
		reg[SW] = INT_CMP(reg[X_], *src);
	}
	return 1;
}

// 레지스터 값을 출력한다.
void printReg(short int mode, int *reg, int loc) {
	printf("A : %.12X X : %.8X\n",	reg[A_], reg[X_]);
	printf("L : %.12X PC : %.12X\n",	reg[L_], reg[PC]);
	printf("B : %.12X S : %.12X\n", reg[B_], reg[S_]);
	printf("T : %.12X\n", reg[T_]);

	// 프로그램 종료시와 break point에 의한 출력 시 출력문을 다르게 한다.
	switch (mode) {
	case EXIT:
		printf("End program\n"); break;
	case BP:
		printf("Stop at checkpoint[%.4X]\n", loc); break;
	}
}

// break point를 hash table에 저장한다.
int addBp(short int init, short int pass, unsigned long int address, int startAddr, int progLen) {
	static bpNode *mapPrev = NULL;
	bpNode **bucket;
	bpNode *tempSym, *prevSym;
	bpNode *symPtr = NULL;
	short int cmpRet;

	// 초기화를 위해 linkedlist 상 이전 노드를 NULL값으로 설정
	if (init == 1) {
		mapPrev = NULL;
		return -1;
	}

	// 버킷 주소 계산
	bucket = bpTab + address % BPTABSIZE;

	// 데이터를 입력하는 경우(pass1)
	if (pass == 1) {
		// 노드를 생성하고 필드 값을 초기화한다.
		symPtr = (bpNode *)calloc(1, sizeof(bpNode));

		if (mapPrev == NULL)
			bpRoot = symPtr;

		symPtr->address = address;
		symPtr->chk = 0;
		symPtr->link = NULL;
		symPtr->mapLink = NULL;
	}

	// 버킷이 비어있고, 
	if (*bucket == NULL) {
		if (pass == 1)	// 입력하는 경우라면
			*bucket = symPtr;	// 버킷에 노드를 바로 할당
	}
	else {					// 버킷에 기존 symbol이 존재할 경우
		prevSym = NULL;
		tempSym = *bucket;
		while (tempSym != NULL) {	// 버킷 내 다른 노드가 존재하면 정렬하기
			cmpRet = INT_CMP(tempSym->address, address);

			if (cmpRet > 0)
				break;
			else if (cmpRet == 0) {
				free(symPtr);
				if (pass == 1)	// pass1인데 중복되는 노드 발견
					fprintf(stderr, "duplicate break point\n");
				// chk가 0이면 breakpoint가 활성화 되있지 않은 경우이므로, 활성화한다.
				if (pass == 2 && tempSym->chk == 0) {
					tempSym->chk = 1;
					return tempSym->address;
				}
				// chk가 0이면 breakpoint가 활성화된있지 않은 경우이므로, 비활성화한다.
				if (pass == 2 && tempSym->chk == 1) {
					tempSym->chk = 0;
					return -1;
				}
				return tempSym->address;
			}

			prevSym = tempSym;
			tempSym = tempSym->link;
		}

		if (pass == 1) {
			if (tempSym != *bucket) {	// symbol이 두번째 노드 이후에 삽입되는 경우
				prevSym->link = symPtr;
				symPtr->link = tempSym;
			}
			else {	// symbol이 첫번째 노드에 삽입되는 경우
				symPtr->link = *bucket;
				*bucket = symPtr;
			}
		}
	}
	// 입력한 순서대로 breakpoint를 출력하기 위해 linkedlist 구성
	if (pass == 1) {
		if (mapPrev != NULL)
			mapPrev->mapLink = symPtr;
		mapPrev = symPtr;
		printf("[ok] create breakpoint %.4X\n", (unsigned int)address);
	}
	return -1;
}

// breakpoint를 입력받은 순서대로 출력
void printBp() {
	bpNode *symPtr;
	
	// linkedlist의 root node가 NULL이 아닐때만 출력하며
	if ((symPtr = bpRoot)!=NULL) {
		puts("break point");
		puts("-----------");
	}

	// linked list를 따라가며 bp의 주소를 출력한다.
	for (; symPtr; symPtr = symPtr->mapLink) {	// 버킷에 노드가 있다면
			printf("%.4x\n", (unsigned int)(symPtr->address));
	}
}

// 레지스터값을 0으로 초기화한다.
void initReg(int *reg) {
	short int i;

	for (i = 0; i < 10; ++i)
		reg[i] = 0;
}

// 메모리 상의 프로그램을 실행한다.
short int run(short int mode, int progLen) {
	static int startAddr;
	static int reg[10] = { 0 };
	static int loc;
	static short int procStat = NEW;
	char *mnemonic;
	short int form;
	short int ret;

	// 현재 프로그램이 실행중인지의 여부를 출력한다.
	if (mode == STAT) {
		return procStat;
	}

	// program을 처음 실행하는 경우 초기화
	switch (procStat) {
	case NEW:
		initReg(reg);
		objErr(INIT_CNT, 0);	// symbol error counter 초기화
		procStat = CONTINUE;
		startAddr = progAddr; break;
	case CONTINUE:
		; break;
	}

	// progAddr로 LOCCTR와 PC레지스터값 갱신
	reg[PC] = loc = progAddr;

	while (1) {	// bp를 발견하지 못할 때까지
		if (loc == progAddr + progLen) {
			procStat = NEW;
			printReg(EXIT, reg, -1);
			activateBp(bpTab, BPTABSIZE);
			return 1;
		}

		// opcode 추출
		extractOpcode(loc, &mnemonic, &form, reg);

		// breakpoint를 만나면 종료
		if ((ret = addBp(0, PASS2, loc, startAddr, progLen)) != -1)
			break;

		// 메모리상의 object code를 분석하여 프로그램 수행
		procObj(&loc, mnemonic, form, reg);

		if (objErr(READ_CNT, 0) > 0) {
			return 0;
		}

		// jump 관련 연산자 수행 시에는 LOCCTR을 increment하지 않는다.
		if (strcmp(mnemonic, "J") != 0 && !(strcmp(mnemonic, "JEQ") == 0 && reg[SW] == 0) && !(strcmp(mnemonic, "JGT") == 0 && reg[SW] == 1) && !(strcmp(mnemonic, "JLT") == 0 && reg[SW] == -1) && strcmp(mnemonic,"JSUB") != 0 && strcmp(mnemonic, "RSUB") != 0)
			loc += form;
	}
	progAddr = loc;
	printReg(BP, reg, loc);

	return 1;
}