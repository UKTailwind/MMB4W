/***********************************************************************************************************************
MMBasic for Windows

Functions.cpp

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
#include "stdlib.h"

extern long long int  llabs(long long int  n);


/********************************************************************************************************************************************
 basic functions
 each function is responsible for decoding a basic function
 all function names are in the form fun_xxxx() so, if you want to search for the function responsible for the ASC() function look for fun_asc

 There are 4 globals used by these functions:

 unsigned char *ep       This is a pointer to the argument of the function
                Eg, in the case of INT(35/7) ep would point to "35/7)"

 fret           Is the return value for a basic function that returns a float

 iret           Is the return value for a basic function that returns an integer

 sret           Is the return value for a basic function that returns a string

 tret           Is the type of the return value.  normally this is set by the caller and is not changed by the function

 ********************************************************************************************************************************************/
 /*

 DOCUMENTATION
 =============
  FIELD$( str$, field)
  or
  FIELD$( str$, field, delim$)
  or
  FIELD$( str$, field, delim$, quote$)

  Extract a substring (ie, field) from 'str$'.  Each is separated by any one of
  the characters in the string 'delim$' and the field number to return is specified
  by 'field' (the first field is field number 1).  Any leading and trailing
  spaces will be trimmed from the returned string.

  Note that 'delim$' can contain a number of characters and the fields
  will then be separated by any one of these characters.  if delim$ is not
  provided it will default to a comma (,) character.

  'quote$' is the set of characters that might be used to quote text.  Typically
  it is the double quote character (") and any text that is surrounded by the quote
  character(s) will be treated as a block and any 'delim$' characters within that
  block will not be used as delimiters.

  This function is useful for splitting apart comma-separated-values (CSV) in data
  streams produced by GPS modules and test equipment.  For example:
   PRINT FIELD$("aaa,bbb,ccc", 2,(unsigned char *)",")
   Will print the string: bbb

   PRINT FIELD$("text1, 'quoted, text', text3", 2, ",", "'")
   will print the string: 'quoted, text'

  */

// return true if the char 'c' is contained in the string 'srch$'
// used only by scan_for_delimiter()  below
// Note: this operates on MMBasic strings
static int MInStr(char* srch, char c) {
    int i;
    for (i = 1; i <= *(unsigned char*)srch; i++)
        if(c == srch[i])
            return true;
    return false;
}

void fun_bound(void) {
    int which = 1;
    getargs((unsigned char **)&ep, 3,(unsigned char *)(unsigned char *)",");
    if(argc == 3)which = (int)getint(argv[2], 0, MAXDIM);
    findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
    if(which == 0)iret = OptionBase;
    else iret = vartbl[VarIndex].dims[which - 1];
    if(iret == -1)iret = 0;
    targ = T_INT;
}

// scan through string p and return p if it points to any char in delims
// this will skip any quoted text (quote delimiters in quotes)
// used only by fun_field() below
// Note: this operates on MMBasic strings
static int scan_for_delimiter(int start, unsigned char* p, unsigned char* delims, unsigned char* quotes) {
    int i;
    unsigned char qidx;
    for (i = start; i <= *(unsigned char*)p && !MInStr((char *)delims, p[i]); i++) {
        if(MInStr((char*)quotes, p[i])) {                                  // if we have a quote
            qidx = p[i];
            i++;                                                    // step over the opening quote
            while (i < *(unsigned char*)p && p[i] != qidx) i++;    // skip the quoted text
        }
    }
    return i;
}
void fun_call(void) {
    int i;
    long long int i64 = 0;
    unsigned char* s = NULL;
    MMFLOAT f;
    unsigned char* q;
    unsigned char* p = (unsigned char*)getCstring(ep); //get the command we want to call
    q = p;
    while (*q) { //convert to upper case for the match
        *q = toupper(*q);
        q++;
    }
    q = ep;
    while (*q) {
        if(*q == ',' || *q == '\'')break;
        q++;
    }
    if(*q == ',')q++;
    i = FindSubFun(p, false);                   // it could be a defined command
    strcat((char *)p, " ");
    strcat((char *)p, (char *) q);
    targ = T_NOTYPE;
    if(i >= 0) {                                // >= 0 means it is a user defined command
        DefinedSubFun(true, p, i, &f, &i64, &s, &targ);
    }
    else error((char *)"Unknown user function");
    if(targ & T_STR) {
        sret = (unsigned char *)GetTempMemory(STRINGSIZE);
        Mstrcpy(sret, s);                                             // if it is a string then save it
    }
    if(targ & T_INT)iret = i64;
    if(targ & T_NBR)fret = f;
}

// syntax:  str$ = FIELD$(string1, nbr, string2, string3)
//          find field nbr in string1 using the delimiters in string2 to separate the fields
//          if string3 is present any chars quoted by chars in string3 will not be searched for delimiters
// Note: this operates on MMBasic strings
void fun_field(void) {
    unsigned char* p, * delims = (unsigned char*)"\1,", * quotes = (unsigned char*)"\0";
    int fnbr, i, j, k;
    getargs((unsigned char **)&ep, 7,(unsigned char *)(unsigned char *)",");
    if(!(argc == 3 || argc == 5 || argc == 7)) error((char *)"Syntax");
    p = getstring(argv[0]);                                         // the string containing the fields
    fnbr = (int)getint(argv[2], 1, MAXSTRLEN);                           // field nbr to return
    if(argc > 3 && *argv[4]) delims = getstring(argv[4]);           // delimiters for fields
    if(argc == 7) quotes = getstring(argv[6]);                      // delimiters for quoted text
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                      // this will last for the life of the command
    targ = T_STR;
    i = 1;
    while (--fnbr > 0) {
        i = scan_for_delimiter(i, p, delims, quotes);
        if(i > *p) return;
        i++;                                                        // step over the delimiter
    }
    while (p[i] == ' ') i++;                                         // trim leading spaces
    j = scan_for_delimiter(i, p, delims, quotes);                   // find the end of the field
    *sret = k = j - i;
    for (j = 1; j <= k; j++, i++) sret[j] = p[i];                    // copy to the return string
    for (k = *sret; k > 0 && sret[k] == ' '; k--);                   // trim trailing spaces
    *sret = k;
}

void fun_str2bin(void) {
    union binmap {
        int8_t c[8];
        uint8_t uc[8];
        float f;
        double d;
        int64_t l;
        uint64_t ul;
        int i;
        uint32_t ui;
        short s;
        uint16_t us;
    }map;
    int j;
    getargs((unsigned char **)&ep, 5,(unsigned char *)(unsigned char *)",");
    if(!(argc == 3 || argc == 5))error((char *)"Syntax");
    if(argc == 5 && !checkstring(argv[4], (unsigned char*)"BIG"))error((char *)"Syntax");
    char* p;
    p = (char *)getstring(argv[2]);
    int len = p[0];
    map.l = 0;
    for (j = 0; j < len; j++)map.c[j] = p[j + 1];
    if(argc == 5) { // big endian so swap byte order
        char k;
        int m;
        for (j = 0; j < (len >> 1); j++) {
            m = len - j - 1;
            k = map.c[j];
            map.c[j] = map.c[m];
            map.c[m] = k;
        }
    }
    if(checkstring(argv[0], (unsigned char*)"DOUBLE")) {
        if(len != 8)error((char *)"String length");
        targ = T_NBR;
        fret = (MMFLOAT)map.d;
    }
    else if(checkstring(argv[0], (unsigned char*)"SINGLE")) {
        if(len != 4)error((char *)"String length");
        targ = T_NBR;
        fret = (MMFLOAT)map.f;
    }
    else if(checkstring(argv[0], (unsigned char*)"INT64")) {
        if(len != 8)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.l;
    }
    else if(checkstring(argv[0], (unsigned char*)"INT32")) {
        if(len != 4)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.i;
    }
    else if(checkstring(argv[0], (unsigned char*)"INT16")) {
        if(len != 2)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.s;
    }
    else if(checkstring(argv[0], (unsigned char*)"INT8")) {
        if(len != 1)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.c[0];
    }
    else if(checkstring(argv[0], (unsigned char*)"UINT64")) {
        if(len != 8)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.ul;
    }
    else if(checkstring(argv[0], (unsigned char*)"UINT32")) {
        if(len != 4)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.ui;
    }
    else if(checkstring(argv[0], (unsigned char*)"UINT16")) {
        if(len != 2)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.us;
    }
    else if(checkstring(argv[0], (unsigned char*)"UINT8")) {
        if(len != 1)error((char *)"String length");
        targ = T_INT;
        iret = (int64_t)map.uc[0];
    }
    else error((char *)"Syntax");

}


void fun_bin2str(void) {
    int j, len = 0;
    union binmap {
        int8_t c[8];
        uint8_t uc[8];
        float f;
        double d;
        int64_t l;
        uint64_t ul;
        int i;
        uint32_t ui;
        short s;
        uint16_t us;
    }map;
    int64_t i64;
    getargs((unsigned char **)&ep, 5,(unsigned char *)(unsigned char *)",");
    if(!(argc == 3 || argc == 5))error((char *)"Syntax");
    if(argc == 5 && !(checkstring(argv[4], (unsigned char*)"BIG")))error((char *)"Syntax");
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this will last for the life of the command
    if(checkstring(argv[0], (unsigned char*)"DOUBLE")) {
        len = 8;
        map.d = (double)getnumber(argv[2]);
    }
    else if(checkstring(argv[0], (unsigned char*)"SINGLE")) {
        len = 4;
        map.f = (float)getnumber(argv[2]);
    }
    else {
        i64 = getinteger(argv[2]);
        if(checkstring(argv[0], (unsigned char*)"INT64")) {
            len = 8;
            map.l = (int64_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"INT32")) {
            len = 4;
            if (i64 > 2147483647LL || i64 < -2147483648LL) error((char *)"Overflow");
            map.i = (int32_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"INT16")) {
            len = 2;
            if (i64 > 32767LL || i64 < -32768LL) error((char *)"Overflow");
            map.s = (int16_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"INT8")) {
            len = 1;
            if (i64 > 127LL || i64 < -128LL) error((char *)"Overflow");
            map.c[0] = (int8_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"UINT64")) {
            len = 8;
            map.ul = (uint64_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"UINT32")) {
            len = 4;
            if (i64 > 4294967295LL || i64 < 0LL) error((char *)"Overflow");
            map.ui = (uint32_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"UINT16")) {
            len = 2;
            if (i64 > 65535LL || i64 < 0LL) error((char *)"Overflow");
            map.us = (uint16_t)i64;
        }
        else if(checkstring(argv[0], (unsigned char*)"UINT8")) {
            len = 1;
            if (i64 > 255LL || i64 < 0LL) error((char *)"Overflow");
            map.uc[0] = (uint8_t)i64;
        }
        else error((char *)"Syntax");
    }


    for (j = 0; j < len; j++)sret[j] = map.c[j];

    if(argc == 5) { // big endian so swap byte order
        unsigned char k;
        int m;
        for (j = 0; j < (len >> 1); j++) {
            m = len - j - 1;
            k = sret[j];
            sret[j] = sret[m];
            sret[m] = k;
        }
    }
    // convert from c type string but it can contain zeroes
    unsigned char* p1, * p2;
    j = len;
    p1 = sret + len; p2 = sret + len - 1;
    while (j--) *p1-- = *p2--;
    *sret = len;
    targ = T_STR;
}



// return the absolute value of a number (ie, without the sign)
// a = ABS(nbr)
void fun_abs(void) {
    unsigned char* s;
    MMFLOAT f;
    long long int  i64;

    targ = T_INT;
    evaluate(ep, &f, &i64, &s, &targ, false);                   // get the value and type of the argument
    if(targ & T_NBR)
        fret = fabs(f);
    else {
        iret = i64;
        if(iret < 0) iret = -iret;
    }
}




// return the ASCII value of the first character in a string (ie, its number value)
// a = ASC(str$)
void fun_asc(void) {
    unsigned char* s;

    s = getstring(ep);
    if(*s == 0)
        iret = 0;
    else
        iret = *(s + 1);
    targ = T_INT;
}



// return the arctangent of a number in radians
void fun_atn(void) {
    fret = atan(getnumber(ep)) * optionangle;
    targ = T_NBR;
}

void fun_atan2(void) {
    MMFLOAT y, x, z;
    getargs((unsigned char **)&ep, 3, (unsigned char*)(unsigned char *)",");
    if(argc != 3)error((char *)"Syntax");
    y = getnumber(argv[0]);
    x = getnumber(argv[2]);
    z = atan2(y, x);
    fret = z * optionangle;
    targ = T_NBR;
}

// convert a number into a one character string
// s$ = CHR$(nbr)
void fun_chr(void) {
    int i;

    i = (int)getint(ep, 0, 0xff);
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this will last for the life of the command
    sret[0] = 1;
    sret[1] = i;
    targ = T_STR;
}



// Round numbers with fractional portions up or down to the next whole number or integer.
void fun_cint(void) {
    iret = getinteger(ep);
    targ = T_INT;
}



// return the cosine of a number in radians
void fun_cos(void) {
    fret = cos(getnumber(ep) / optionangle);
    targ = T_NBR;
}


// convert radians to degrees.  Thanks to Alan Williams for the contribution
void fun_deg(void) {
    fret = (MMFLOAT)((MMFLOAT)getnumber(ep) * RADCONV);
    targ = T_NBR;
}



// Returns the exponential value of a number.
void fun_exp(void) {
    fret = exp(getnumber(ep));
    targ = T_NBR;
}

// utility function used by HEX$(), OCT$() and BIN$()
void DoHexOctBin(int base) {
    unsigned long long int  i;
    int j = 1;
    getargs((unsigned char **)&ep, 3, (unsigned char*)(unsigned char *)",");
    i = (unsigned long long int)getinteger(argv[0]);                // get the number
    if(argc == 3) j = (int)getint(argv[2], 1, MAXSTRLEN);                // get the optional number of chars to return
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                    // this will last for the life of the command
    IntToStrPad((char *)sret, (signed long long int)i, '0', j, base);
    CtoM(sret);
    targ = T_STR;
}

// utility function used by HEX$(), OCT$() and BIN$()
//void DoHexOctBin(char *tp,int base) {
void fun_base(void) {
    unsigned long long int i;
    int j = 1;
    //	getargs(&tp, 3, ",");
    getargs(&ep, 5, (unsigned char*)",");
    int base = (int)getint(argv[0], 2, 36);
    i = (unsigned long long int)getinteger(argv[2]);                // get the number
    if (argc == 5) j = (int)getint(argv[4], 0, MAXSTRLEN);                // get the optional number of chars to return
    sret = (unsigned char*)GetTempMemory(STRINGSIZE);                                      // this will last for the life of the command
    IntToStrPad((char*)sret, (signed long long int)i, '0', j, base);
    CtoM(sret);
    targ = T_STR;
}


// return the hexadecimal representation of a number
// s$ = HEX$(nbr)
//void fun_hex(void) {
//    DoHexOctBin(16);
//}



// return the octal representation of a number
// s$ = OCT$(nbr)
//void fun_oct(void) {
//    DoHexOctBin(8);
//}



// return the binary representation of a number
// s$ = BIN$(nbr)
//void fun_bin(void) {
//    DoHexOctBin(2);
//}



// syntax:  nbr = INSTR([start,] string1, string2)
//          find the position of string2 in string1 starting at start chars in string1
// returns an integer
void fun_instr(void) {
    unsigned char* s1 = NULL, * s2 = NULL;
    int start = 0;
    getargs((unsigned char **)&ep, 5, (unsigned char*)(unsigned char *)",");

    if(argc == 5) {
        start = (int)getint(argv[0], 1, MAXSTRLEN + 1) - 1;
        s1 = getstring(argv[2]);
        s2 = getstring(argv[4]);
    }
    else if(argc == 3) {
        start = 0;
        s1 = getstring(argv[0]);
        s2 = getstring(argv[2]);
    }
    else
        error((char *)"Argument count");

    targ = T_INT;
    if(start > *s1 - *s2 + 1 || *s2 == 0)
        iret = 0;
    else {
        // find s2 in s1 using MMBasic strings
        int i;
        for (i = start; i < *s1 - *s2 + 1; i++) {
            if(memcmp(s1 + i + 1, s2 + 1, *s2) == 0) {
                iret = i + 1;
                return;
            }
        }
    }
    iret = 0;
}





// Truncate an expression to the next whole number less than or equal to the argument. 
void fun_int(void) {
    iret = (long long int)floor(getnumber(ep));
    targ = T_INT;
}


// Truncate a number to a whole number by eliminating the decimal point and all characters 
// to the right of the decimal point.
void fun_fix(void) {
    iret = (long long int)getnumber(ep);
    targ = T_INT;
}



// Return a substring offset by a number of characters from the left (beginning) of the string.
// s$ = LEFT$( string$, nbr )
void fun_left(void) {
    int i;
    unsigned char* s;
    getargs((unsigned char **)&ep, 3, (unsigned char*)(unsigned char *)",");

    if(argc != 3) error((char *)"Argument count");
    s = (unsigned char*)GetTempMemory(STRINGSIZE);                                       // this will last for the life of the command
    Mstrcpy(s, getstring(argv[0]));
    i = (int)getint(argv[2], 0, MAXSTRLEN);
    if(i < *s) *s = i;                                              // truncate if it is less than the current string length
    sret = s;
    targ = T_STR;
}



// Return a substring of ?string$? with ?number-of-chars? from the right (end) of the string.
// s$ = RIGHT$( string$, number-of-chars )
void fun_right(void) {
    int nbr;
    unsigned char* s, * p1, * p2;
    getargs((unsigned char **)&ep, 3, (unsigned char*)(unsigned char *)",");

    if(argc != 3) error((char *)"Argument count");
    s = getstring(argv[0]);
    nbr = (int)getint(argv[2], 0, MAXSTRLEN);
    if(nbr > *s) nbr = *s;											// get the number of chars to copy
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this will last for the life of the command
    p1 = sret; p2 = s + (*s - nbr) + 1;
    *p1++ = nbr;													// inset the length of the returned string
    while (nbr--) *p1++ = *p2++;										// and copy the characters
    targ = T_STR;
}



// return the length of a string
// nbr = LEN( string$ )
void fun_len(void) {
    iret = *(unsigned char*)getstring(ep);                         // first byte is the length
    targ = T_INT;
}



// Return the natural logarithm of the argument 'number'.
// n = LOG( number )
void fun_log(void) {
    MMFLOAT f;
    f = getnumber(ep);
    if(f == 0) error((char *)"Divide by zero");
    if(f < 0) error((char *)"Negative argument");
    fret = log(f);
    targ = T_NBR;
}



// Returns a substring of ?string$? beginning at ?start? and continuing for ?nbr? characters.
// S$ = MID$(s, spos [, nbr])
void fun_mid(void) {
    unsigned char* s, * p1, * p2;
    int spos, nbr = 0, i;
    getargs((unsigned char **)&ep, 5, (unsigned char*)(unsigned char *)",");

    if(argc == 5) {													// we have MID$(s, n, m)
        nbr = (int)getint(argv[4], 0, MAXSTRLEN);						// nbr of chars to return
    }
    else if(argc == 3) {											// we have MID$(s, n)
        nbr = MAXSTRLEN;											// default to all chars
    }
    else
        error((char *)"Argument count");

    s = getstring(argv[0]);											// the string
    spos = (int)getint(argv[2], 1, MAXSTRLEN);						    // the mid position

    sret = (unsigned char*)GetTempMemory(STRINGSIZE);									// this will last for the life of the command
    targ = T_STR;
    if(spos > *s || nbr == 0)										// if the numeric args are not in the string
        return;														// return a null string
    else {
        i = *s - spos + 1;											// find how many chars remaining in the string
        if(i > nbr) i = nbr;										// reduce it if we don't need that many
        p1 = sret; p2 = s + spos;
        *p1++ = i;													// set the length of the MMBasic string
        while (i--) *p1++ = *p2++;									// copy the nbr chars required
    }
}



// Return the value of Pi.  Thanks to Alan Williams for the contribution
// n = PI
void fun_pi(void) {
    fret = PI_VALUE;
    targ = T_NBR;
}



// convert degrees to radians.  Thanks to Alan Williams for the contribution
// r = RAD( degrees )
void fun_rad(void) {
    fret = (MMFLOAT)((MMFLOAT)getnumber(ep) / RADCONV);
    targ = T_NBR;
}


// generate a random number that is greater than or equal to 0 but less than 1
// n = RND()
void fun_rnd(void) {
    fret = (MMFLOAT)rand() / ((MMFLOAT)RAND_MAX + (MMFLOAT)RAND_MAX / 1000000);
    targ = T_NBR;
}



// Return the sign of the argument
// n = SGN( number )
void fun_sgn(void) {
    MMFLOAT f;
    f = getnumber(ep);
    if(f > 0)
        iret = +1;
    else if(f < 0)
        iret = -1;
    else
        iret = 0;
    targ = T_INT;
}



// Return the sine of the argument 'number' in radians.
// n = SIN( number )
void fun_sin(void) {
    fret = sin(getnumber(ep) / optionangle);
    targ = T_NBR;
}



// Return the square root of the argument 'number'.
// n = SQR( number )
void fun_sqr(void) {
    MMFLOAT f;
    f = getnumber(ep);
    if(f < 0) error((char *)"Negative argument");
    fret = sqrt(f);
    targ = T_NBR;
}



// Return the tangent of the argument 'number' in radians.
// n = TAN( number )
void fun_tan(void) {
    fret = tan(getnumber(ep) / optionangle);
    targ = T_NBR;
}



// Returns the numerical value of the ?string$?.
// n = VAL( string$ )
void fun_val(void) {
    unsigned char* p, * t1, * t2;
    p = getCstring(ep);
    targ = T_INT;
    if(*p == '&') {
        p++; iret = 0;
        switch (toupper(*p++)) {
        case 'H': while (isxdigit(*p)) {
            iret = (iret << 4) | ((toupper(*p) >= 'A') ? toupper(*p) - 'A' + 10 : *p - '0');
            p++;
        }
                break;
        case 'O': while (*p >= '0' && *p <= '7') {
            iret = (iret << 3) | (*p++ - '0');
        }
                break;
        case 'B': while (*p == '0' || *p == '1') {
            iret = (iret << 1) | (*p++ - '0');
        }
                break;
        default: iret = 0;
        }
    }
    else {
        fret = (MMFLOAT)strtod((char*)p, (char**)&t1);
        iret = strtoll((char*)p, (char**)&t2, 10);
        if(t1 > t2) targ = T_NBR;
    }
}

void fun_eval(void) {
    unsigned char* s, * st, * temp_tknbuf;
    temp_tknbuf = (unsigned char*)GetTempMemory(STRINGSIZE);
    strcpy((char*)temp_tknbuf, (char*)tknbuf);                                    // first save the current token buffer in case we are in immediate mode
    // we have to fool the tokeniser into thinking that it is processing a program line entered at the console
    st = (unsigned char*)GetTempMemory(STRINGSIZE);
    strcpy((char*)st, (char*)getstring(ep));                                      // then copy the argument
    MtoC(st);                                                       // and convert to a C string
    inpbuf[0] = 'r'; inpbuf[1] = '=';                               // place a dummy assignment in the input buffer to keep the tokeniser happy
    strcpy((char*)inpbuf + 2, (char*)st);
    tokenise(true);                                                 // and tokenise it (the result is in tknbuf)
    strcpy((char*)st, (char*)(tknbuf + 3));
    targ = T_NOTYPE;
    evaluate(st, &fret, &iret, &s, &targ, false);                   // get the value and type of the argument
    if(targ & T_STR) {
        Mstrcpy(st, s);                                             // if it is a string then save it
        sret = st;
    }
    strcpy((char*)tknbuf, (char*)temp_tknbuf);                                    // restore the saved token buffer
}


void fun_errno(void) {
    iret = MMerrno;
    targ = T_INT;
}


void fun_errmsg(void) {
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);
    strcpy((char *)sret, MMErrMsg);
    CtoM(sret);
    targ = T_STR;
}



// Returns a string of blank spaces 'number' bytes long.
// s$ = SPACE$( number )
void fun_space(void) {
    int i;

    i = (int)getint(ep, 0, MAXSTRLEN);
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this will last for the life of the command
    memset(sret + 1, ' ', i);
    *sret = i;
    targ = T_STR;
}



// Returns a string in the decimal (base 10) representation of  'number'.
// s$ = STR$( number, m, n, c$ )
void fun_str(void) {
    unsigned char* s;
    MMFLOAT f;
    long long int i64;
    int t;
    int m, n;
    unsigned char ch, * p;

    getargs((unsigned char**)&ep, 7, (unsigned char*)(unsigned char *)",");
    if((argc & 1) != 1) error((char *)"Syntax");
    t = T_NOTYPE;
    p = evaluate(argv[0], &f, &i64, &s, &t, false);                 // get the value and type of the argument
    if(!(t & T_INT || t & T_NBR)) error((char*)"Expected a number");
    m = 0; n = STR_AUTO_PRECISION; ch = ' ';
    if(argc > 2) m = (int)getint(argv[2], -128, 128);                    // get the number of digits before the point
    if(argc > 4) n = (int)getint(argv[4], -20, 20);                      // get the number of digits after the point
    if(argc == 7) {
        p = getstring(argv[6]);
        if(*p == 0) error((char*)"Zero length argument");
        ch = ((unsigned char)p[1] & 0x7f);
    }

    sret = (unsigned char*)GetTempMemory(STRINGSIZE);									    // this will last for the life of the command
    if(t & T_NBR)
        FloatToStr((char*)sret, f, m, n, ch);                              // convert the float
    else {
        if(n < 0)
            FloatToStr((char*)sret, (MMFLOAT)i64, m, n, ch);                        // convert as a float
        else {
            IntToStrPad((char *)sret, i64, ch, m, 10);                      // convert the integer
            if(n != STR_AUTO_PRECISION && n > 0) {
                strcat((char *)sret, ".");
                while (n--) strcat((char*)sret, "0");                       // and add on any zeros after the point
            }
        }
    }
    CtoM(sret);
    targ = T_STR;
}



// Returns a string 'nbr' bytes long
// s$ = STRING$( nbr,  string$ )
// s$ = STRING$( nbr,  number )
void fun_string(void) {
    int i, j, t = T_NOTYPE;
    void* p;

    getargs((unsigned char**)&ep, 3,(unsigned char *)(unsigned char *)",");
    if(argc != 3) error((char *)"Syntax");

    i = (int)getint(argv[0], 0, MAXSTRLEN);
    p = DoExpression(argv[2], &t);                                  // get the value and type of the argument
    if(t & T_STR) {
        if(!*(char*)p) error((char *)"Argument value: $", argv[2]);
        j = *((char*)p + 1);
    }
    else if(t & T_INT)
        j = (int)( * (long long int*)p);
    else
        j = FloatToInt32(*((MMFLOAT*)p));
    if(j < 0 || j > 255) error((char *)"Argument value: $", argv[2]);

    sret = (unsigned char *)GetTempMemory(STRINGSIZE);                                      // this will last for the life of the command
    memset(sret + 1, j, i);
    *sret = i;
    targ = T_STR;
}



// Returns string$ converted to uppercase characters.
// s$ = UCASE$( string$ )
void fun_ucase(void) {
    unsigned char* s, * p;
    int i;

    s = getstring(ep);
    p = sret = (unsigned char *)GetTempMemory(STRINGSIZE);								// this will last for the life of the command
    i = *p++ = *s++;												// get the length of the string and save in the destination
    while (i--) {
        *p = toupper(*s);
        p++; s++;
    }
    targ = T_STR;
}



// Returns string$ converted to lowercase characters.
// s$ = LCASE$( string$ )
void fun_lcase(void) {
    unsigned char* s, * p;
    int i;

    s = getstring(ep);
    p = sret = (unsigned char *)GetTempMemory(STRINGSIZE);								// this will last for the life of the command
    i = *p++ = *s++;												// get the length of the string and save in the destination
    while (i--) {
        *p = tolower(*s);
        p++; s++;
    }
    targ = T_STR;
}


// function (which looks like a pre defined variable) to return the version number
// it pulls apart the VERSION string to generate the number
void fun_version(void) {
    char* p;
    fret = strtol(VERSION, &p, 10);
    fret += (MMFLOAT)strtol(p + 1, &p, 10) / 100;
    fret += (MMFLOAT)strtol(p + 1, &p, 10) / 10000;
    fret += (MMFLOAT)strtol(p + 1, &p, 10) / 1000000;
    targ = T_NBR;
}



// Returns the current cursor position in the line in characters.
// n = POS
void fun_pos(void) {
    iret = MMCharPos;
    targ = T_INT;
}



// Outputs spaces until the column indicated by 'number' has been reached.
// PRINT TAB( number )
void fun_tab(void) {
    int i;
    unsigned char* p;

    i = (int)getint(ep, 1, 255);
    sret = p = (unsigned char*)GetTempMemory(STRINGSIZE);							    // this will last for the life of the command
    if(MMCharPos > i) {
        i--;
        *p++ = '\r';
        *p++ = '\n';
    }
    else
        i -= MMCharPos;
    memset(p, ' ', i);
    p[i] = 0;
    CtoM(sret);
    targ = T_STR;
}



// get a character from the console input queue
// s$ = INKEY$
void fun_inkey(void) {
    int i;

    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this buffer is automatically zeroed so the string is zero size

    i = getConsole(1);
    if(i != -1) {
        sret[0] = 1;												// this is the length
        sret[1] = i;												// and this is the character
    }
    targ = T_STR;
}



// used by ACos() and ASin() below
MMFLOAT arcsinus(MMFLOAT x) {
    return 2.0L * atan(x / (1.0L + sqrt(1.0L - x * x)));
}


// Return the arcsine (in radians) of the argument 'number'.
// n = ASIN(number)
void fun_asin(void) {
    MMFLOAT f = getnumber(ep);
    if(f < -1.0 || f > 1.0) error((char *)"Number out of bounds");
    if(f == 1.0) {
        fret = PI_VALUE / 2;
    }
    else if(f == -1.0) {
        fret = -PI_VALUE / 2;
    }
    else {
        fret = arcsinus(f);
    }
    fret *= optionangle;
    targ = T_NBR;
}


// Return the arccosine (in radians) of the argument 'number'.
// n = ACOS(number)
void fun_acos(void) {
    MMFLOAT f = getnumber(ep);
    if(f < -1.0L || f > 1.0L) error((char *)"Number out of bounds");
    if(f == 1.0L) {
        fret = 0.0L;
    }
    else if(f == -1.0L) {
        fret = PI_VALUE;
    }
    else {
        fret = PI_VALUE / 2 - arcsinus(f);
    }
    fret *= optionangle;
    targ = T_NBR;
}


// utility function to do the max/min comparison and return the value
// it is only called by fun_max() and fun_min() below.
void do_max_min(int cmp) {
    int i;
    MMFLOAT nbr, f;
    getargs((unsigned char **)&ep, (MAX_ARG_COUNT * 2) - 1,(unsigned char *)(unsigned char *)",");
    if((argc & 1) != 1) error((char *)"Syntax");
    if(cmp) nbr = -FLT_MAX; else nbr = FLT_MAX;
    for (i = 0; i < argc; i += 2) {
        f = getnumber(argv[i]);
        if(cmp && f > nbr) nbr = f;
        if(!cmp && f < nbr) nbr = f;
    }
    fret = nbr;
    targ = T_NBR;
}


void fun_max(void) {
    do_max_min(1);
}


void fun_min(void) {
    do_max_min(0);
}
void fun_ternary(void) {
    MMFLOAT f = 0;
    long long int i64 = 0;
    unsigned char* s = NULL;
    int t = T_NOTYPE;
    getargs((unsigned char **)&ep, 5,(unsigned char *)(unsigned char *)",");
    if(argc != 5)error((char *)"Syntax");
    int which = (int)getnumber(argv[0]);
    if(which) {
        evaluate(argv[2], &f, &i64, &s, &t, false);
    }
    else {
        evaluate(argv[4], &f, &i64, &s, &t, false);
    }
    if(t & T_INT) {
        iret = i64;
        targ = T_INT;
        return;
    }
    else if(t & T_NBR) {
        fret = f;
        targ = T_NBR;
        return;
    }
    else if(t & T_STR) {
        sret = (unsigned char *)GetTempMemory(STRINGSIZE);
        Mstrcpy(sret, s);                                   // copy the string
        targ = T_STR;
        return;
    }
    else error((char *)"Syntax");
}
// A convenient way of evaluating an expression
// it takes two arguments:
//     p = pointer to the expression in memory (leading spaces will be skipped)
//     t = pointer to the type
//         if *t = T_STR or T_NBR or T_INT will throw an error if the result is not the correct type
//         if *t = T_NOTYPE it will not throw an error and will return the type found in *t
// it returns with a void pointer to a float, integer or string depending on the value returned in *t
// this will check that the expression is terminated correctly and throw an error if not
extern "C" void  * DoExpression(unsigned char* p, int* t) {
    static MMFLOAT f;
    static long long int  i64;
    static unsigned char* s;

    evaluate(p, &f, &i64, &s, t, false);
    if(*t & T_INT) return &i64;
    if(*t & T_NBR) return &f;
    if(*t & T_STR) return s;

    error((char *)"Internal fault (sorry)");
    return NULL;                                                    // to keep the compiler happy
}
// function (which looks like a pre defined variable) to return MM.CMDLINE$
// it uses the command line for a shortcut RUN (the + symbol) which was stored in tknbuf[]
void fun_cmdline(void) {
    char* q, * p = runcmd;
    sret = (unsigned char *)GetTempMemory(STRINGSIZE);									// this buffer is automatically zeroed so the string is zero size
    skipspace(p);
    if (*p == 34) {
        do {
            p++;
        } while (*p != 34);
        p++;
        skipspace(p);
        if (*p == ',') {
            p++;
            skipspace(p);
        }
    }
    if ((q = strchr(p, '|'))) {
        q--;
        *q = 0;
    }
    strcpy((char *)sret, p);                                   // copy the string
    CtoM(sret);
    targ = T_STR;
}

