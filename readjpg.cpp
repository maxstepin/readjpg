/* readjpg.cpp
 *
 * Simple parser of JPEG structure
 *
 * Copyright (c) 2017 Max Stepin
 * https://github.com/maxstepin
 *
 * MIT License
 * -----------
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool verbose = false;

int get_length(unsigned char * p, bool pr)
{
  int len = p[0] * 256 + p[1];
  if (pr)
  {
    char szStr[99];
    sprintf(szStr, " [len=%d]", len);
    printf("%-13s", szStr);
  }
  return len;
}

unsigned short get16(unsigned char * p, int ii)
{
  return (ii) ? *(short *)p : p[0] * 256 + p[1];
}

unsigned int get32(unsigned char * p, bool ii)
{
  return (ii) ? *(int *)p : ((p[0] * 256 + p[1]) * 256 + p[2]) * 256 + p[3];
}

void findjpg(unsigned char * pData, int size, bool pr);

int readjpg(unsigned char * pData, int c0, int size, bool pr)
{
  int i, j, len;
  int yuv[256];
  int xres = 0;
  int yres = 0;

  int c = c0;
  int m = pData[c++];
  int n = pData[c++];

  if (m == 0xFF && n == 0xD8)
  {
    if (pr) printf("[0x%7X] %X%X [SOI] %s\n", c-2, m, n, verbose ? "(Start Of Image)" : "");
    while (c < size-1)
    {
      m = pData[c++]; if (m != 0xFF) continue;
      n = pData[c];   if (n == 0xFF) continue;
      c++;
      if (n <= 0xBF) continue;
      if (n >= 0xD0 && n <= 0xD7) continue;

      if (pr) printf("[0x%7X] %X%X", c-2, m, n);

      if (n >= 0xC0 && n <= 0xCF) // [SOFn]
      {
        len = get_length(pData + c, pr);
        c += 2;
        if (pr)
        {
          if (n == 0xC4)
            printf(" [DHT]  %s", verbose ? "(Huffman Table)" : "");
          else
          if (n == 0xCC)
            printf(" [DAC]  %s", verbose ? "(Arithmetic Coding)" : "");
          else
            printf(" [SOF%d]", n - 0xC0);
        }

        if (n <= 0xC3 || n == 0xC9 || n == 0xCA || n == 0xCB)
        {
          yres = pData[c+1]*256 + pData[c+2];
          xres = pData[c+3]*256 + pData[c+4];
          if (pr)
          {
            printf(" (%s%s)", verbose ? "Start Of Frame - " : "", (n == 0xC0) ? "Baseline" : (n == 0xC1 || n == 0xC9) ? "Extended sequential" : (n == 0xC2 || n == 0xCA) ? "Progressive" : "Lossless");
            printf(" [%dx%d]", xres, yres);
            yuv[0] = yuv[1] = yuv[2] = yuv[3] = yuv[4] = yuv[5] = -1;
            for (i=0; i<pData[c+5]; i++)
            {
              j = pData[c+6+i*3];
              yuv[j] = pData[c+6+i*3+1];
            }
            if (yuv[0] < 0 && yuv[1] == 0x22 && yuv[2] == 0x11 && yuv[3] == 0x11 && yuv[4] < 0 && yuv[5] < 0) printf(" [420]"); else
            if (yuv[0] < 0 && yuv[1] == 0x21 && yuv[2] == 0x11 && yuv[3] == 0x11 && yuv[4] < 0 && yuv[5] < 0) printf(" [422]"); else
            if (yuv[0] < 0 && yuv[1] == 0x11 && yuv[2] == 0x11 && yuv[3] == 0x11 && yuv[4] < 0 && yuv[5] < 0) printf(" [444]"); else
            if (yuv[0] == 0x22 && yuv[1] == 0x11 && yuv[2] == 0x11 && yuv[3] < 0 && yuv[4] < 0 && yuv[5] < 0) printf(" [420-intel]"); else
            if (yuv[0] == 0x21 && yuv[1] == 0x11 && yuv[2] == 0x11 && yuv[3] < 0 && yuv[4] < 0 && yuv[5] < 0) printf(" [422-intel]"); else
            if (yuv[0] == 0x11 && yuv[1] == 0x11 && yuv[2] == 0x11 && yuv[3] < 0 && yuv[4] < 0 && yuv[5] < 0) printf(" [444-intel]"); else
            for (i=0; i<pData[c+5]; i++)
            {
              j = pData[c+6+i*3];
              if (j==1) printf(" Y=%d:%d",  yuv[j] >> 4, yuv[j] & 15); else
              if (j==2) printf(" Cb=%d:%d", yuv[j] >> 4, yuv[j] & 15); else
              if (j==3) printf(" Cr=%d:%d", yuv[j] >> 4, yuv[j] & 15); else
              if (j==4) printf(" I=%d:%d",  yuv[j] >> 4, yuv[j] & 15); else
              if (j==5) printf(" Q=%d:%d",  yuv[j] >> 4, yuv[j] & 15);
            }
          }
          else
            printf(" th=[%dx%d]", xres, yres);
        }
        c += (len-2);
      }
      else
      if (n == 0xD8) // [SOI]
      {
        if (pr) printf(" [SOI] %s", verbose ? "(Start Of Image)" : "");
      }
      else
      if (n == 0xD9) // [EOI]
      {
        if (pr) printf(" [EOI] %s", verbose ? "(End Of Image)" : "");

        while (c+28 <= size && memcmp(pData + c, "CANON OPTIONAL", 14) == 0)
        {
          if (pr) printf("\n[0x%7X] %s", c, pData + c);
          len = 28 + ((pData[c+24]*256 + pData[c+25])*256 + pData[c+26])*256 + pData[c+27];
          if (pr) printf(" [len=0x%X]", len);
          c += len;
        }
        if (pr) printf("\n");

        while (c < size-1 && (pData[c] != 0xFF || pData[c+1] != 0xD8))
          c++;

        continue;
      }
      else
      if (n == 0xDA) // [SOS]
      {
        len = get_length(pData + c, false);
        if (pr) printf(" [SOS] %s", verbose ? "(Start Of Scan)" : "");
        c += len;
      }
      else
      if (n == 0xDB) // [DQT]
      {
        len = get_length(pData + c, pr);
        if (pr) printf(" [DQT]  %s", verbose ? "(Quantization Table)" : "");
        c += len;
      }
      else
      if (n == 0xDC) // [DNL]
      {
        len = get_length(pData + c, pr);
        if (pr) printf(" [DNL]  %s", verbose ? "(Number of lines)" : "");
        c += len;
      }
      else
      if (n == 0xDD) // [DRI]
      {
        len = get_length(pData + c, pr);
        if (pr) printf(" [DRI]  %s", verbose ? "(Restart Interval)" : "");
        c += len;
      }
      else
      if (n == 0xDE) // [DHP]
      {
        len = get_length(pData + c, pr);
        if (pr) printf(" [DHP]  %s", verbose ? "(Hierarchical Progression)" : "");
        c += len;
      }
      else
      if (n == 0xDF) // [EXP]
      {
        len = get_length(pData + c, pr);
        if (pr) printf(" [EXP]  %s", verbose ? "(Expand Reference Component)" : "");
        c += len;
      }
      else
      if (n >= 0xE0 && n <= 0xEF) // [APPn]
      {
        len = get_length(pData + c, pr);
        if (!pr)
        {
          c += len;
          continue;
        }
        if (n == 0xE0) // [APP0]
        {
          printf(" [%s]", pData + c + 2);
          if (memcmp(pData + c + 2, "JFIF", 4) == 0)
          {
            printf(" [ver=%d.%d]", pData[c+7], pData[c+8]);
            printf(" [%dx%d %s]", pData[c+10]*256 + pData[c+11], pData[c+12]*256 + pData[c+13], (pData[c+9] == 1) ? "dpi" : (pData[c+9] == 2) ? "dpcm" : "(aspect)");
            if (pData[c+14] != 0 || pData[c+15] != 0)
              printf(" [thumb=%dx%d]", pData[c+14], pData[c+15]);
          }
          else
          if (memcmp(pData + c + 2, "JFXX", 4) == 0)
          {
            if (pData[c+7] == 0x10) printf(" [thumb in JPEG]"); else
            if (pData[c+7] == 0x11) printf(" [thumb in 1 bpp]"); else
            if (pData[c+7] == 0x13) printf(" [thumb in 3 bpp]");
          }
        }
        else
        if (n == 0xE1) // Exif
        {
          printf(" [EXIF]");
        }
        else
        if (n == 0xE2 && memcmp(pData + c + 2, "FPXR", 4) == 0) // APP2-FPXR
        {
          printf(" [%c%c%c%c]", pData[c+2], pData[c+3], pData[c+4], pData[c+5]);
        }
        else
        if (n == 0xE2 && memcmp(pData + c + 2, "MPF", 3) == 0)  // APP2-MPF
        {
          printf(" [%c%c%c] ", pData[c+2], pData[c+3], pData[c+4]);
          bool ii = (pData[c+6] == 'I' && pData[c+7] == 'I');
          unsigned char * p = pData + c + 16;
          unsigned short tag = get16(p, ii);
          while ((tag & 0xFF00) == 0xB000)
          {
            if (tag == 0xB002)
            {
              unsigned int offset1 = get32(p + 8, ii);
              unsigned int offset2 = get32(pData + c + 2 + offset1, ii);
              p = pData + c + 8 + offset2;
            }
            else
              p += 12;

            tag = get16(p, ii);
          }
          if (tag == 0xB101)
          {
            unsigned int val = get32(p + 8, ii);
            if (val == 1) printf(" LEFT"); else
            if (val == 2) printf(" RIGHT");
          }
        }
        else
        if (n == 0xEE) // Copyright
        {
          printf(" [");
          for (i=2; i<len; i++)
          {
            if (pData[c+i] == 0) break;
            if (pData[c+i] == 10) break;
            if (pData[c+i] == 13) break;
            printf("%c", pData[c+i]);
          }
          printf("]");
        }
        else
          printf(" [APP%d]", n - 0xE0);

        if (n >= 0xE1 && n <= 0xED)
          findjpg(pData + c + 2, len - 2, false);

        c += len;
      }
      else
      if (n >= 0xF0 && n <= 0xFD) // [JPGn]
      {
        len = get_length(pData + c, pr);
        if (pr) printf(" [JPG%d]", n - 0xF0);
        c += len;
      }
      else
      if (n == 0xFE)  // Comment
      {
        len = get_length(pData + c, pr);
        if (pr)
        {
          printf(" [");
          for (i=2; i<len; i++)
          {
            if (pData[c+i] == 0) break;
            if (pData[c+i] == 10) break;
            if (pData[c+i] == 13) break;
            printf("%c", pData[c+i]);
          }
          printf("]");
        }
        c += len;
      }
      if (pr) printf("\n");
    }
  }
  else
    if (pr) printf("\nNot a JPEG\n");

  return c;
}

void findjpg(unsigned char * pData, int size, bool pr)
{
  int c = 0;
  while (c < size-1)
  {
    if (pData[c] == 0xFF && pData[c+1] == 0xD8)
      c = readjpg(pData, c, size, pr);
    else
      c++;
  }
}

int loadjpg(char * szIn)
{
  FILE * f1;

  if ((f1 = fopen(szIn, "rb")) == NULL)
  {
    printf( "ERROR: can't read the file\n" );
    return 1;
  }

  printf("\n%s\n", szIn);
  for (int i=0; szIn[i] != '\0'; i++) printf("-");
  printf("\n");

  fseek(f1, 0, SEEK_END);
  int size = (int)ftell(f1);

  if (size == 0)
  {
    printf("ERROR: filesize = 0\n");
    fclose(f1);
    return 1;
  }

  unsigned char * pData = new unsigned char[size];

  fseek(f1, 0, SEEK_SET);
  int size_read = fread(pData, 1, size, f1);
  fclose(f1);

  if (size_read != size)
  {
    printf("ERROR: incomplete read (%d != %d)\n", size_read, size);
    return 1;
  }

  readjpg(pData, 0, size, true);

  delete[] pData;
  return 0;
}

int main(int argc, char * argv[])
{
  char  * szIn;
  char  * szOpt;

  if (argc <= 1)
  {
    printf("\nUsage: readjpg file.jpg [/verbose]\n");
    return 1;
  }

  szIn = argv[1];

  for (int i=2; i<argc; i++)
  {
    szOpt = argv[i];

    if (szOpt[0] == '/' || szOpt[0] == '-')
    {
      if (szOpt[1] == 'v' || szOpt[1] == 'V')
        verbose = true;
    }
  }

  int res = loadjpg(szIn);

  if (!res)
    printf("OK\n");

  return res;
}
