/***********************************************************************************************************************
MMBasic for Windows

Serial.cpp

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
int comn[MAXCOMPORTS] = { 0 };														// true if COM1 is enabled
int com_buf_size[MAXCOMPORTS] = { 0 };													// size of the buffer used to receive chars
int com_baud[MAXCOMPORTS] = { 0 };												// determines the baud rate
char* com_interrupt[MAXCOMPORTS] = { 0 };											// pointer to the interrupt routine
int com_ilevel[MAXCOMPORTS] = { 0 };													// number nbr of chars in the buffer for an interrupt
unsigned char* comRx_buf[MAXCOMPORTS] = { 0 };											// pointer to the buffer for received characters
volatile int comRx_head[MAXCOMPORTS] = { 0 };
volatile int comRx_tail[MAXCOMPORTS] = { 0 };								// head and tail of the ring buffer for com1
volatile int comcomplete[MAXCOMPORTS] = { 0 };
int comused = 0;
HANDLE hComm[MAXCOMPORTS] = { 0 };
// used to track the 9th bit
extern "C" int TestPort(int portno)
{
    int found = 0;
    HKEY hKey;
    LSTATUS res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey);
    LPSTR    achKey = (LPSTR)GetTempMemory(STRINGSIZE);   // buffer for subkey name
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

    if (cValues > 0 && cValues <= 128 && retCode == ERROR_SUCCESS)
    {
        char** c = (char**)GetTempMemory((cValues) * sizeof(*c) + (cValues) * (cchMaxValue + 1));
        char* b = (char*)GetTempMemory(cbMaxValueData + 1);
        LPSTR  achValue = (LPSTR)GetTempMemory(cchMaxValue + 1);
        DWORD fred = cbMaxValueData + 1;
        for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
        {
            cchValue = 255;
            achValue[0] = '\0';
            memset(b, 0, cbMaxValueData + 1);
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
                c[i] = (char*)((int)c + sizeof(char*) * (cValues)+i * (cchMaxValue + 1));
                strcpy(c[i], b);
            }
        }
        for (int j = 0; j < (int)cValues; j++) {
            char** x = NULL;
            int a = strtol((const char*)&c[j][3], x, 10);
            if (a == portno)found = 1;
        }
        RegCloseKey((HKEY)hKey);
    }
    return found;
}

extern "C" int SerialOpen(char* spec) {
    int baud, i, inv, oc, s2, de, parity, bits, bufsize, ilevel;
    char* interrupt;
    getargs((unsigned char **)&spec, 21, (unsigned char*)":,");
    char** x = NULL;
    int port = strtol((const char*)&argv[0][3], x, 10);
    if(port==0)error((char*)"COM specification");
    if (comn[port])error((char*)"Already open");
    if (!TestPort(port))error((char*)"Port not found");
    if (argc != 2 && (argc & 0x01) == 0) error((char*)"COM specification");

    bits = 8;
    de = parity = inv = oc = s2 = 0;
    for (i = 0; i < 3; i++) {
        if (str_equal(argv[argc - 1], (const unsigned char *)"EVEN")) {
            if (parity)error((char*)"Syntax");
            else { parity = 2; argc -= 2; }	// set even parity
        }
        if (str_equal(argv[argc - 1], (const unsigned char*)"ODD")) {
            if (parity)error((char*)"Syntax");
            else { parity = 1; argc -= 2; }	// set even parity
        }
        if (str_equal(argv[argc - 1], (const unsigned char*)"MARK")) {
            if (parity)error((char*)"Syntax");
            else { parity = 3; argc -= 2; }	// set even parity
        }
        if (str_equal(argv[argc - 1], (const unsigned char*)"SPACE")) {
            if (parity)error((char*)"Syntax");
            else { parity = 4; argc -= 2; }	// set even parity
        }
        if (str_equal(argv[argc - 1], (const unsigned char*)"S2")) {
            if (s2)error((char*)"Syntax");
            s2 = 2; argc -= 2; }	// get the two stop bit option
        if (str_equal(argv[argc - 1], (const unsigned char*)"S1P5")) {
            if (s2)error((char*)"Syntax");
            s2 = 1; argc -= 2;
        }	// get the two stop bit option
        if (str_equal(argv[argc - 1], (const unsigned char*)"7BIT")) { bits = 7; argc -= 2; }	// set the 7 bit byte option
    }
    if (argc >= 3 && *argv[2]) {
        baud = (int)getint(argv[2],110,4000000);									// get the baud rate as a number
    } else baud = COM_DEFAULT_BAUD_RATE;
    if (argc >= 5 && *argv[4])
        bufsize = (int)getint(argv[4],256, 4000000);								// get the buffer size as a number
    else
        bufsize = COM_DEFAULT_BUF_SIZE;

    if (argc >= 7) {
        InterruptUsed = true;
        argv[6] = (unsigned char *)strupr((char *)argv[6]);
        interrupt = (char *)GetIntAddress(argv[6]);							// get the interrupt location
    }
    else
        interrupt = NULL;

    if (argc >= 9) {
        ilevel = (int)getint(argv[8],1, 4000000);								// get the buffer level for interrupt as a number
        if (ilevel < 1 || ilevel > bufsize) error((char *)"COM specification");
    }
    else
        ilevel = 1;


    LPDCB  lpDCB=NULL;
    lpDCB = (LPDCB)GetTempMemory(sizeof LPDCB);
    char comport[20] = "\\\\.\\";
    strcat(comport, (const char *)argv[0]);
    hComm[port]= CreateFileA(comport,                //port name
        GENERIC_READ | GENERIC_WRITE, //Read/Write
        0,                            // No Sharing
        NULL,                         // No Security
        OPEN_EXISTING,// Open existing port only
        0,            // Non Overlapped I/O
        NULL);        // Null for Comm Devices

    if (hComm[port] == INVALID_HANDLE_VALUE)
        error((char *)"Error in opening serial port");
    GetCommState(hComm[port], lpDCB);
    lpDCB->BaudRate = baud;
    lpDCB->ByteSize = bits;
    lpDCB->StopBits = s2;
    lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
    if (parity) {
        lpDCB->fParity = true;
        lpDCB->Parity = parity;
    }
    COMMTIMEOUTS timeouts = { 0, //interval timeout. 0 = not used
                              0, // read multiplier
                             1, // read constant (milliseconds)
                              0, // Write multiplier
                              0  // Write Constant
    };
    SetCommTimeouts(hComm[port], &timeouts);
    SetCommState(hComm[port], lpDCB);
/*    GetCommState(hComm[port], lpDCB);
    PInt(lpDCB->BaudRate);PRet();
    PInt(lpDCB->ByteSize); PRet();
    PInt(lpDCB->StopBits); PRet();
    PInt(lpDCB->fParity); PRet();
    PInt(lpDCB->Parity); PRet();*/
    com_buf_size[port] = bufsize;
    com_interrupt[port] = interrupt;
    com_ilevel[port] = ilevel;
    comRx_buf[port] = (unsigned char *)GetMemory(bufsize);						// setup the buffer
    comRx_head[port] = comRx_tail[port] = 0;
    comn[port] = 1;
    comused = 1;
    return port;
}
extern "C" void SerialClose(int comnbr) {
    comn[comnbr] = false;
    com_interrupt[comnbr] = NULL;
    FreeMemorySafe((void**)&comRx_buf[comnbr]);
    CloseHandle(hComm[comnbr]);
    int testcom = 0;
    for (int i = 0; i < MAXCOMPORTS; i++) {
        if (comn[i])testcom = 1;
    }
    if (!testcom)comused = 0;
//    memset(hComm[comnbr], 0, sizeof(HANDLE));
}
extern "C" void SerialCloseAll(void) {
    for (int i = 1; i < MAXCOMPORTS; i++) {
        com_interrupt[i] = NULL;
        if (comn[i]){
            FreeMemory(comRx_buf[i]);
            comRx_buf[i] = NULL;
            CloseHandle(hComm[i]);
        }
        comn[i] = 0;
        int testcom = 0;
    }
    comused = 0;
}



/***************************************************************************************************
Add a character to the serial output buffer.
****************************************************************************************************/
extern "C" unsigned char SerialPutchar(int comnbr, unsigned char c) {
   DWORD dNoOfBytesWritten = 0;
//    MMputchar(c);
        WriteFile(hComm[comnbr],        // Handle to the Serial port
        (char *)&c,     // Data to be written to the port
        1,  //No of bytes to write
        &dNoOfBytesWritten, //Bytes written
        NULL);
    return c;
}


/***************************************************************************************************
Get the status the serial receive buffer.
Returns the number of characters waiting in the buffer
****************************************************************************************************/
extern "C" int SerialRxStatus(int comnbr) {
	int i = 0;
    i = comRx_head[comnbr] - comRx_tail[comnbr];
    if (i < 0) i += com_buf_size[comnbr];
    return i;
}
/***************************************************************************************************
Get the status the serial transmit buffer.
Returns the number of characters waiting in the buffer
****************************************************************************************************/
extern "C" int SerialTxStatus(int comnbr) {
	int i = 0;
	return i;
}
/***************************************************************************************************
Get a character from the serial receive buffer.
Note that this is returned as an integer and -1 means that there are no characters available
****************************************************************************************************/
extern "C" int SerialGetchar(int comnbr) {
	int c;
   	c = -1;                                                         // -1 is no data
    if (comRx_head[comnbr] != comRx_tail[comnbr]) {                            // if the queue has something in it
        c = comRx_buf[comnbr][comRx_tail[comnbr]];                            // get the char
        comRx_tail[comnbr] = (comRx_tail[comnbr] + 1) % com_buf_size[comnbr];        // and remove from the buffer
    }
    return c;
 }

extern "C" void SendBreak(int fnbr) {
    int port = FileTable[fnbr].com;
    LPDCB  lpDCB = NULL;
    lpDCB = (LPDCB)GetTempMemory(sizeof LPDCB);
    GetCommState(hComm[port], lpDCB);
    int baud = lpDCB->BaudRate;
    int breaklength = 1000000 / baud * 20;
    SetCommBreak(hComm[port]);
    uSec(breaklength);
    ClearCommBreak(hComm[port]);
}
