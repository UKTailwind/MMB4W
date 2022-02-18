#define __BMPDECODER_C__
/******************************************************************************

* FileName:        BmpDecoder.c
* Dependencies:    Image decoding library; project requires File System library
* Processor:       PIC24/dsPIC30/dsPIC33/PIC32MX
* Compiler:        C30 v2.01/C32 v0.00.18
* Company:         Microchip Technology, Inc.

 * Software License Agreement
 *
 * Copyright ? 2008 Microchip Technology Inc.  All rights reserved.
 * Microchip licenses to you the right to use, modify, copy and distribute
 * Software only when embedded on a Microchip microcontroller or digital
 * signal controller, which is integrated into your product or third party
 * product (pursuant to the sublicense terms in the accompanying license
 * agreement).
 *
 * You should refer to the license agreement accompanying this Software
 * for additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED ?AS IS? WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR
 * OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,
 * BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL,
 * INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA,
 * COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY
 * CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF),
 * OR OTHER SIMILAR COSTS.

Author                 Date           Comments
--------------------------------------------------------------------------------
Pradeep Budagutta    03-Mar-2008    First release
*******************************************************************************/

#include "MainThread.h"
#include "MMBasic_Includes.h"
#include <stdio.h>

//** SD CARD INCLUDES ***********************************************************

extern uFileTable FileTable[MAXOPENFILES + 1];
#define  IMG_FILE   FileTable[fnbr].fptr
#define  IMG_FREAD  fread

#define  IMG_vSetboundaries()
#define  IMG_vLoopCallback()
#define  IMG_vCheckAndAbort()  CheckAbort()
union colourmap
{
    char rgbbytes[4];
    unsigned int rgb;
} colour;
int32_t FSerror;
#define  IMG_vSetColor(red, green, blue)    {colour.rgbbytes[0] = red; colour.rgbbytes[1] = green; colour.rgbbytes[2] = blue;colour.rgbbytes[3] = (char)0xFF;}
#define  IMG_vPutPixel(xx, yy)              DrawPixel(xx + x, yy + y, colour.rgb)

int IMG_wImageWidth, IMG_wImageHeight;
int bufpos;

void vPutPixel(int x, int y, int xx, int yy, char* p) {
    if (xx == 0)bufpos = 0;
    p[bufpos++] = colour.rgbbytes[2];
    p[bufpos++] = colour.rgbbytes[1];
    p[bufpos++] = colour.rgbbytes[0];
    if (xx == (IMG_wImageWidth - 1))DrawBuffer(xx + x, yy + y, xx + x + IMG_wImageWidth - 1, yy + y, (uint32_t *)p);
}

/*************************/
/**** DATA STRUCTURES ****/
/*************************/
typedef struct _BMPDECODER
{
    uint32_t lWidth;
    uint32_t lHeight;
    uint32_t lImageOffset;
    uint32_t wPaletteEntries;
    uint8_t bBitsPerPixel;
    uint8_t bHeaderType;
    uint8_t blBmMarkerFlag : 1;
    uint8_t blCompressionType : 3;
    uint8_t bNumOfPlanes : 3;
    uint8_t b16bit565flag : 1;
    uint8_t aPalette[256][3]; /* Each palette entry has RGB */
} BMPDECODER;

/**************************/
/******* FUNCTIONS  *******/
/**************************/

/*******************************************************************************
Function:       void BDEC_vResetData(BMPDECODER *pBmpDec)

Precondition:   None

Overview:       This function resets the variables so that new Bitmap image
                can be decoded

Input:          Bitmap decoder's data structure

Output:         None
*******************************************************************************/
void BDEC_vResetData(BMPDECODER* pBmpDec)
{
    pBmpDec->lWidth = 0;
    pBmpDec->lHeight = 0;
    pBmpDec->lImageOffset = 0;
    pBmpDec->wPaletteEntries = 0;
    pBmpDec->bBitsPerPixel = 0;
    pBmpDec->bHeaderType = 0;
    pBmpDec->blBmMarkerFlag = 0;
    pBmpDec->blCompressionType = 0;
    pBmpDec->bNumOfPlanes = 0;
    pBmpDec->b16bit565flag = 0;
}

/*******************************************************************************
Function:       uint8_t BDEC_bReadHeader(BMPDECODER *pBmpDec)

Precondition:   None

Overview:       This function reads the Bitmap file header and
                fills the data structure

Input:          Bitmap decoder's data structure

Output:         Error code - '0' means no error
*******************************************************************************/
uint8_t BDEC_bReadHeader(BMPDECODER* pBmpDec, int fnbr)
{
    uint8_t bByte1, bByte2;
    uint32_t wWord;
    uint32_t lLong;
    IMG_FREAD(&bByte1, 1, 1, IMG_FILE);  /* Marker */
    IMG_FREAD(&bByte2, 1, 1, IMG_FILE);  /* Marker */

    if (bByte1 == 'B' && bByte2 == 'M')
    {
        pBmpDec->blBmMarkerFlag = 1;
    }
    else
    {
        return(100);
    }

    IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* File length */
    IMG_FREAD(&wWord, 2, 1, IMG_FILE);  /* Reserved */
    IMG_FREAD(&wWord, 2, 1, IMG_FILE);  /* Reserved */

    IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Image offset */
    pBmpDec->lImageOffset = lLong;

    IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Header length */
    pBmpDec->bHeaderType = (uint8_t)lLong;

    if (pBmpDec->bHeaderType >= 40)
    {
        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Image Width */
        pBmpDec->lWidth = lLong;

        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Image Height */
        pBmpDec->lHeight = lLong;

        IMG_FREAD(&wWord, 2, 1, IMG_FILE);  /* Number of Planes */
        pBmpDec->bNumOfPlanes = (uint8_t)wWord;

        IMG_FREAD(&wWord, 2, 1, IMG_FILE);  /* Bits per Pixel */
        pBmpDec->bBitsPerPixel = (uint8_t)wWord;

        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Compression info */
        pBmpDec->blCompressionType = (uint8_t)lLong;

        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Image length */
        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* xPixels per metre */
        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* yPixels per metre */

        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Palette entries */
        pBmpDec->wPaletteEntries = (uint32_t)lLong;

        if (pBmpDec->wPaletteEntries == 0)
        {
            uint32_t wTemp = (uint32_t)(pBmpDec->lImageOffset - 14 - 40) / 4;
            if (wTemp > 0)
            {
                pBmpDec->wPaletteEntries = wTemp; /* This is because of a bug in MSPAINT */
            }
        }

        IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Important colors */
        if (pBmpDec->bBitsPerPixel == 16 && pBmpDec->bHeaderType > 40)
        {
            IMG_FREAD(&lLong, 4, 1, IMG_FILE);  /* Red mask */
            if ((uint32_t)lLong == 0xF800)
            {
                pBmpDec->b16bit565flag = 1;
            }
        }

        //                  IMG_FSEEK(IMG_FILE, pBmpDec->bHeaderType + 14, 0);

        if (pBmpDec->wPaletteEntries <= 256)
        {
            uint32_t wCounter;
            for (wCounter = 0; wCounter < pBmpDec->wPaletteEntries; wCounter++)
            {
                IMG_FREAD(&pBmpDec->aPalette[wCounter][2], 1, 1, IMG_FILE); /* R */
                IMG_FREAD(&pBmpDec->aPalette[wCounter][1], 1, 1, IMG_FILE); /* G */
                IMG_FREAD(&pBmpDec->aPalette[wCounter][0], 1, 1, IMG_FILE); /* B */
                IMG_FREAD(&wWord, 1, 1, IMG_FILE); /* Dummy */
            }
        }
    }
    return(0);
}

/*******************************************************************************
Function:       uint8_t BMP_bDecode(IMG_FILE *pFile)

Precondition:   None

Overview:       This function decodes and displays a Bitmap image

Input:          Image file

Output:         Error code - '0' means no error
*******************************************************************************/
extern "C" uint8_t BMP_bDecode(int x, int y, int fnbr)
{
    BMPDECODER BmpDec;
    uint32_t wX, wY;
    uint8_t bPadding;
    BDEC_vResetData(&BmpDec);
    BDEC_bReadHeader(&BmpDec, fnbr);
    if (BmpDec.blBmMarkerFlag == 0 || BmpDec.bHeaderType < 40 || (BmpDec.blCompressionType != 0 && BmpDec.blCompressionType != 3))
    {
        return 100;
    }
    IMG_wImageWidth = (uint32_t)BmpDec.lWidth;
    IMG_wImageHeight = (uint32_t)BmpDec.lHeight;
    IMG_vSetboundaries();
    char* linebuff = (char *)GetMemory(IMG_wImageWidth * sizeof(uint32_t)); //get a line buffer

//        IMG_FSEEK(pFile, BmpDec.lImageOffset, 0);
    if (BmpDec.wPaletteEntries == 0 && BmpDec.bBitsPerPixel == 8) /* Grayscale Image */
    {
        bPadding = (4 - (BmpDec.lWidth % 4)) % 4;
        for (wY = 0; wY < BmpDec.lHeight; wY++)
        {
            IMG_vLoopCallback();
            IMG_vCheckAndAbort();
            for (wX = 0; wX < BmpDec.lWidth; wX++)
            {
                uint8_t bY;
                IMG_FREAD(&bY, 1, 1, IMG_FILE); /* Y */
                IMG_vSetColor(bY, bY, bY);
                IMG_vPutPixel(wX, BmpDec.lHeight - wY - 1);
            }
            for (wX = 0; wX < bPadding; wX++)
            {
                uint8_t bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
            }
        }
    }
    else if (BmpDec.bBitsPerPixel == 16) /* 16-bit Color Image */
    {
        bPadding = (4 - ((BmpDec.lWidth * 2) % 4)) % 4;
        for (wY = 0; wY < BmpDec.lHeight; wY++)
        {
            IMG_vLoopCallback();
            IMG_vCheckAndAbort();
            for (wX = 0; wX < BmpDec.lWidth; wX++)
            {
                uint32_t wColor;
                uint8_t bR, bG, bB;
                IMG_FREAD(&wColor, 2, 1, IMG_FILE); /* RGB */
                if (BmpDec.b16bit565flag == 1)
                {
                    bR = (wColor >> 11) << 3;
                    bG = ((wColor & 0x07E0) >> 5) << 2;
                    bB = (wColor & 0x001F) << 3;
                }
                else
                {
                    bR = ((wColor & 0x7FFF) >> 10) << 3;
                    bG = ((wColor & 0x03E0) >> 5) << 3;
                    bB = (wColor & 0x001F) << 3;
                }
                IMG_vSetColor(bR, bG, bB);
                IMG_vPutPixel(wX, BmpDec.lHeight - wY - 1);
            }
            for (wX = 0; wX < bPadding; wX++)
            {
                uint8_t bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
            }
        }
    }
    else if (BmpDec.bBitsPerPixel == 24) /* True color Image */
    {
        int pp;
        bPadding = (4 - ((BmpDec.lWidth * 3) % 4)) % 4;
        for (wY = 0; wY < BmpDec.lHeight; wY++)
        {
            IMG_vLoopCallback();
            IMG_vCheckAndAbort();
            IMG_FREAD(linebuff, 1, BmpDec.lWidth * 3, IMG_FILE); /* B */
                pp = 0;
            for (wX = 0; wX < BmpDec.lWidth; wX++)
            {
                colour.rgbbytes[3] = (char)0xFF;
                colour.rgbbytes[2] = linebuff[pp++];
                colour.rgbbytes[1] = linebuff[pp++];
                colour.rgbbytes[0] = linebuff[pp++];
                IMG_vPutPixel(wX, BmpDec.lHeight - wY - 1);
            }
        for (wX = 0; wX < bPadding; wX++)
            {
                uint8_t bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
            }
        }
    }
    else if (BmpDec.wPaletteEntries != 0 && BmpDec.bBitsPerPixel == 1) /* B/W Image */
    {
        uint32_t wBytesPerRow = BmpDec.lWidth / 8;
        uint8_t bAdditionalBitsPerRow = BmpDec.lWidth % 8;
        bPadding = (4 - ((wBytesPerRow + (bAdditionalBitsPerRow ? 1 : 0)) % 4)) % 4;
        for (wY = 0; wY < BmpDec.lHeight; wY++)
        {
            uint8_t bBits, bValue;
            IMG_vLoopCallback();
            IMG_vCheckAndAbort();
            for (wX = 0; wX < wBytesPerRow; wX++)
            {
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);

                for (bBits = 0; bBits < 8; bBits++)
                {
                    uint8_t bIndex = (bValue & (0x80 >> bBits)) ? 1 : 0;
                    IMG_vSetColor(BmpDec.aPalette[bIndex][0], BmpDec.aPalette[bIndex][1], BmpDec.aPalette[bIndex][2]);
                    IMG_vPutPixel(wX * 8 + bBits, BmpDec.lHeight - wY - 1);
                }
            }
            if (bAdditionalBitsPerRow > 0)
            {
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);

                for (bBits = 0; bBits < bAdditionalBitsPerRow; bBits++)
                {
                    uint8_t bIndex = (bValue & (0x80 >> bBits)) ? 1 : 0;
                    IMG_vSetColor(BmpDec.aPalette[bIndex][0], BmpDec.aPalette[bIndex][1], BmpDec.aPalette[bIndex][2]);
                    IMG_vPutPixel(wX * 8 + bBits, BmpDec.lHeight - wY - 1);
                }
            }
            for (wX = 0; wX < bPadding; wX++)
            {
                uint8_t bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
            }
        }
    }
    else if (BmpDec.wPaletteEntries != 0 && BmpDec.bBitsPerPixel == 4) /* 16 colors Image */
    {
        uint32_t wBytesPerRow = BmpDec.lWidth / 2;
        uint8_t bAdditionalNibblePerRow = BmpDec.lWidth % 2;
        bPadding = (4 - ((wBytesPerRow + bAdditionalNibblePerRow) % 4)) % 4;
        for (wY = 0; wY < BmpDec.lHeight; wY++)
        {
            IMG_vLoopCallback();
            IMG_vCheckAndAbort();
            for (wX = 0; wX < wBytesPerRow; wX++)
            {
                uint8_t bIndex, bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
                bIndex = bValue >> 4;
                IMG_vSetColor(BmpDec.aPalette[bIndex][0], BmpDec.aPalette[bIndex][1], BmpDec.aPalette[bIndex][2]);
                IMG_vPutPixel(wX * 2, BmpDec.lHeight - wY - 1);
                bIndex = bValue & 0x0F;
                IMG_vSetColor(BmpDec.aPalette[bIndex][0], BmpDec.aPalette[bIndex][1], BmpDec.aPalette[bIndex][2]);
                IMG_vPutPixel(wX * 2 + 1, BmpDec.lHeight - wY - 1);
            }
            if (bAdditionalNibblePerRow)
            {
                uint8_t bIndex, bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE); /* Bits8 */
                bIndex = bValue >> 4;
                IMG_vSetColor(BmpDec.aPalette[bIndex][0], BmpDec.aPalette[bIndex][1], BmpDec.aPalette[bIndex][2]);
                IMG_vPutPixel(wX * 2, BmpDec.lHeight - wY - 1);
            }
            for (wX = 0; wX < bPadding; wX++)
            {
                uint8_t bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
            }
        }
    }
    else if (BmpDec.wPaletteEntries != 0 && BmpDec.bBitsPerPixel == 8) /* 256 colors Image */
    {
        bPadding = (4 - (BmpDec.lWidth % 4)) % 4;
        for (wY = 0; wY < BmpDec.lHeight; wY++)
        {
            IMG_vLoopCallback();
            IMG_vCheckAndAbort();
            for (wX = 0; wX < BmpDec.lWidth; wX++)
            {
                uint8_t bIndex;
                IMG_FREAD(&bIndex, 1, 1, IMG_FILE);
                IMG_vSetColor(BmpDec.aPalette[bIndex][0], BmpDec.aPalette[bIndex][1], BmpDec.aPalette[bIndex][2]);
                IMG_vPutPixel(wX, BmpDec.lHeight - wY - 1);
            }
            for (wX = 0; wX < bPadding; wX++)
            {
                uint8_t bValue;
                IMG_FREAD(&bValue, 1, 1, IMG_FILE);
            }
        }
    }
    FreeMemorySafe((void **)&linebuff);
    return 0;
}

