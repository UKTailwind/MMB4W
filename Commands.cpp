/***********************************************************************************************************************
MMBasic for Windows

Commands.cpp

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
	in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed
	on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed
	by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software
	without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

************************************************************************************************************************/
#include "olcPixelGameEngine.h"
#include "MainThread.h"
#include "MMBasic_Includes.h"
#include <math.h>

void flist(int, int, int);
//void clearprog(void);
void ListNewLine(int* ListCnt, int all);
void ListProgramFlash(unsigned char* p, int all);
char *MMErrMsg=NULL;                                           // the error message
unsigned char* KeyInterrupt = NULL;
volatile int Keycomplete = 0;
int keyselect = 0;
char runcmd[STRINGSIZE] = { 0 };


// stack to keep track of nested FOR/NEXT loops
struct s_forstack forstack[MAXFORLOOPS + 1];
int forindex;



// stack to keep track of nested DO/LOOP loops
struct s_dostack dostack[MAXDOLOOPS];
int doindex;                                // counts the number of nested DO/LOOP loops


// stack to keep track of GOSUBs, SUBs and FUNCTIONs
unsigned char* gosubstack[MAXGOSUB];
unsigned char* errorstack[MAXGOSUB];
int gosubindex;

unsigned char DimUsed = false;						// used to catch OPTION BASE after DIM has been used

int TraceOn;                                // used to track the state of TRON/TROFF
unsigned char* TraceBuff[TRACE_BUFF_SIZE];
int TraceBuffIndex;                       // used for listing the contents of the trace buffer
int OptionErrorSkip=0;                                               // how to handle an error
int MMerrno=0;                                                        // the error number
int ListCnt; 
sa_data datastore[MAXRESTORE];
int restorepointer = 0;
const unsigned int CaseOption = 0xffffffff;	// used to store the case of the listed output

void cmd_null(void) {
	// do nothing (this is just a placeholder for commands that have no action)
}

// utility routine used by DoDim() below and other places in the interpreter
// checks if the type has been explicitly specified as in DIM FLOAT A, B, ... etc
extern "C" unsigned char* CheckIfTypeSpecified(unsigned char* p, int* type, int AllowDefaultType) {
    unsigned char* tp;

    if((tp = checkstring(p, (unsigned char*)"INTEGER")) != NULL)
        *type = T_INT | T_IMPLIED;
    else if((tp = checkstring(p, (unsigned char*)"STRING")) != NULL)
        *type = T_STR | T_IMPLIED;
    else if((tp = checkstring(p, (unsigned char*)"FLOAT")) != NULL)
        *type = T_NBR | T_IMPLIED;
    else {
        if(!AllowDefaultType) error((char *)"Variable type");
        tp = p;
        *type = DefaultType;                                        // if the type is not specified use the default
    }
    return tp;
}

void execute_one_command(unsigned char* p) {
	int cmd, i;

	CheckAbort();
	targ = T_CMD;
	skipspace(p);													// skip any whitespace
	if(*p >= C_BASETOKEN && *p - C_BASETOKEN < CommandTableSize - 1 && (commandtbl[*p - C_BASETOKEN].type & T_CMD)) {
		cmd = *p - C_BASETOKEN;
		if(*p == cmdWHILE || *p == cmdDO || *p == cmdFOR) error((char*)"Invalid inside THEN ... ELSE");
		cmdtoken = *p;
		cmdline = p + 1;
		skipspace(cmdline);
		commandtbl[cmd].fptr();						                // execute the command
	}
	else {
		if(!isnamestart(*p)) error((char*)"Invalid character");
		i = FindSubFun(p, false);                                   // it could be a defined command
		if(i >= 0)                                                  // >= 0 means it is a user defined command
			DefinedSubFun(false, p, i, NULL, NULL, NULL, NULL);
		else
			error((char*)"Unknown command");
	}
	ClearTempMemory();											    // at the end of each command we need to clear any temporary string vars
}
void cmd_quit(void) {
	SystemMode = MODE_QUIT;
}
void cmd_inc(void) {
	unsigned char* p, *q;
	int vtype;
	getargs(&cmdline, 3, (unsigned char *)",");
	if(argc == 1) {
		p = (unsigned char *)findvar(argv[0], V_FIND);
		if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
		vtype = TypeMask(vartbl[VarIndex].type);
		if(vtype & T_STR) error((char *)"Invalid variable");                // sanity check
		if(vtype & T_NBR)
			(*(MMFLOAT*)p) = (*(MMFLOAT*)p) + 1.0;
		else if(vtype & T_INT)*(long long int*)p = *(long long int*)p + 1;
		else error((char *)"Syntax");
	}
	else {
		p = (unsigned char*)findvar(argv[0], V_FIND);
		if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
		vtype = TypeMask(vartbl[VarIndex].type);
		if(vtype & T_STR) {
			q = getstring(argv[2]);
			if (*p + *q > MAXSTRLEN) error((char*)"String too long");
			Mstrcat(p, q);
		}
		else if(vtype & T_NBR) {
			(*(MMFLOAT*)p) = (*(MMFLOAT*)p) + getnumber(argv[2]);
		}
		else if(vtype & T_INT) {
			*(long long int*)p = *(long long int*)p + getinteger(argv[2]);
		}
		else error((char *)"syntax");
	}
}
// the PRINT command
void cmd_print(void) {
	unsigned char* s, * p;
	unsigned char* ss;
	MMFLOAT f;
	long long int  i64;
	int i, t, fnbr;
	int docrlf;														// this is used to suppress the cr/lf if needed

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char*)";,");				// this is a macro and must be the first executable stmt

//    s = 0; *s = 56;											    // for testing the exception handler

	docrlf = true;

	if(argc > 0 && *argv[0] == '#') {								// check if the first arg is a file number
		argv[0]++;
		if((*argv[0] == 'G') || (*argv[0] == 'g')) {
			argv[0]++;
			if(!((*argv[0] == 'P') || (*argv[0] == 'p')))error((char *)"Syntax");
			argv[0]++;
			if(!((*argv[0] == 'S') || (*argv[0] == 's')))error((char *)"Syntax");
			if(!GPSchannel) error((char *)"GPS not activated");
			if(argc != 3) error((char *)"Only a single string parameter allowed");
			p = argv[2];
			t = T_NOTYPE;
			p = evaluate(p, &f, &i64, &s, &t, true);			// get the value and type of the argument
			ss = (unsigned char*)s;
			if(!(t & T_STR)) error((char *)"Only a single string parameter allowed");
			int i, xsum = 0;
			if(ss[1] != '$' || ss[ss[0]] != '*')error((char *)"GPS command must start with dollar and end with star");
			for (i = 1; i <= ss[0]; i++) {
				SerialPutchar(GPSchannel, s[i]);
				if(s[i] == '$')xsum = 0;
				if(s[i] != '*')xsum ^= s[i];
			}
			i = xsum / 16;
			i = i + '0';
			if(i > '9')i = i - '0' + 'A';
			SerialPutchar(GPSchannel, i);
			i = xsum % 16;
			i = i + '0';
			if(i > '9')i = i - '0' + 'A';
			SerialPutchar(GPSchannel, i);
			SerialPutchar(GPSchannel, 13);
			SerialPutchar(GPSchannel, 10);
			return;
		}
		else {
			fnbr = (int)getinteger(argv[0]);									// get the number
			i = 1;
			if(argc >= 2 && *argv[1] == ',') i = 2;						// and set the next argument to be looked at
		}
	}
	else {
		fnbr = 0;													// no file number so default to the standard output
		i = 0;
	}
	if (argc >= 3) {
		if (checkstring(argv[2], (unsigned char*)"BREAK")) {
			if (FileTable[fnbr].com == 0 || FileTable[fnbr].com > MAXCOMPORTS)error((char*)"Syntax");
			SendBreak(fnbr);
			return;
		}
	}
	for (; i < argc; i++) {											// step through the arguments
		if(*argv[i] == ',') {
			MMfputc('\t', fnbr);									// print a tab for a comma
			docrlf = false;                                         // a trailing comma should suppress CR/LF
		}
		else if(*argv[i] == ';') {
			docrlf = false;											// other than suppress cr/lf do nothing for a semicolon
		}
		else {														// we have a normal expression
			p = argv[i];
			while (*p) {
				t = T_NOTYPE;
				p = evaluate(p, &f, &i64, &s, &t, true);			// get the value and type of the argument
				if(t & T_NBR) {
					*inpbuf = ' ';                                  // preload a space
					FloatToStr((char *)(inpbuf + ((f >= 0) ? 1 : 0)), f, 0, STR_AUTO_PRECISION, ' ');// if positive output a space instead of the sign
					MMfputs((unsigned char*)CtoM(inpbuf), fnbr);					// convert to a MMBasic string and output
				}
				else if(t & T_INT) {
					*inpbuf = ' ';                                  // preload a space
					IntToStr((char *)(inpbuf + ((i64 >= 0) ? 1 : 0)), i64, 10); // if positive output a space instead of the sign
					MMfputs((unsigned char*)CtoM(inpbuf), fnbr);					// convert to a MMBasic string and output
				}
				else if(t & T_STR) {
					MMfputs((unsigned char*)s, fnbr);								// print if a string (s is a MMBasic string)
				}
				else error((char *)"Attempt to print reserved word");
			}
			docrlf = true;
		}
	}
	if(docrlf) MMfputs((unsigned char*)"\2\r\n", fnbr);								// print the terminating cr/lf unless it has been suppressed
	PrintPixelMode = 0;
}

void cmd_debug(void) {
	unsigned char* s, * p;
	MMFLOAT f;
	long long int  i64;
	int i, t, fnbr;
	int docrlf;														// this is used to suppress the cr/lf if needed

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char*)";,");				// this is a macro and must be the first executable stmt

//    s = 0; *s = 56;											    // for testing the exception handler

	docrlf = true;


	{
		fnbr = 99999;													// no file number so default to the standard output
		i = 0;
	}
	for (; i < argc; i++) {											// step through the arguments
		if (*argv[i] == ',') {
			MMfputc('\t', fnbr);									// print a tab for a comma
			docrlf = false;                                         // a trailing comma should suppress CR/LF
		}
		else if (*argv[i] == ';') {
			docrlf = false;											// other than suppress cr/lf do nothing for a semicolon
		}
		else {														// we have a normal expression
			p = argv[i];
			while (*p) {
				t = T_NOTYPE;
				p = evaluate(p, &f, &i64, &s, &t, true);			// get the value and type of the argument
				if (t & T_NBR) {
					*inpbuf = ' ';                                  // preload a space
					FloatToStr((char*)(inpbuf + ((f >= 0) ? 1 : 0)), f, 0, STR_AUTO_PRECISION, ' ');// if positive output a space instead of the sign
					MMfputs((unsigned char*)CtoM(inpbuf), fnbr);					// convert to a MMBasic string and output
				}
				else if (t & T_INT) {
					*inpbuf = ' ';                                  // preload a space
					IntToStr((char*)(inpbuf + ((i64 >= 0) ? 1 : 0)), i64, 10); // if positive output a space instead of the sign
					MMfputs((unsigned char*)CtoM(inpbuf), fnbr);					// convert to a MMBasic string and output
				}
				else if (t & T_STR) {
					MMfputs((unsigned char*)s, fnbr);								// print if a string (s is a MMBasic string)
				}
				else error((char*)"Attempt to print reserved word");
			}
			docrlf = true;
		}
	}
	if (docrlf) MMfputs((unsigned char*)"\2\r\n", fnbr);								// print the terminating cr/lf unless it has been suppressed
	PrintPixelMode = 0;
}


// the LET command
// because the LET is implied (ie, line does not have a recognisable command)
// it ends up as the place where mistyped commands are discovered.  This is why
// the error message is "Unknown command"
void  cmd_let(void) {
	int t, size;
	MMFLOAT f;
	long long int  i64;
	unsigned char* s;
	unsigned char* p1, *p2;

	p1 = cmdline;

	// search through the line looking for the equals sign
	while (*p1 && tokenfunction(*p1) != op_equal) p1++;
	if(!*p1) error((char *)"Unknown command");

	// check that we have a straight forward variable
	p2 = skipvar(cmdline, false);
	skipspace(p2);
	if(p1 != p2) error((char *)"Syntax");

	// create the variable and get the length if it is a string
	p2 = (unsigned char *)findvar(cmdline, V_FIND);
	size = vartbl[VarIndex].size;
	if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");

	// step over the equals sign, evaluate the rest of the command and save in the variable
	p1++;
	if(vartbl[VarIndex].type & T_STR) {
		t = T_STR;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(*s > size) error((char *)"String too long");
		Mstrcpy(p2, s);
	}
	else if(vartbl[VarIndex].type & T_NBR) {
		t = T_NBR;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(t & T_NBR)
			(*(MMFLOAT*)p2) = f;
		else
			(*(MMFLOAT*)p2) = (MMFLOAT)i64;
	}
	else {
		t = T_INT;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(t & T_INT)
			(*(long long int*)p2) = i64;
		else
			(*(long long int*)p2) = FloatToInt64(f);
	}
	checkend(p1);
}
int as_strcmpi(const char* s1, const char* s2)
{
	const unsigned char* p1 = (const unsigned char*)s1;
	const unsigned char* p2 = (const unsigned char*)s2;
	unsigned char c1, c2;

	if(p1 == p2)
		return 0;

	do
	{
		c1 = tolower(*p1++);
		c2 = tolower(*p2++);
		if(c1 == '\0')
			break;
	} while (c1 == c2);

	return c1 - c2;
}

void sortStrings(char** arr, int n)
{
	char temp[16];
	int i, j;
	// Sorting strings using bubble sort
	for (j = 0; j < n - 1; j++)
	{
		for (i = j + 1; i < n; i++)
		{
			if(as_strcmpi(arr[j], arr[i]) > 0)
			{
				strcpy(temp, arr[j]);
				strcpy(arr[j], arr[i]);
				strcpy(arr[i], temp);
			}
		}
	}
}
void sortPorts(char** arr, int n)
{
	char temp[16];
	int i, j;
	// Sorting strings using bubble sort
	for (j = 0; j < n - 1; j++)
	{
		for (i = j + 1; i < n; i++)
		{
			char** c=NULL;
			int a = strtol((const char *) & arr[j][3], c, 10);
			int b = strtol((const char*)&arr[i][3], c, 10);
			if (a>b)
			{
				strcpy(temp, arr[j]);
				strcpy(arr[j], arr[i]);
				strcpy(arr[i], temp);
			}
		}
	}
}

void ListFile(char* pp, int all) {
/******	char buff[STRINGSIZE];
	FRESULT fr;
	FILINFO fno;
	int fnbr;
	int i, ListCnt = 1;
	fr = f_stat(pp, &fno);
	if(fr == FR_OK && !(fno.fattrib & AM_DIR)) {
		fnbr = FindFreeFileNbr();
		if(!BasicFileOpen(pp, fnbr, FA_READ)) return;
		while (!FileEOF(fnbr)) {                                     // while waiting for the end of file
			memset(buff, 0, 256);
			MMgetline(fnbr, (char*)buff);									    // get the input line
			for (i = 0; i < strlen(buff); i++)if(buff[i] == TAB) buff[i] = ' ';
			MMPrintString(buff);
			ListCnt += strlen(buff) / OptionWidth;
			ListNewLine(&ListCnt, all);
		}
		FileClose(fnbr);
	}
	else error((char *)"File not found");*/
}
void QueryKey(HKEY hKey)
{
	LPSTR    achKey=(LPSTR)GetTempMemory(STRINGSIZE);   // buffer for subkey name
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD i, retCode;

	
	DWORD cchValue = 255;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hKey,                    // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 

	// Enumerate the subkeys, until RegEnumKeyEx fails.

	// Enumerate the key values. 

	if (cValues>0 && cValues<=128 && retCode== ERROR_SUCCESS)
	{
		char** c = (char**)GetTempMemory((cValues) * sizeof(*c) + (cValues) *(cchMaxValue + 1));
		MMPrintString((char *)"Number of com ports : ");
		PInt(cValues);
		PRet();
		char *b = (char *)GetTempMemory(cbMaxValueData+1);
		LPSTR  achValue = (LPSTR)GetTempMemory(cchMaxValue+1);
		DWORD fred= cbMaxValueData+1;
		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = 255;
			achValue[0] = '\0';
			memset(b,0, cbMaxValueData + 1);
			fred = cbMaxValueData + 1;
			retCode = RegEnumValueA(hKey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				(LPBYTE)b,
				&fred);

			if (retCode == ERROR_SUCCESS)
			{
				c[i] = (char*)((int)c + sizeof(char*) * (cValues) + i * (cchMaxValue + 1));
				strcpy(c[i], b);
			}
		}
		sortPorts(c, cValues);
		for (int j = 0; j < (int)cValues; j++) {
			MMPrintString(c[j]);
			PRet();
		}
	}
}

void cmd_list(void) {
	unsigned char* p;
	int i, j, k, m, step;
	if((p = checkstring(cmdline, (unsigned char*)"ALL"))) {
		if(!(*p == 0 || *p == '\'')) {
			getargs(&p, 1, (unsigned char *)",");
			char* buff = (char *)GetTempMemory(STRINGSIZE);
			strcpy(buff, (const char *)getCstring(argv[0]));
			if(strchr(buff, '.') == NULL) strcat(buff, ".BAS");
			ListProgram((unsigned char*)buff, true);
		}
		else {
			ListProgram(NULL, true);
			checkend(p);
		}
	}
	else if ((p = checkstring(cmdline, (unsigned char*)"FLASH"))) {
		ListProgramFlash((unsigned char*)ProgMemory, false);
		checkend(p);
	}
	else if (p = checkstring(cmdline, (unsigned char *)"PAGES")) {
		PO("MODE ", 3); MMPrintString((char*)" has "); PInt(MAXPAGES+1); MMPrintString((char*)" pages\r\n");
		MMPrintString((char*)"Page no.   Page Address   Width   Height   Size  "); PRet();
		for (int i = 0; i <= MAXPAGES; i++) {
			MMPrintString((char*)" ");
			if (i < 10)MMPrintString((char*)" ");
			PInt(i);
			MMPrintString((char *)"         &H");
			PIntH((uint32_t)PageTable[i].address);
			if (PageTable[i].xmax < 1000)MMPrintString((char*)"     ");
			else MMPrintString((char*)"    ");
			PInt((uint32_t)PageTable[i].xmax);
			MMPrintString((char*)"     ");
			PInt((uint32_t)PageTable[i].ymax);
			if (PageTable[i].size < 0xFFFF)MMPrintString((char*)" ");
			MMPrintString((char*)"    &H");
			PIntH((uint32_t)PageTable[i].size);
			PRet();
		}
	}
	else if ((p = checkstring(cmdline, (unsigned char*)"COM PORTS"))) {
		HKEY hregkey;
		LSTATUS res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hregkey);
		QueryKey(hregkey);
		RegCloseKey((HKEY)hregkey);
	}
	else if ((p = checkstring(cmdline, (unsigned char*)"FILE ALL"))) {
		getargs(&p, 1, (unsigned char*)",");
		char* buff = (char*)GetTempMemory(STRINGSIZE);
		strcpy(buff, (char*)getCstring(argv[0]));
		if (strchr(buff, '.') == NULL) strcat(buff, ".BAS");
		ListProgram((unsigned char*)buff, true);
	}
	else if ((p = checkstring(cmdline, (unsigned char*)"FILE"))) {
		getargs(&p, 1, (unsigned char *)",");
		char* buff = (char *)GetTempMemory(STRINGSIZE);
		strcpy(buff, (char *)getCstring(argv[0]));
		if (strchr(buff, '.') == NULL) strcat(buff, ".BAS");
		ListProgram((unsigned char *)buff, false);
	}
	else if ((p = checkstring(cmdline, (unsigned char*)"COMMANDS"))) {
		step = OptionWidth/18;
		m = 0;
		char** c = (char **)GetTempMemory((CommandTableSize + 5) * sizeof(*c) + (CommandTableSize + 5) * 18);
		for (i = 0; i < CommandTableSize + 5; i++) {
			c[m] = (char*)((int)c + sizeof(char*) * (CommandTableSize + 5) + m * 18);
			if(m < CommandTableSize)strcpy(c[m], (const char *)commandtbl[i].name);
			else if(m == CommandTableSize)strcpy(c[m], "Color");
			else if(m == CommandTableSize + 1)strcpy(c[m], "Else If");
			else if(m == CommandTableSize + 2)strcpy(c[m], "End If");
			else if(m == CommandTableSize + 3)strcpy(c[m], "Exit Do");
			else strcpy(c[m], "Cat");
			if(strcasecmp(c[m],"CSub")!=0  && strcasecmp(c[m], "End CSub") != 0)m++;
		}
		sortStrings(c, m);
		for (i = 1; i < m; i += step) {
			for (k = 0; k < step; k++) {
				if(i + k < m) {
					MMPrintString(c[i + k]);
					if(k != (step - 1))for (j = strlen(c[i + k]); j < 15; j++)MMputchar(' ');
				}
			}
			MMPrintString((char *)"\r\n");
		}
		MMPrintString((char *)"Total of "); PInt(m - 1); MMPrintString((char *)" commands\r\n");
	}
	else if((p = checkstring(cmdline, (unsigned char*)"FUNCTIONS"))) {
		m = 0;
		step = OptionWidth / 18;
		char** c = (char**)GetTempMemory((TokenTableSize + 5) * sizeof(*c) + (TokenTableSize + 5) * 18);
		for (i = 0; i < TokenTableSize + 5; i++) {
			c[m] = (char*)((int)c + sizeof(char*) * (TokenTableSize + 5) + m * 18);
			if(m < TokenTableSize)strcpy(c[m], (const char*)tokentbl[i].name);
			else if(m == TokenTableSize)strcpy(c[m], "=>");
			else if(m == TokenTableSize + 1)strcpy(c[m], "=<");
			/*    			else if(m==TokenTableSize+2)strcpy(c[m],"OCT$(");
							else if(m==TokenTableSize+3)strcpy(c[m],"HEX$(");
							else if(m==TokenTableSize+4)strcpy(c[m],"MM.I2C");
			*/    			else if(m == TokenTableSize + 2)strcpy(c[m], "MM.Fontheight");
			else if(m == TokenTableSize + 3)strcpy(c[m], "MM.Fontwidth");
			else strcpy(c[m], "MM.Info$(");
			m++;
		}
		sortStrings(c, m);
		for (i = 1; i < m; i += step) {
			for (k = 0; k < step; k++) {
				if(i + k < m) {
					MMPrintString(c[i + k]);
					if(k != (step - 1))for (j = strlen(c[i + k]); j < 15; j++)MMputchar(' ');
				}
			}
			MMPrintString((char *)"\r\n");
		}
		MMPrintString((char *)"Total of "); PInt(m - 1); MMPrintString((char *)" functions and operators\r\n");
	}
	else {
		if(!(*cmdline == 0 || *cmdline == '\'')) {
			getargs(&cmdline, 1, (unsigned char *)",");
			char* buff = (char *)GetTempMemory(STRINGSIZE);
			strcpy(buff, (const char*)getCstring(argv[0]));
			if(strchr(buff, '.') == NULL) strcat(buff, ".BAS");
			ListProgram((unsigned char*)buff, false);
		}
		else {
			ListProgram(NULL, false);
			checkend(cmdline);
		}
	}
}


void ListNewLine(int* ListCnt, int all) {
	MMPrintString((char *)"\r\n");
	(*ListCnt)++;
	if(!all && *ListCnt >= OptionHeight) {
		MMPrintString((char *)"PRESS ANY KEY ...");
		MMgetchar();
		MMPrintString((char *)"\r                 \r");
		*ListCnt = 1;
	}
}


void ListProgramFlash(unsigned char* p, int all) {
	char b[STRINGSIZE];
	char* pp;
	int ListCnt = 1;


	while (!(*p == 0 || *p == 0xff)) {                               // normally a LIST ends at the break so this is a safety precaution
		if(*p == T_NEWLINE) {
			p = llist((unsigned char*) b, (unsigned char *)p);                                        // otherwise expand the line
			pp = b;
			while (*pp) {
				if(MMCharPos >= OptionWidth) ListNewLine(&ListCnt, all);
				MMputchar(*pp++);
			}
			ListNewLine(&ListCnt, all);
			if(p[0] == 0 && p[1] == 0) break;                       // end of the listing ?
		}
	}
}
void ListProgram(unsigned char* pp, int all) {
	char buff[STRINGSIZE];
	int fnbr;
	int i, ListCnt = 1;
	if (pp == NULL) {
		if (!*lastfileedited)error((char*)"Nothing to list");
		strcpy(buff, lastfileedited);
	}
	else {
		fullfilename((char *)pp, buff, NULL);
	}
	if(!existsfile(buff))error((char*)"File not found");
	fnbr = FindFreeFileNbr();
	if (!BasicFileOpen((char *)buff, fnbr, (char *)"rb")) return;
	while (!MMfeof(fnbr)) {                                     // while waiting for the end of file
		memset(buff, 0, 256);
		MMgetline(fnbr, (char*)buff);									    // get the input line
		for (i = 0; i < (int)strlen(buff); i++)if (buff[i] == TAB) buff[i] = ' ';
		MMPrintString(buff);
		ListCnt += strlen(buff) / OptionWidth;
		ListNewLine(&ListCnt, all);
	}
	FileClose(fnbr);
}

void execute(char* mycmd) {
	//    char *temp_tknbuf;
	unsigned char* ttp;
	int i = 0, toggle = 0;
	//    temp_tknbuf = GetTempStrMemory();
	//    strcpy(temp_tknbuf, tknbuf);
		// first save the current token buffer in case we are in immediate mode
		// we have to fool the tokeniser into thinking that it is processing a program line entered at the console
	skipspace(mycmd);
	strcpy((char *)inpbuf, (const char *)getCstring((unsigned char *)mycmd));                                      // then copy the argument
	if (!(toupper(inpbuf[0]) == 'R' && toupper(inpbuf[1]) == 'U' && toupper(inpbuf[2]) == 'N')) { //convert the string to upper case
		while (inpbuf[i]) {
			if (inpbuf[i] == 34) {
				if (toggle == 0)toggle = 1;
				else toggle = 0;
			}
			if (!toggle) {
				if (inpbuf[i] == ':')error((char *)"Only single statements allowed");
				inpbuf[i] = toupper(inpbuf[i]);
			}
			i++;
		}
		tokenise(true);                                                 // and tokenise it (the result is in tknbuf)
		memset(inpbuf, 0, STRINGSIZE);
		tknbuf[strlen((char *)tknbuf)] = 0;
		tknbuf[strlen((char*)tknbuf) + 1] = 0;
		ttp = nextstmt;                                                 // save the globals used by commands
		ScrewUpTimer = 1000;
		ExecuteProgram(tknbuf);                                              // execute the function's code
		ScrewUpTimer = 0;
		// TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
		nextstmt = ttp;
		return;
	}
	else {
		unsigned char* p = inpbuf;
		char * s=NULL;
		char fn[STRINGSIZE] = { 0 };
		p[0] = GetCommandValue((unsigned char *)"RUN");
		memmove(&p[1], &p[4], strlen((char *)p) - 4);
		p[strlen((char*)p) - 3] = 0;
//		MMPrintString(fn); PRet();
		CloseAudio(1);
		strcpy((char *)tknbuf, (char*)inpbuf);
		longjmp(jmprun, 1);
	}
}
void cmd_execute(void) {
	execute((char*)cmdline);
}


void cmd_run(void) {
	skipspace(cmdline);
	memset(runcmd, 0, STRINGSIZE);
	memcpy(runcmd, cmdline, strlen((char *)cmdline));

	if (*cmdline && *cmdline != '\'') {
		if (!FileLoadProgram(cmdline, 0)) return;
	}
	else {
		if (*lastfileedited == 0)error((char*)"Nothing to run");
		if (!FileLoadProgram((unsigned char *)lastfileedited, 1)) return;
	}
	ClearRuntime();
	WatchdogSet = false;
	PrepareProgram(true);
	IgnorePIN = false;
	if(*ProgMemory != T_NEWLINE) return;                             // no program to run
	nextstmt = ProgMemory;
}



void  cmd_continue(void) {
	if(*cmdline == tokenFOR) {
		if(forindex == 0) error((char *)"No FOR loop is in effect");
		nextstmt = forstack[forindex - 1].nextptr;
		return;
	}
	if(checkstring(cmdline, (unsigned char*)"DO")) {
		if(doindex == 0) error((char *)"No DO loop is in effect");
		nextstmt = dostack[doindex - 1].loopptr;
		return;
	}
	// must be a normal CONTINUE
	checkend(cmdline);
	if(CurrentLinePtr) error((char *)"Invalid in a program");
	if(ContinuePoint == NULL) error((char *)"Cannot continue");
	//    IgnorePIN = false;
	nextstmt = ContinuePoint;
}

void cmd_new(void) {
	checkend(cmdline);
	ClearProgram();
	uSec(250000);
	memset(inpbuf, 0, STRINGSIZE);
	memset(lastfileedited, 0, STRINGSIZE);
	memset(Option.lastfilename, 0, STRINGSIZE);
	SaveOptions();
	longjmp(mark, 1);							                    // jump back to the input prompt
}


void cmd_erase(void) {
	int i, j, k, len;
	char p[MAXVARLEN + 1], * s, * x;

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");				// getargs macro must be the first executable stmt in a block
	if((argc & 0x01) == 0) error((char *)"Argument count");

	for (i = 0; i < argc; i += 2) {
		strcpy((char*)p, (const char *)argv[i]);
		while (!isnamechar(p[strlen(p) - 1])) p[strlen(p) - 1] = 0;

		makeupper((unsigned char *)p);                                               // all variables are stored as uppercase
		for (j = MAXVARS / 2; j < MAXVARS; j++) {
			s = p;  x = vartbl[j].name; len = strlen(p);
			while (len > 0 && *s == *x) {                            // compare the variable to the name that we have
				len--; s++; x++;
			}
			if(!(len == 0 && (*x == 0 || strlen(p) == MAXVARLEN))) continue;

			// found the variable
			if(((vartbl[i].type & T_STR) || vartbl[i].dims[0] != 0) && !(vartbl[i].type & T_PTR)) {
				FreeMemory((unsigned char *)vartbl[i].val.s);                        // free any memory (if allocated)
				vartbl[i].val.s = NULL;
			}
			k = i + 1;
			if(k == MAXVARS)k = MAXVARS / 2;
			if(vartbl[k].type) {
				vartbl[j].name[0] = '~';
				vartbl[j].type = T_BLOCKED;
			}
			else {
				vartbl[j].name[0] = 0;
				vartbl[j].type = T_NOTYPE;
			}
			vartbl[i].dims[0] = 0;                                    // and again
			vartbl[i].level = 0;
			Globalvarcnt--;
			break;
		}
		if(j == MAXVARS) error((char *)"Cannot find $", p);
	}
}
void cmd_clear(void) {
	checkend(cmdline);
	if(LocalIndex)error((char *)"Invalid in a subroutine");
	ClearVars(0);
}


void cmd_goto(void) {
	if(isnamestart(*cmdline))
		nextstmt = findlabel(cmdline);								// must be a label
	else
		nextstmt = findline((int)getinteger(cmdline), true);				// try for a line number
	CurrentLinePtr = nextstmt;
}



void cmd_if(void) {
	int r, i, testgoto, testelseif;
	unsigned char ss[3];														// this will be used to split up the argument line
	unsigned char* p, * tp;
	unsigned char* rp = NULL;

	ss[0] = tokenTHEN;
	ss[1] = tokenELSE;
	ss[2] = 0;

	testgoto = false;
	testelseif = false;

retest_an_if:
	{																// start a new block
		getargs(&cmdline, 20, ss);									// getargs macro must be the first executable stmt in a block

		if(testelseif && argc > 2) error((char *)"Unexpected text");

		// if there is no THEN token retry the test with a GOTO.  If that fails flag an error
		if(argc < 2 || *argv[1] != ss[0]) {
			if(testgoto) error((char *)"IF without THEN");
			ss[0] = tokenGOTO;
			testgoto = true;
			goto retest_an_if;
		}


		// allow for IF statements embedded inside this IF
		if(argc >= 3 && *argv[2] == cmdIF) argc = 3;                // this is IF xx=yy THEN IF ... so we want to evaluate only the first 3
		if(argc >= 5 && *argv[4] == cmdIF) argc = 5;                // this is IF xx=yy THEN cmd ELSE IF ... so we want to evaluate only the first 5

		if(argc == 4 || (argc == 5 && *argv[3] != ss[1])) error((char *)"Syntax");

		r = (getnumber(argv[0]) != 0);								// evaluate the expression controlling the if statement

		if(r) {
			// the test returned TRUE
			// first check if it is a multiline if(ie, only 2 args)
			if(argc == 2) {
				// if multiline do nothing, control will fall through to the next line (which is what we want to execute next)
				;
			}
			else {
				// This is a standard single line IF statement
				// Because the test was TRUE we are just interested in the THEN cmd stage.
				if(*argv[1] == tokenGOTO) {
					cmdline = argv[2];
					cmd_goto();
					return;
				}
				else if(isdigit(*argv[2])) {
					nextstmt = findline((int)getinteger(argv[2]), true);
				}
				else {
					if(argc == 5) {
						// this is a full IF THEN ELSE and the statement we want to execute is between the THEN & ELSE
						// this is handled by a special routine
						execute_one_command((unsigned char *)argv[2]);
					}
					else {
						// easy - there is no ELSE clause so just point the next statement pointer to the byte after the THEN token
						for (p = cmdline; *p && *p != ss[0]; p++);	// search for the token
						nextstmt = p + 1;							// and point to the byte after
					}
				}
			}
		}
		else {
			// the test returned FALSE so we are just interested in the ELSE stage (if present)
			// first check if it is a multiline if(ie, only 2 args)
			if(argc == 2) {
				// search for the next ELSE, or ENDIF and pass control to the following line
				// if an ELSEIF is found re execute this function to evaluate the condition following the ELSEIF
				i = 1; p = nextstmt;
				while (1) {
					p = GetNextCommand(p, &rp, (unsigned char *)"No matching ENDIF");
					if(*p == cmdtoken) {
						// found a nested IF command, we now need to determine if it is a single or multiline IF
						// search for a THEN, then check if only white space follows.  If so, it is multiline.
						tp = p + 1;
						while (*tp && *tp != ss[0]) tp++;
						if(*tp) tp++;								// step over the THEN
						skipspace(tp);
						if(*tp == 0 || *tp == '\'')					// yes, only whitespace follows
							i++;									// count it as a nested IF
						else										// no, it is a single line IF
							skipelement(p);							// skip to the end so that we avoid an ELSE
						continue;
					}

					if(*p == cmdELSE && i == 1) {
						// found an ELSE at the same level as this IF.  Step over it and continue with the statement after it
						skipelement(p);
						nextstmt = p;
						break;
					}

					if((*p == cmdELSEIF) && i == 1) {
						// we have found an ELSEIF statement at the same level as our IF statement
						// setup the environment to make this function evaluate the test following ELSEIF and jump back
						// to the start of the function.  This is not very clean (it uses the dreaded goto for a start) but it works
						p++;                                        // step over the token
						skipspace(p);
						CurrentLinePtr = rp;
						if(*p == 0) error((char *)"Syntax");        // there must be a test after the elseif
						cmdline = p;
						skipelement(p);
						nextstmt = p;
						testgoto = false;
						testelseif = true;
						goto retest_an_if;
					}

					if(*p == cmdENDIF) i--;						// found an ENDIF so decrement our nested counter
					if(i == 0) {
						// found our matching ENDIF stmt.  Step over it and continue with the statement after it
						skipelement(p);
						nextstmt = p;
						break;
					}
				}
			}
			else {
				// this must be a single line IF statement
				// check if there is an ELSE on the same line
				if(argc == 5) {
					// there is an ELSE command
					if(isdigit(*argv[4]))
						// and it is just a number, so get it and find the line
						nextstmt = findline((int)getinteger(argv[4]), true);
					else {
						// there is a statement after the ELSE clause  so just point to it (the byte after the ELSE token)
						for (p = cmdline; *p && *p != ss[1]; p++);	// search for the token
						nextstmt = p + 1;							// and point to the byte after
					}
				}
				else {
					// no ELSE on a single line IF statement, so just continue with the next statement
					skipline(cmdline);
					nextstmt = cmdline;
				}
			}
		}
	}
}



void cmd_else(void) {
	int i;
	unsigned char* p, * tp;

	// search for the next ENDIF and pass control to the following line
	i = 1; p = nextstmt;

	if(cmdtoken == cmdELSE) checkend(cmdline);

	while (1) {
		p = GetNextCommand(p, NULL, (unsigned char *)"No matching ENDIF");
		if(*p == cmdIF) {
			// found a nested IF command, we now need to determine if it is a single or multiline IF
			// search for a THEN, then check if only white space follows.  If so, it is multiline.
			tp = p + 1;
			while (*tp && *tp != tokenTHEN) tp++;
			if(*tp) tp++;											// step over the THEN
			skipspace(tp);
			if(*tp == 0 || *tp == '\'')								// yes, only whitespace follows
				i++;												// count it as a nested IF
		}
		if(*p == cmdENDIF) i--;				    // found an ENDIF so decrement our nested counter
		if(i == 0) break;											// found our matching ENDIF stmt
	}
	// found a matching ENDIF.  Step over it and continue with the statement after it
	skipelement(p);
	nextstmt = p;
}



void cmd_end(void) {
	checkend(cmdline);
	memset(inpbuf, 0, STRINGSIZE);
	longjmp(mark, 1);												// jump back to the input prompt
}


void cmd_select(void) {
	int i, type;
	unsigned char* p, * rp = NULL, * SaveCurrentLinePtr;
	void* v;
	MMFLOAT f = 0;
	long long int  i64 = 0;
	unsigned char s[STRINGSIZE];

	// these are the tokens that we will be searching for
	// they are cached the first time this command is called

	type = T_NOTYPE;
	v = DoExpression(cmdline, &type);                               // evaluate the select case value
	type = TypeMask(type);
	if(type & T_NBR) f = *(MMFLOAT*)v;
	if(type & T_INT) i64 = *(long long int*)v;
	if(type & T_STR) Mstrcpy((unsigned char*)s, (unsigned char*)v);

	// now search through the program looking for a matching CASE statement
	// i tracks the nesting level of any nested SELECT CASE commands
	SaveCurrentLinePtr = CurrentLinePtr;                            // save where we are because we will have to fake CurrentLinePtr to get errors reported correctly
	i = 1; p = nextstmt;
	while(1) {
		p = GetNextCommand(p, &rp, (unsigned char*)"No matching END SELECT");

		if(*p == cmdSELECT_CASE) i++;                                  // found a nested SELECT CASE command, increase the nested count and carry on searching

		// is this a CASE stmt at the same level as this SELECT CASE.
		if(*p == cmdCASE && i == 1) {
			int t;
			MMFLOAT ft, ftt;
			long long int  i64t, i64tt;
			unsigned char* st, * stt;

			CurrentLinePtr = rp;                                    // and report errors at the line we are on

			// loop through the comparison elements on the CASE line.  Each element is separated by a comma
			do {
				p++;
				skipspace(p);
				t = type;
				// check for CASE IS,  eg  CASE IS > 5  -or-  CASE > 5  and process it if it is
				// an operator can be >, <>, etc but it can also be a prefix + or - so we must not catch them
				if((SaveCurrentLinePtr = checkstring(p, (unsigned char*)"IS")) || ((tokentype(*p) & T_OPER) && !(*p == GetTokenValue((unsigned char*)"+") || *p == GetTokenValue((unsigned char*)"-")))) {
					int o;
					if(SaveCurrentLinePtr) p += 2;
					skipspace(p);
					if(tokentype(*p) & T_OPER)
						o = *p++ - C_BASETOKEN;                     // get the operator
					else
						error((char *)"Syntax");
					if(type & T_NBR) ft = f;
					if(type & T_INT) i64t = i64;
					if(type & T_STR) st = s;
					while (o != E_END) p = doexpr(p, &ft, &i64t, &st, &o, &t); // get the right hand side of the expression and evaluate the operator in o
					if(!(t & T_INT)) error((char *)"Syntax");     // comparisons must always return an integer
					if(i64t) {                                      // evaluates to true
						skipelement(p);
						nextstmt = p;
						CurrentLinePtr = SaveCurrentLinePtr;
						return;                                     // if we have a match just return to the interpreter and let it execute the code
					}
					else {                                        // evaluates to false
						skipspace(p);
						continue;
					}
				}

				// it must be either a single value (eg, "foo") or a range (eg, "foo" TO "zoo")
				// evaluate the first value
				p = evaluate(p, &ft, &i64t, &st, &t, true);
				skipspace(p);
				if(*p == tokenTO) {                      // is there is a TO keyword?
					p++;
					t = type;
					p = evaluate(p, &ftt, &i64tt, &stt, &t, false); // evaluate the right hand side of the TO expression
					if(((type & T_NBR) && f >= ft && f <= ftt) || ((type & T_INT) && i64 >= i64t && i64 <= i64tt) || (((type & T_STR) && Mstrcmp(s, st) >= 0) && (Mstrcmp(s, stt) <= 0))) {
						skipelement(p);
						nextstmt = p;
						CurrentLinePtr = SaveCurrentLinePtr;
						return;                                     // if we have a match just return to the interpreter and let it execute the code
					}
					else {
						skipspace(p);
						continue;                                   // otherwise continue searching
					}
				}

				// if we got to here the element must be just a single match.  So make the test
				if(((type & T_NBR) && f == ft) || ((type & T_INT) && i64 == i64t) || ((type & T_STR) && Mstrcmp(s, st) == 0)) {
					skipelement(p);
					nextstmt = p;
					CurrentLinePtr = SaveCurrentLinePtr;
					return;                                         // if we have a match just return to the interpreter and let it execute the code
				}
				skipspace(p);
			} while (*p == ',');                                     // keep looping through the elements on the CASE line
			checkend(p);
			CurrentLinePtr = SaveCurrentLinePtr;
		}

		// test if we have found a CASE ELSE statement at the same level as this SELECT CASE
		// if true it means that we did not find a matching CASE - so execute this code
		if(*p == cmdCASE_ELSE && i == 1) {
			p++;                                                    // step over the token
			checkend(p);
			skipelement(p);
			nextstmt = p;
			CurrentLinePtr = SaveCurrentLinePtr;
			return;
		}

		if(*p == cmdEND_SELECT) i--;                                // found an END SELECT so decrement our nested counter

		if(i == 0) {
			// found our matching END SELECT stmt.  Step over it and continue with the statement after it
			skipelement(p);
			nextstmt = p;
			CurrentLinePtr = SaveCurrentLinePtr;
			return;
		}
	}
}


// if we have hit a CASE or CASE ELSE we must search for a END SELECT at this level and resume at that point
void cmd_case(void) {
	int i;
	unsigned char* p;

	// search through the program looking for a END SELECT statement
	// i tracks the nesting level of any nested SELECT CASE commands
	i = 1; p = nextstmt;
	while (1) {
		p = GetNextCommand(p, NULL, (unsigned char *)"No matching END SELECT");

		if(*p == cmdSELECT_CASE) i++;                               // found a nested SELECT CASE command, we now need to search for its END CASE

		if(*p == cmdEND_SELECT) i--;                                // found an END SELECT so decrement our nested counter
		if(i == 0) {
			// found our matching END SELECT stmt.  Step over it and continue with the statement after it
			skipelement(p);
			nextstmt = p;
			break;
		}
	}
}


void cmd_input(void) {
	unsigned char s[STRINGSIZE];
	unsigned char* p, * sp, * tp;
	int i, fnbr;
	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char*)",;");				// this is a macro and must be the first executable stmt

	// is the first argument a file number specifier?  If so, get it
	if(argc >= 3 && *argv[0] == '#') {
		argv[0]++;
		fnbr = (int)getinteger(argv[0]);
		i = 2;
	}
	else {
		fnbr = 0;
		// is the first argument a prompt?
		// if so, print it followed by an optional question mark
		if(argc >= 3 && *argv[0] == '"' && (*argv[1] == ',' || *argv[1] == ';')) {
			*(argv[0] + strlen((char*)argv[0]) - 1) = 0;
			argv[0]++;
			MMPrintString((char*)argv[0]);
			if(*argv[1] == ';') MMPrintString((char*)"? ");
			i = 2;
		}
		else {
			MMPrintString((char*)"? ");									// no prompt?  then just print the question mark
			i = 0;
		}
	}

	if(argc - i < 1) error((char *)"Syntax");						// no variable to input to
	*inpbuf = 0;													// start with an empty buffer
	MMgetline(fnbr, (char *)inpbuf);									    // get the line
	p = inpbuf;

	// step through the variables listed for the input statement
	// and find the next item on the line and assign it to the variable
	for (; i < argc; i++) {
		sp = s;														// sp is a temp pointer into s[]
		if(*argv[i] == ',' || *argv[i] == ';') continue;
		skipspace(p);
		if(*p != 0) {
			if(*p == '"') {											// if it is a quoted string
				p++;												// step over the quote
				while (*p && *p != '"')  *sp++ = *p++;				// and copy everything upto the next quote
				while (*p && *p != ',') p++;							// then find the next comma
			}
			else {												// otherwise it is a normal string of characters
				while (*p && *p != ',') *sp++ = *p++;				// copy up to the comma
				while (sp > s && sp[-1] == ' ') sp--;				// and trim trailing whitespace
			}
		}
		*sp = 0;													// terminate the string
		tp = (unsigned char*)findvar((unsigned char*)argv[i], V_FIND);								// get the variable and save its new value
		if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
		if(vartbl[VarIndex].type & T_STR) {
			if(strlen((char*)s) > vartbl[VarIndex].size) error((char *)"String too long");
			strcpy((char*)tp, (char*)s);
			CtoM(tp);												// convert to a MMBasic string
		}
		else
			if(vartbl[VarIndex].type & T_INT) {
				*((long long int*)tp) = strtoll((char*)s, (char**)&sp, 10);			// convert to an integer
			}
			else
				*((MMFLOAT*)tp) = (MMFLOAT)atof((char*)s);
		if(*p == ',') p++;
	}
}


void cmd_trace(void) {
	if (checkstring(cmdline, (unsigned char *)"ON"))
		TraceOn = true;
	else if (checkstring(cmdline, (unsigned char*)"OFF"))
		TraceOn = false;
	else if (checkstring(cmdline, (unsigned char*)"LIST")) {
		int i;
		cmdline += 4;
		skipspace(cmdline);
		if (*cmdline == 0 || *cmdline == '\'')  //'
			i = TRACE_BUFF_SIZE - 1;
		else
			i = (int)getint(cmdline, 0, TRACE_BUFF_SIZE - 1);
		i = TraceBuffIndex - i;
		if (i < 0) i += TRACE_BUFF_SIZE;
		while (i != TraceBuffIndex) {
			TraceLines((char *)TraceBuff[i]);
			if (++i >= TRACE_BUFF_SIZE) i = 0;
		}
	}
	else
		error((char *)"Unknown command");
}



// FOR command
void cmd_for(void) {
	int i, t, vlen, test;
	unsigned char ss[4];														// this will be used to split up the argument line
	unsigned char* p, * tp, * xp;
	void* vptr;
	unsigned char* vname, vtype;
	//	static unsigned char fortoken, nexttoken;

		// cache these tokens for speed
	//	if(!fortoken) fortoken = GetCommandValue((unsigned char *)"For");
	//	if(!nexttoken) nexttoken = GetCommandValue((unsigned char *)"Next");

	ss[0] = tokenEQUAL;
	ss[1] = tokenTO;
	ss[2] = tokenSTEP;
	ss[3] = 0;

	{																// start a new block
		getargs(&cmdline, 7, ss);									// getargs macro must be the first executable stmt in a block
		if(argc < 5 || argc == 6 || *argv[1] != ss[0] || *argv[3] != ss[1]) error((char *)"FOR with misplaced = or TO");
		if(argc == 6 || (argc == 7 && *argv[5] != ss[2])) error((char *)"Syntax");

		// get the variable name and trim any spaces
		vname = argv[0];
		if(*vname && *vname == ' ') vname++;
		while (*vname && vname[strlen((char*)vname) - 1] == ' ') vname[strlen((char*)vname) - 1] = 0;
		vlen = strlen((char*)vname);
		vptr = findvar(argv[0], V_FIND);					        // create the variable
		if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
		vtype = TypeMask(vartbl[VarIndex].type);
		if(vtype & T_STR) error((char *)"Invalid variable");                // sanity check

		// check if the FOR variable is already in the stack and remove it if it is
		// this is necessary as the program can jump out of the loop without hitting
		// the NEXT statement and this will eventually result in a stack overflow
		for (i = 0; i < forindex; i++) {
			if(forstack[i].var == vptr && forstack[i].level == LocalIndex) {
				while (i < forindex - 1) {
					forstack[i].forptr = forstack[i + 1].forptr;
					forstack[i].nextptr = forstack[i + 1].nextptr;
					forstack[i].var = forstack[i + 1].var;
					forstack[i].vartype = forstack[i + 1].vartype;
					forstack[i].level = forstack[i + 1].level;
					forstack[i].tovalue.i = forstack[i + 1].tovalue.i;
					forstack[i].stepvalue.i = forstack[i + 1].stepvalue.i;
					i++;
				}
				forindex--;
				break;
			}
		}

		if(forindex == MAXFORLOOPS) error((char *)"Too many nested FOR loops");

		forstack[forindex].var = vptr;								// save the variable index
		forstack[forindex].vartype = vtype;							// save the type of the variable
		forstack[forindex].level = LocalIndex;						// save the level of the variable in terms of sub/funs
		forindex++;                                                 // incase functions use for loops
		if(vtype & T_NBR) {
			*(MMFLOAT*)vptr = getnumber(argv[2]);					// get the starting value for a float and save
			forstack[forindex - 1].tovalue.f = getnumber(argv[4]);		// get the to value and save
			if(argc == 7)
				forstack[forindex - 1].stepvalue.f = getnumber(argv[6]);// get the step value for a float and save
			else
				forstack[forindex - 1].stepvalue.f = 1.0;				// default is +1
		}
		else {
			*(long long int*)vptr = getinteger(argv[2]);			// get the starting value for an integer and save
			forstack[forindex - 1].tovalue.i = getinteger(argv[4]);		// get the to value and save
			if(argc == 7)
				forstack[forindex - 1].stepvalue.i = getinteger(argv[6]);// get the step value for an integer and save
			else
				forstack[forindex - 1].stepvalue.i = 1;					// default is +1
		}
		forindex--;

		forstack[forindex].forptr = nextstmt + 1;					// return to here when looping

		// now find the matching NEXT command
		t = 1; p = nextstmt;
		while (1) {
			p = GetNextCommand(p, &tp, (unsigned char*)"No matching NEXT");
			//            if(*p == fortoken) t++;                                 // count the FOR
			//            if(*p == nexttoken) {                                   // is it NEXT
			if(*p == cmdFOR) t++;                                 // count the FOR
			if(*p == cmdNEXT) {                                   // is it NEXT
				xp = p + 1;											// point to after the NEXT token
				while (*xp && mystrncasecmp(xp, vname, vlen)) xp++;	// step through looking for our variable
				if(*xp && !isnamechar(xp[vlen]))					// is it terminated correctly?
					t = 0;											// yes, found the matching NEXT
				else
					t--;											// no luck, just decrement our stack counter
			}
			if(t == 0) {											// found the matching NEXT
				forstack[forindex].nextptr = p;					// pointer to the start of the NEXT command
				break;
			}
		}

		// test the loop value at the start
		if(forstack[forindex].vartype & T_INT)
			test = (forstack[forindex].stepvalue.i >= 0 && *(long long int*)vptr > forstack[forindex].tovalue.i) || (forstack[forindex].stepvalue.i < 0 && *(long long int*)vptr < forstack[forindex].tovalue.i);
		else
			test = (forstack[forindex].stepvalue.f >= 0 && *(MMFLOAT*)vptr > forstack[forindex].tovalue.f) || (forstack[forindex].stepvalue.f < 0 && *(MMFLOAT*)vptr < forstack[forindex].tovalue.f);

		if(test) {
			// loop is invalid at the start, so go to the end of the NEXT command
			skipelement(p);            // find the command after the NEXT command
			nextstmt = p;              // this is where we will continue
		}
		else {
			forindex++;					// save the loop data and continue on with the command after the FOR statement
		}
	}
}



void cmd_next(void) {
	int i, vindex, test;
	void* vtbl[MAXFORLOOPS];
	int vcnt;
	unsigned char* p;
	getargs(&cmdline, MAXFORLOOPS * 2, (unsigned char*)(unsigned char *)",");						// getargs macro must be the first executable stmt in a block

	vindex = 0;														// keep lint happy

	for (vcnt = i = 0; i < argc; i++) {
		if(i & 0x01) {
			if(*argv[i] != ',') error((char *)"Syntax");
		}
		else
			vtbl[vcnt++] = findvar(argv[i], V_FIND | V_NOFIND_ERR); // find the variable and error if not found
	}

loopback:
	// first search the for stack for a loop with the same variable specified on the NEXT's line
	if(vcnt) {
		for (i = forindex - 1; i >= 0; i--)
			for (vindex = vcnt - 1; vindex >= 0; vindex--)
				if(forstack[i].var == vtbl[vindex])
					goto breakout;
	}
	else {
		// if no variables specified search the for stack looking for an entry with the same program position as
		// this NEXT statement. This cheats by using the cmdline as an identifier and may not work inside an IF THEN ELSE
		for (i = 0; i < forindex; i++) {
			p = forstack[i].nextptr + 1;
			skipspace(p);
			if(p == cmdline) goto breakout;
		}
	}

	error((char *)"Cannot find a matching FOR");

breakout:

	// found a match
	// apply the STEP value to the variable and test against the TO value
	if(forstack[i].vartype & T_INT) {
		*(long long int*)forstack[i].var += forstack[i].stepvalue.i;
		test = (forstack[i].stepvalue.i >= 0 && *(long long int*)forstack[i].var > forstack[i].tovalue.i) || (forstack[i].stepvalue.i < 0 && *(long long int*)forstack[i].var < forstack[i].tovalue.i);
	}
	else {
		*(MMFLOAT*)forstack[i].var += forstack[i].stepvalue.f;
		test = (forstack[i].stepvalue.f >= 0 && *(MMFLOAT*)forstack[i].var > forstack[i].tovalue.f) || (forstack[i].stepvalue.f < 0 && *(MMFLOAT*)forstack[i].var < forstack[i].tovalue.f);
	}

	if(test) {
		// the loop has terminated
		// remove the entry in the table, then skip forward to the next element and continue on from there
		while (i < forindex - 1) {
			forstack[i].forptr = forstack[i + 1].forptr;
			forstack[i].nextptr = forstack[i + 1].nextptr;
			forstack[i].var = forstack[i + 1].var;
			forstack[i].vartype = forstack[i + 1].vartype;
			forstack[i].level = forstack[i + 1].level;
			forstack[i].tovalue.i = forstack[i + 1].tovalue.i;
			forstack[i].stepvalue.i = forstack[i + 1].stepvalue.i;
			i++;
		}
		forindex--;
		if(vcnt > 0) {
			// remove that entry from our FOR stack
			for (; vindex < vcnt - 1; vindex++) vtbl[vindex] = vtbl[vindex + 1];
			vcnt--;
			if(vcnt > 0)
				goto loopback;
			else
				return;
		}

	}
	else {
		// we have not reached the terminal value yet, so go back and loop again
		nextstmt = forstack[i].forptr;
	}
}




void cmd_do(void) {
	int i;
	unsigned char* p, * tp, * evalp;
	if(cmdtoken == cmdWHILE)error((char *)"Unknown command");
	// if it is a DO loop find the WHILE token and (if found) get a pointer to its expression
	while (*cmdline && *cmdline != tokenWHILE) cmdline++;
	if(*cmdline == tokenWHILE) {
		evalp = ++cmdline;
	}
	else
		evalp = NULL;
	// check if this loop is already in the stack and remove it if it is
	// this is necessary as the program can jump out of the loop without hitting
	// the LOOP or WEND stmt and this will eventually result in a stack overflow
	for (i = 0; i < doindex; i++) {
		if(dostack[i].doptr == nextstmt) {
			while (i < doindex - 1) {
				dostack[i].evalptr = dostack[i + 1].evalptr;
				dostack[i].loopptr = dostack[i + 1].loopptr;
				dostack[i].doptr = dostack[i + 1].doptr;
				dostack[i].level = dostack[i + 1].level;
				i++;
			}
			doindex--;
			break;
		}
	}

	// add our pointers to the top of the stack
	if(doindex == MAXDOLOOPS) error((char *)"Too many nested DO or WHILE loops");
	dostack[doindex].evalptr = evalp;
	dostack[doindex].doptr = nextstmt;
	dostack[doindex].level = LocalIndex;

	// now find the matching LOOP command
	i = 1; p = nextstmt;
	while (1) {
		p = GetNextCommand(p, &tp, (unsigned char*)"No matching LOOP");
		if(*p == cmdtoken) i++;                                     // entered a nested DO or WHILE loop
		if(*p == cmdLOOP) i--;									// exited a nested loop

		if(i == 0) {												// found our matching LOOP or WEND stmt
			dostack[doindex].loopptr = p;
			break;
		}
	}

	if(dostack[doindex].evalptr != NULL) {
		// if this is a DO WHILE ... LOOP statement
		// search the LOOP statement for a WHILE or UNTIL token (p is pointing to the matching LOOP statement)
		p++;
		while (*p && *p < 0x80) p++;
		if(*p == tokenWHILE) error((char *)"LOOP has a WHILE test");
		if(*p == tokenUNTIL) error((char *)"LOOP has an UNTIL test");
	}

	doindex++;

	// do the evaluation (if there is something to evaluate) and if false go straight to the command after the LOOP or WEND statement
	if(dostack[doindex - 1].evalptr != NULL && getnumber(dostack[doindex - 1].evalptr) == 0) {
		doindex--;                                                  // remove the entry in the table
		nextstmt = dostack[doindex].loopptr;                        // point to the LOOP or WEND statement
		skipelement(nextstmt);                                      // skip to the next command
	}

}




void cmd_loop(void) {
	unsigned char* p;
	int tst = 0;                                                    // initialise tst to stop the compiler from complaining
	int i;

	// search the do table looking for an entry with the same program position as this LOOP statement
	for (i = 0; i < doindex; i++) {
		p = dostack[i].loopptr + 1;
		skipspace(p);
		if(p == cmdline) {
			// found a match
			// first check if the DO statement had a WHILE component
			// if not find the WHILE statement here and evaluate it
			if(dostack[i].evalptr == NULL) {						// if it was a DO without a WHILE
				if(*cmdline >= 0x80) {								// if there is something
					if(*cmdline == tokenWHILE)
						tst = (getnumber(++cmdline) != 0);			// evaluate the expression
					else if(*cmdline == tokenUNTIL)
						tst = (getnumber(++cmdline) == 0);			// evaluate the expression
					else
						error((char *)"Syntax");
				}
				else {
					tst = 1;										// and loop forever
					checkend(cmdline);								// make sure that there is nothing else
				}
			}
			else {													// if was DO WHILE
				tst = (getnumber(dostack[i].evalptr) != 0);			// evaluate its expression
				checkend(cmdline);									// make sure that there is nothing else
			}

			// test the expression value and reset the program pointer if we are still looping
			// otherwise remove this entry from the do stack
			if(tst)
				nextstmt = dostack[i].doptr;						// loop again
			else{
				// the loop has terminated
				// remove the entry in the table, then just let the default nextstmt run and continue on from there
				doindex = i;
				// just let the default nextstmt run
			}
			return;
		}
	}
	error((char *)"LOOP without a matching DO");
}



void cmd_exitfor(void) {
	if(forindex == 0) error((char *)"No FOR loop is in effect");
	nextstmt = forstack[--forindex].nextptr;
	checkend(cmdline);
	skipelement(nextstmt);
}



void cmd_exit(void) {
	if(doindex == 0) error((char *)"No DO loop is in effect");
	nextstmt = dostack[--doindex].loopptr;
	checkend(cmdline);
	skipelement(nextstmt);
}



void cmd_error(void) {
	char* s, p[STRINGSIZE];
	if (*cmdline && *cmdline != '\'') {
		s = (char *)getCstring(cmdline);

		strcpy(p, s);
		error(p);
	}
	else
		error((char *)"");
}



void cmd_randomize(void) {
	int i;
	getargs(&cmdline, 1, (unsigned char *)",");
	if(argc == 1)i = (int)getinteger(argv[0]);
	else {
		int64_t j;
		QueryPerformanceCounter((LARGE_INTEGER*)&j);
		i = (int)(j & 0xFFFFFFFF);
	}
	if(i < 0) error((char *)"Number out of bounds");
	srand(i);
}



// this is the Sub or Fun command
// it simply skips over text until it finds the end of it
void cmd_subfun(void) {
	unsigned char* p, returntoken, errtoken;

	if(gosubindex != 0) error((char *)"No matching END declaration");       // we have hit a SUB/FUN while in another SUB or FUN
	if(cmdtoken == cmdSUB) {
		returntoken = cmdENDSUB;
		errtoken = cmdENDFUNCTION;
	}
	else {
		returntoken = cmdENDFUNCTION;
		errtoken = cmdENDSUB;
	}
	p = nextstmt;
	while (1) {
		p = GetNextCommand(p, NULL, (unsigned char *)"No matching END declaration");
		if(*p == cmdSUB || *p == cmdFUN || *p == errtoken) error((char *)"No matching END declaration");
		if(*p == returntoken) {                                     // found the next return
			skipelement(p);
			nextstmt = p;                                           // point to the next command
			break;
		}
	}
}




void cmd_gosub(void) {
	if(gosubindex >= MAXGOSUB) error((char *)"Too many nested GOSUB");
	errorstack[gosubindex] = CurrentLinePtr;
	gosubstack[gosubindex++] = nextstmt;
	LocalIndex++;
	if(isnamestart(*cmdline))
		nextstmt = findlabel(cmdline);								// must be a label
	else
		nextstmt = findline((int)getinteger(cmdline), true);				// try for a line number
	CurrentLinePtr = nextstmt;
}


void cmd_mid(void) {
	unsigned char* p;
	getargs(&cmdline, 5, (unsigned char *)",");
	findvar(argv[0], V_NOFIND_ERR);
	if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
	if(!(vartbl[VarIndex].type & T_STR)) error((char *)"Not a string");
	unsigned char* sourcestring = getstring(argv[0]);
	int start = (int)getint(argv[2], 1, sourcestring[0]);
	int num = 0;
	if(argc == 5)num = (int)getint(argv[4], 1, sourcestring[0]);
	if(start + num - 1 > sourcestring[0])error((char *)"Selection exceeds length of string");
	while (*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
	if(!*cmdline) error((char *)"Syntax");
	++cmdline;
	if(!*cmdline) error((char *)"Syntax");
	char* value = (char*)getstring(cmdline);
	if(num == 0)num = value[0];
	if(num > value[0])error((char *)"Supplied string too short");
	p = (unsigned char*)&value[1];
	memcpy(&sourcestring[start], p, num);
}

void cmd_return(void) {
	checkend(cmdline);
	if(gosubindex == 0 || gosubstack[gosubindex - 1] == NULL) error((char *)"Nothing to return to");
	ClearVars(LocalIndex--);                                        // delete any local variables
	TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
	nextstmt = gosubstack[--gosubindex];                            // return to the caller
	CurrentLinePtr = errorstack[gosubindex];
}




void cmd_endfun(void) {
	checkend(cmdline);
	if(gosubindex == 0 || gosubstack[gosubindex - 1] != NULL) error((char *)"Nothing to return to");
	nextstmt = (unsigned char*)"\0\0\0";                                            // now terminate this run of ExecuteProgram()
}



void cmd_read(void) {
	int i, j, k, len, card;
	unsigned char *p, datatoken, *lineptr = NULL, *ptr;
	int vcnt, vidx, num_to_read = 0;
	if (checkstring(cmdline, (unsigned char*)"SAVE")) {
		if(restorepointer== MAXRESTORE - 1)error((char*)"Too many saves");
		datastore[restorepointer].SaveNextDataLine = NextDataLine;
		datastore[restorepointer].SaveNextData = NextData;
		restorepointer++;
		return;
	}
	if (checkstring(cmdline, (unsigned char*)"RESTORE")) {
		if (!restorepointer)error((char*)"Nothing to restore");
		restorepointer--;
		NextDataLine = datastore[restorepointer].SaveNextDataLine;
		NextData = datastore[restorepointer].SaveNextData;
		return;
	}

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");                // getargs macro must be the first executable stmt in a block
	if(argc == 0) error((char *)"Syntax");
	// first count the elements and do the syntax checking
	for(vcnt = i = 0; i < argc; i++) {
		if(i & 0x01) {
			if(*argv[i] != ',') error((char *)"Syntax");
		}
		else{
			findvar(argv[i], V_FIND | V_EMPTY_OK);
			if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
			card = 1;
			if(emptyarray) { //empty array
				for(k = 0; k < MAXDIM; k++) {
					j = (vartbl[VarIndex].dims[k] - OptionBase + 1);
					if(j)card *= j;
				}
			}
			num_to_read += card;
		}
	}
	char** vtbl = (char **)GetTempMemory(num_to_read * sizeof(char*));
	int* vtype = (int *)GetTempMemory(num_to_read * sizeof(int));
	int* vsize = (int *)GetTempMemory(num_to_read * sizeof(int));
	// step through the arguments and save the pointer and type
	for(vcnt = i = 0; i < argc; i += 2) {
		vtbl[vcnt] = (char *)findvar(argv[i], V_FIND | V_EMPTY_OK);
		ptr = (unsigned char *)vtbl[vcnt];
		card = 1;
		if(emptyarray) { //empty array
			for (k = 0; k < MAXDIM; k++) {
				j = (vartbl[VarIndex].dims[k] - OptionBase + 1);
				if(j)card *= j;
			}
		}
		for (k = 0; k < card; k++) {
			if(k) {
				if(vartbl[VarIndex].type & (T_INT | T_NBR))ptr += 8;
				else ptr += vartbl[VarIndex].size + 1;
				vtbl[vcnt] = (char*)ptr;
			}
			vtype[vcnt] = TypeMask(vartbl[VarIndex].type);
			vsize[vcnt] = vartbl[VarIndex].size;
			vcnt++;
		}
	}

	// setup for a search through the whole memory
	vidx = 0;
	datatoken = GetCommandValue((unsigned char *)"Data");
	p = lineptr = NextDataLine;
	if(*p == 0xff) error((char *)"No DATA to read");                        // error if there is no program

  // search looking for a DATA statement.  We keep returning to this point until all the data is found
search_again:
	while (1) {
		if(*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
		if(*p == 0 || *p == 0xff) error((char *)"No DATA to read");         // end of the program and we still need more data
		if(*p == T_NEWLINE) lineptr = p++;
		if(*p == T_LINENBR) p += 3;
		skipspace(p);
		if(*p == T_LABEL) {                                         // if there is a label here
			p += p[1] + 2;                                          // skip over the label
			skipspace(p);                                           // and any following spaces
		}
		if(*p == datatoken) break;                                  // found a DATA statement
		while (*p) p++;                                              // look for the zero marking the start of the next element
	}
	NextDataLine = lineptr;
	p++;                                                            // step over the token
	skipspace(p);
	if(!*p || *p == '\'') { CurrentLinePtr = lineptr; error((char *)"No DATA to read"); }

	// we have a DATA statement, first split the line into arguments
	{                                                           // new block, the getargs macro must be the first executable stmt in a block
		getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char *)",");
		if((argc & 1) == 0) { CurrentLinePtr = lineptr; error((char *)"Syntax"); }
		// now step through the variables on the READ line and get their new values from the argument list
		// we set the line number to the number of the DATA stmt so that any errors are reported correctly
		while (vidx < vcnt) {
			// check that there is some data to read if not look for another DATA stmt
			if(NextData > argc) {
				skipline(p);
				NextData = 0;
				goto search_again;
			}
			CurrentLinePtr = lineptr;
			if(vtype[vidx] & T_STR) {
				char* p1, * p2;
				if(*argv[NextData] == '"') {                               // if quoted string
					for (len = 0, p1 = vtbl[vidx], p2 = (char*)argv[NextData] + 1; *p2 && *p2 != '"'; len++, p1++, p2++) {
						*p1 = *p2;                                   // copy up to the quote
					}
				}
				else {                                            // else if not quoted
					for (len = 0, p1 = vtbl[vidx], p2 = (char*)argv[NextData]; *p2 && *p2 != '\''; len++, p1++, p2++) {
						if(*p2 < 0x20 || *p2 >= 0x7f) error((char *)"Invalid character");
						*p1 = *p2;                                  // copy up to the comma
					}
				}
				if(len > vsize[vidx]) error((char *)"String too long");
				*p1 = 0;                                            // terminate the string
				CtoM((unsigned char*)vtbl[vidx]);                                   // convert to a MMBasic string
			}
			else if(vtype[vidx] & T_INT)
				*((long long int*)vtbl[vidx]) = getinteger(argv[NextData]); // much easier if integer variable
			else
				*((MMFLOAT*)vtbl[vidx]) = getnumber(argv[NextData]);      // same for numeric variable

			vidx++;
			NextData += 2;
		}
	}
}

void cmd_call(void) {
	int i;
	unsigned char* q;
	unsigned char* p = getCstring(cmdline); //get the command we want to call
/*	q=p;
	while(*q){ //convert to upper case for the match
		*q=mytoupper(*q);
		q++;
	}*/
	q = cmdline;
	while (*q) {
		if(*q == ',' || *q == '\'')break;
		q++;
	}
	if(*q == ',')q++;
	i = FindSubFun(p, false);                   // it could be a defined command
	strcat((char *)p, " ");
	strcat((char*)p, (const char *)q);
	//	MMPrintString(p);PRet();
	if(i >= 0) {                                // >= 0 means it is a user defined command
		DefinedSubFun(false, p, i, NULL, NULL, NULL, NULL);
	}
	else
		error((char *)"Unknown user subroutine");
}



void cmd_restore(void) {
	if(*cmdline == 0 || *cmdline == '\'') {
		NextDataLine = ProgMemory;
		NextData = 0;
	}
	else {
		skipspace(cmdline);
		if(*cmdline == '"') {
			NextDataLine = findlabel(getCstring(cmdline));
			NextData = 0;
		}
		else if(isdigit(*cmdline) || *cmdline == GetTokenValue((unsigned char*)"+") || *cmdline == GetTokenValue((unsigned char*)"-") || *cmdline == '.') {
			NextDataLine = findline((int)getinteger(cmdline), true);		// try for a line number
			NextData = 0;
		}
		else {
			void* ptr = findvar(cmdline, V_NOFIND_NULL);
			if(ptr) {
				if(vartbl[VarIndex].type & T_NBR) {
					if(vartbl[VarIndex].dims[0] > 0) {		// Not an array
						error((char *)"Syntax");
					}
					NextDataLine = findline((int)getinteger(cmdline), true);
				}
				else if(vartbl[VarIndex].type & T_INT) {
					if(vartbl[VarIndex].dims[0] > 0) {		// Not an array
						error((char *)"Syntax");
					}
					NextDataLine = findline((int)getinteger(cmdline), true);
				}
				else {
					NextDataLine = findlabel(getCstring(cmdline));					    // must be a label
				}
			}
			else if(isnamestart(*cmdline)) {
				NextDataLine = findlabel(cmdline);					    // must be a label
			}
			NextData = 0;
		}
	}
}




void cmd_lineinput(void) {
	unsigned char* vp;
	int i, fnbr;
	getargs(&cmdline, 3, (unsigned char*)",;");										// this is a macro and must be the first executable stmt
	if(argc == 0 || argc == 2) error((char *)"Syntax");

	i = 0;
	fnbr = 0;
	if(argc == 3) {
		// is the first argument a file number specifier?  If so, get it
		if(*argv[0] == '#' && *argv[1] == ',') {
			argv[0]++;
			fnbr = (int)getinteger(argv[0]);
		}
		else {
			// is the first argument a prompt?  if so, print it otherwise there are too many arguments
			if(*argv[1] != ',' && *argv[1] != ';') error((char *)"Syntax");
			MMfputs((unsigned char*)getstring(argv[0]), 0);
		}
		i = 2;
	}

	if(argc - i != 1) error((char *)"Syntax");
	vp = (unsigned char*)findvar(argv[i], V_FIND);
	if(vartbl[VarIndex].type & T_CONST) error((char *)"Cannot change a constant");
	if(!(vartbl[VarIndex].type & T_STR)) error((char *)"Invalid variable");
	MMgetline(fnbr, (char *)inpbuf);									    // get the input line
	if(strlen((char*)inpbuf) > vartbl[VarIndex].size) error((char *)"String too long");
	strcpy((char*)vp, (char*)inpbuf);
	CtoM(vp);														// convert to a MMBasic string
}


void cmd_on(void) {
	int r;
	unsigned char ss[4];													    // this will be used to split up the argument line
	unsigned char* p;
	// first check if this is:   ON KEY location
	p = checkstring(cmdline, (unsigned char*)"KEY");
	if(p) {
		getargs(&p, 3, (unsigned char *)",");
		if(argc == 1) {
			if(*argv[0] == '0' && !isdigit(*(argv[0] + 1))) {
				OnKeyGOSUB = NULL;                                      // the program wants to turn the interrupt off
			}
			else {
				OnKeyGOSUB = GetIntAddress(argv[0]);						    // get a pointer to the interrupt routine
				InterruptUsed = true;
			}
			return;
		}
		else {
			keyselect = (int)getint(argv[0], 0, 255);
			if(keyselect == 0) {
				KeyInterrupt = NULL;                                      // the program wants to turn the interrupt off
			}
			else {
				if(*argv[2] == '0' && !isdigit(*(argv[2] + 1))) {
					KeyInterrupt = NULL;                                      // the program wants to turn the interrupt off
				}
				else {
					KeyInterrupt = GetIntAddress((unsigned char *)argv[2]);						    // get a pointer to the interrupt routine
					InterruptUsed = true;
				}
			}
			return;
		}
	}
	p = checkstring(cmdline, (unsigned char *)"ERROR");
	if (p) {
		if (checkstring(p, (unsigned char*)"ABORT")) {
			OptionErrorSkip = 0;
			return;
		}
		MMerrno = 0;                                                // clear the error flags
		if(MMErrMsg!=NULL)*MMErrMsg = 0;
		if (checkstring(p, (unsigned char*)"CLEAR")) return;
		if (checkstring(p, (unsigned char*)"IGNORE")) {
			OptionErrorSkip = -1;
			return;
		}
		if ((p = checkstring(p, (unsigned char*)"SKIP"))) {
			if (*p == 0 || *p == '\'')
				OptionErrorSkip = 1;
			else
				OptionErrorSkip = (int)getint(p, 1, 10000);
			return;
		}
		error((char*)"Syntax");
	}

	// if we got here the command must be the traditional:  ON nbr GOTO|GOSUB line1, line2,... etc

	ss[0] = tokenGOTO;
	ss[1] = tokenGOSUB;
	ss[2] = ',';
	ss[3] = 0;
	{																// start a new block
		getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ss);				// getargs macro must be the first executable stmt in a block
		if(argc < 3 || !(*argv[1] == ss[0] || *argv[1] == ss[1])) error((char *)"Syntax");
		if(argc % 2 == 0) error((char *)"Syntax");

		r = (int)getint(argv[0], 0, 255);									// evaluate the expression controlling the statement
		if(r == 0 || r > argc / 2) return;							// microsoft say that we just go on to the next line

		if(*argv[1] == ss[1]) {
			// this is a GOSUB, same as a GOTO but we need to first push the return pointer
			if(gosubindex >= MAXGOSUB) error((char *)"Too many nested GOSUB");
			errorstack[gosubindex] = CurrentLinePtr;
			gosubstack[gosubindex++] = nextstmt;
			LocalIndex++;
		}

		if(isnamestart(*argv[r * 2]))
			nextstmt = findlabel(argv[r * 2]);						// must be a label
		else
			nextstmt = findline((int)getinteger(argv[r * 2]), true);		// try for a line number
	}
	//    IgnorePIN = false;
}


unsigned char* SetValue(unsigned char* p, int t, void* v) {
	MMFLOAT f;
	long long int  i64;
	unsigned char* s;
	char TempCurrentSubFunName[MAXVARLEN + 1];
	strcpy(TempCurrentSubFunName, (char*)CurrentSubFunName);			    // save the current sub/fun name
	if(t & T_STR) {
		p = evaluate(p, &f, &i64, &s, &t, true);
		Mstrcpy((unsigned char *)v, s);
	}
	else if(t & T_NBR) {
		p = evaluate(p, &f, &i64, &s, &t, false);
		if(t & T_NBR)
			(*(MMFLOAT*)v) = f;
		else
			(*(MMFLOAT*)v) = (MMFLOAT)i64;
	}
	else {
		p = evaluate(p, &f, &i64, &s, &t, false);
		if(t & T_INT)
			(*(long long int*)v) = i64;
		else
			(*(long long int*)v) = FloatToInt64(f);
	}
	strcpy((char*)CurrentSubFunName, TempCurrentSubFunName);			    // restore the current sub/fun name
	return p;
}



// define a variable
// DIM [AS INTEGER|FLOAT|STRING] var[(d1 [,d2,...]] [AS INTEGER|FLOAT|STRING] [, ..., ...]
// LOCAL also uses this function the routines only differ in that LOCAL can only be used in a sub/fun
void cmd_dim(void) {
	int i, j, k, type, typeSave, ImpliedType = 0, VIndexSave, StaticVar = false;
	unsigned char* p, chSave, * chPosit;
	unsigned char VarName[(MAXVARLEN * 2) + 1];
	void* v, * tv;

	if(*cmdline == tokenAS) cmdline++;                              // this means that we can use DIM AS INTEGER a, b, etc
	p = CheckIfTypeSpecified(cmdline, &type, true);                 // check for DIM FLOAT A, B, ...
	ImpliedType = type;
	{                                                               // getargs macro must be the first executable stmt in a block
		getargs(&p, (MAX_ARG_COUNT * 2) - 1, (unsigned char*)(unsigned char *)",");
		if((argc & 0x01) == 0) error((char *)"Syntax");

		for (i = 0; i < argc; i += 2) {
			p = skipvar(argv[i], false);                            // point to after the variable
			while (!(*p == 0 || *p == tokenAS || *p == (unsigned char)'\'' || *p == tokenEQUAL))
				p++;                                                // skip over a LENGTH keyword if there and see if we can find "AS"
			chSave = *p; chPosit = p; *p = 0;                       // save the char then terminate the string so that LENGTH is evaluated correctly
			if(chSave == tokenAS) {                                 // are we using Microsoft syntax (eg, AS INTEGER)?
				if(ImpliedType & T_IMPLIED) error((char *)"Type specified twice");
				p++;                                                // step over the AS token
				p = CheckIfTypeSpecified(p, &type, true);           // and get the type
				if(!(type & T_IMPLIED)) error((char *)"Variable type");
			}

			if(cmdtoken == cmdLOCAL) {
				if(LocalIndex == 0) error((char *)"Invalid here");
				type |= V_LOCAL;                                    // local if defined in a sub/fun
			}

			if(cmdtoken == cmdSTATIC) {
				if(LocalIndex == 0) error((char *)"Invalid here");
				// create a unique global name
				if(*CurrentInterruptName)
					strcpy((char *)VarName, (const char *)CurrentInterruptName);          // we must be in an interrupt sub
				else
					strcpy((char*)VarName, (const char*)CurrentSubFunName);             // normal sub/fun
				for (k = 1; k <= MAXVARLEN; k++) if(!isnamechar(VarName[k])) {
					VarName[k] = 0;                                 // terminate the string on a non valid char
					break;
				}
				strcat((char*)VarName, (const char*)argv[i]);					        // by prefixing the var name with the sub/fun name
				StaticVar = true;
			}
			else
				strcpy((char*)VarName, (const char*)argv[i]);

			v = findvar(VarName, type | V_NOFIND_NULL);             // check if the variable exists
			typeSave = type;
			VIndexSave = VarIndex;
			if(v == NULL) {											// if not found
				v = findvar(VarName, type | V_FIND | V_DIM_VAR);    // create the variable
				type = TypeMask(vartbl[VarIndex].type);
				VIndexSave = VarIndex;
				*chPosit = chSave;                                  // restore the char previously removed
				if(vartbl[VarIndex].dims[0] == -1) error((char *)"Array dimensions");
				if(vartbl[VarIndex].dims[0] > 0) {
					DimUsed = true;                                 // prevent OPTION BASE from being used
					v = vartbl[VarIndex].val.s;
				}
				while (*p && *p != '\'' && tokenfunction(*p) != op_equal) p++;	// search through the line looking for the equals sign
				if(tokenfunction(*p) == op_equal) {
					p++;                                            // step over the equals sign
					skipspace(p);
					if(vartbl[VarIndex].dims[0] > 0 && *p == '(') {
						// calculate the overall size of the array
						for (j = 1, k = 0; k < MAXDIM && vartbl[VIndexSave].dims[k]; k++) {
							j *= (vartbl[VIndexSave].dims[k] + 1 - OptionBase);
						}
						do {
							p++;                                    // step over the opening bracket or terminating comma
							p = SetValue(p, type, v);
							if(type & T_STR) v = (char*)v + vartbl[VIndexSave].size + 1;
							if(type & T_NBR) v = (char*)v + sizeof(MMFLOAT);
							if(type & T_INT) v = (char*)v + sizeof(long long int);
							skipspace(p); j--;
						} while (j > 0 && *p == ',');
						if(*p != ')') error((char *)"Number of initialising values");
						if(j != 0) error((char *)"Number of initialising values");
					}
					else
						SetValue(p, type, v);
				}
				type = ImpliedType;
			}
			else {
				if(!StaticVar) error((char *)"$ already declared", VarName);
			}


			// if it is a STATIC var create a local var pointing to the global var
			if(StaticVar) {
				tv = findvar(argv[i], typeSave | V_LOCAL | V_NOFIND_NULL);    						// check if the local variable exists
				if(tv != NULL) error((char *)"$ already declared", argv[i]);
				tv = findvar(argv[i], typeSave | V_LOCAL | V_FIND | V_DIM_VAR);         			// create the variable
				if(vartbl[VIndexSave].dims[0] > 0 || (vartbl[VIndexSave].type & T_STR)) {
					FreeMemory((unsigned char *)tv);                                                                 // we don't need the memory allocated to the local
					vartbl[VarIndex].val.s = vartbl[VIndexSave].val.s;                              // point to the memory of the global variable
				}
				else
					vartbl[VarIndex].val.ia = &(vartbl[VIndexSave].val.i);                    		// point to the data of the variable
				vartbl[VarIndex].type = vartbl[VIndexSave].type | T_PTR;           					// set the type to a pointer
				vartbl[VarIndex].size = vartbl[VIndexSave].size;                   					// just in case it is a string copy the size
				for (j = 0; j < MAXDIM; j++) vartbl[VarIndex].dims[j] = vartbl[VIndexSave].dims[j];  // just in case it is an array copy the dimensions
			}
		}
	}
}




void  cmd_const(void) {
	unsigned char* p;
	void* v;
	int i, type;

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, (unsigned char*)(unsigned char *)",");				// getargs macro must be the first executable stmt in a block
	if((argc & 0x01) == 0) error((char *)"Syntax");

	for (i = 0; i < argc; i += 2) {
		p = skipvar(argv[i], false);                                // point to after the variable
		skipspace(p);
		if(tokenfunction(*p) != op_equal) error((char *)"Syntax");  // must be followed by an equals sign
		p++;                                                        // step over the equals sign
		type = T_NOTYPE;
		v = DoExpression(p, &type);                                 // evaluate the constant's value
		type = TypeMask(type);
		type |= V_FIND | V_DIM_VAR | T_CONST | T_IMPLIED;
		if(LocalIndex != 0) type |= V_LOCAL;                        // local if defined in a sub/fun
		findvar(argv[i], type);                                     // create the variable
		if(vartbl[VarIndex].dims[0] != 0) error((char *)"Invalid constant");
		if(TypeMask(vartbl[VarIndex].type) != TypeMask(type)) error((char *)"Invalid constant");
		else {
			if(type & T_NBR) vartbl[VarIndex].val.f = *(MMFLOAT*)v;           // and set its value
			if(type & T_INT) vartbl[VarIndex].val.i = *(long long int*)v;
			if(type & T_STR) Mstrcpy((unsigned char*)vartbl[VarIndex].val.s, (unsigned char*)v);
		}
	}
}


// utility function used by llist() below
// it copys a command or function honouring the case selected by the user
void strCopyWithCase(unsigned char* d, unsigned char* s) {
	if(Option.Listcase == CONFIG_LOWER) {
		while (*s) *d++ = tolower(*s++);
	}
	else if(Option.Listcase == CONFIG_UPPER) {
		while (*s) *d++ = mytoupper(*s++);
	}
	else {
		while (*s) *d++ = *s++;
	}
	*d = 0;
}


// list a line into a buffer (b) given a pointer to the beginning of the line (p).
// the returned string is a C style string (terminated with a zero)
// this is used by cmd_list(), cmd_edit() and cmd_xmodem()
unsigned char* llist(unsigned char* b, unsigned char* p) {
	int i, firstnonwhite = true;
	unsigned char* b_start = b;

	while (1) {
		if (*p == T_NEWLINE) {
			p++;
			firstnonwhite = true;
			continue;
		}

		if (*p == T_LINENBR) {
			i = (((p[1]) << 8) | (p[2]));							// get the line number
			p += 3;													// and step over the number
			IntToStr((char *)b, i, 10);
			b += strlen((char*)b);
			if (*p != ' ') *b++ = ' ';
		}

		if (*p == T_LABEL) {											// got a label
			for (i = p[1], p += 2; i > 0; i--)
				*b++ = *p++;										// copy to the buffer
			*b++ = ':';												// terminate with a colon
			if (*p && *p != ' ') *b++ = ' ';							// and a space if necessary
			firstnonwhite = true;
		}														// this deliberately drops through in case the label is the only thing on the line

		if (*p >= C_BASETOKEN) {
			if (firstnonwhite) {
				if (*p == GetCommandValue((unsigned char*)"Let"))
					*b = 0;											// use nothing if it LET
				else {
					strCopyWithCase(b, commandname(*p));			// expand the command (if it is not LET)
					b += strlen((char*)b);                                 // update pointer to the end of the buffer
					if (isalpha(*(b - 1))) *b++ = ' ';               // add a space to the end of the command name
				}
				firstnonwhite = false;
			}
			else {												// not a command so must be a token
				strCopyWithCase(b, tokenname(*p));					// expand the token
				b += strlen((char*)b);                                 // update pointer to the end of the buffer
				if (*p == tokenTHEN || *p == tokenELSE)
					firstnonwhite = true;
				else
					firstnonwhite = false;
			}
			p++;
			continue;
		}

		// hey, an ordinary char, just copy it to the output
		if (*p) {
			*b = *p;												// place the char in the buffer
			if (*p != ' ') firstnonwhite = false;
			p++;  b++;												// move the pointers
			continue;
		}

		// at this point the char must be a zero
		// zero char can mean both a separator or end of line
		if (!(p[1] == T_NEWLINE || p[1] == 0)) {
			*b++ = ':';												// just a separator
			firstnonwhite = true;
			p++;
			continue;
		}

		// must be the end of a line - so return to the caller
		while (*(b - 1) == ' ' && b > b_start) --b;                    // eat any spaces on the end of the line
		*b = 0;														// terminate the output buffer
		return ++p;
	} // end while
}



// get the file name for the RUN, LOAD, SAVE, KILL, COPY, DRIVE, FILES, FONT LOAD, LOADBMP, SAVEBMP, SPRITE LOAD and EDIT commands
// this function allows the user to omit the quote marks around a string constant when in immediate mode
// the first argument is a pointer to the file name (normally on the command line of the command)
// the second argument is a pointer to the LastFile buffer.  If this pointer is NULL the LastFile buffer feature will not be used
// This returns a temporary string to the filename
unsigned char* GetFileName(unsigned char* CmdLinePtr, unsigned char* LastFilePtr) {
	unsigned char* tp, * t;

	if(CurrentLinePtr) return getCstring(CmdLinePtr);               // if running a program get the filename as an expression

	// if we reach here we are in immediate mode
	if(*CmdLinePtr == 0 && LastFilePtr != NULL) return LastFilePtr; // if the command line is empty and we have a pointer to the last file, return that

	if(strchr((char*)CmdLinePtr, '"') == NULL && strchr((char*)CmdLinePtr, '$') == NULL) {// quotes or a $ symbol indicate that it is an expression
		tp = (unsigned char*)GetTempMemory(STRINGSIZE);                                  // this will last for the life of the command
		strcpy((char*)tp, (char*)CmdLinePtr);	                                    // save the string
		t = (unsigned char*)strchr((char*)tp, ' '); if(t) *t = 0;                          // trim any trailing spaces
		for (t = tp; *t; t++) if(*t <= ' ' || *t > 'z') error((char *)"Filename must be quoted");
		return tp;
	}
	else
		return getCstring(CmdLinePtr);	                            // treat the command line as a string expression and get its value
}
// lists the program to a specified file handle
// this decodes line numbers and tokens and outputs them in plain english
// LISTing a program is exactly the same as listing to a file (ie, SAVE)
void flist(int fnbr, int fromnbr, int tonbr) {
	unsigned char* fromp = ProgMemory + 1;
	unsigned char b[STRINGSIZE];
	int i;

	if(fromnbr != 1) fromp = findline(fromnbr, (fromnbr == tonbr) ? true : false);	// set our pointer to the start line
	ListCnt = 1;

	while (1) {

		if(*fromp == T_LINENBR) {
			i = (((fromp[1]) << 8) | (fromp[2]));					// get the line number
			if(i != NOLINENBR && i > tonbr) break;					// end of the listing
			fromp = llist(b, fromp);								// otherwise expand the line
			MMfputs((unsigned char*)CtoM(b), fnbr);									// convert to a MMBasic string and output
//            #if defined(DOS)
//            	MMfputs((unsigned char *)"\1\n", fnbr);								// print the terminating lf
//            #else
			MMfputs((unsigned char*)"\2\r\n", fnbr);							// print the terminating cr/lf
//            #endif
			if(i != NOLINENBR && i >= tonbr) break;					// end of the listing
			// check if it is more than a screenfull
			if(fnbr == 0 && ListCnt >= VCHARS && !(fromp[0] == 0 && fromp[1] == 0)) {
				MMPrintString((char*)"PRESS ANY KEY ...");
				MMgetchar();
				MMPrintString((char*)"\r                 \r");
				ListCnt = 1;
			}
		}
		//else
		//	error((char *)"Internal error in flist()");

		// finally, is it the end of the program?
		if(fromp[0] == 0) break;
	}
	if(fnbr != 0) FileClose(fnbr);
}



