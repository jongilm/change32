// change32.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <ctype.h>

#define MAX_REPLACEMENTS 10
#define MAX_REPLACE_LEN 128

FILE *infile;
FILE *outfile;
char infilename[_MAX_PATH];
char outfilename[_MAX_PATH];
char *buffer;
char *test_from_ptr;
int  verbose = 0;
int  displayed = 0;

struct tReplacement
{
   char oldstringarg[MAX_REPLACE_LEN];
   char newstringarg[MAX_REPLACE_LEN];
   char oldstring[MAX_REPLACE_LEN];
   char newstring[MAX_REPLACE_LEN];
   int  oldlen;
   int  newlen;
   long OccurancesChanged;
} chg[MAX_REPLACEMENTS]; // TODO should make this a malloc'd linked list


void usage(void)
{
   printf("USAGE: change32 [options] <file>\n");
   printf("       Options:\n");
   printf("          -o outfile\n");
   printf("          -r <oldstring> <newstring> (may be repeated up to %d times)\n",MAX_REPLACEMENTS);
   printf("          -v = verbose\n");
   printf("       Supported escape sequences:\n");
   printf("          Seq.   Name              Seq.   Name\n");
   printf("          \\a     Alert (bell)      \\?     Literal quotation mark\n");
   printf("          \\b     Backspace         \\'     Single quotation mark\n");
   printf("          \\f     Formfeed          \\\"     Double quotation mark\n");
   printf("          \\n     Newline           \\\\     Backslash\n");
   printf("          \\r     Carriage return   \\xdd   ASCII character in hex notation\n");
   printf("          \\t     Horizontal tab\n");
   printf("          \\v     Vertical tab\n");
   printf("(J.Gilmore 20 May 2001 - jonnie@gilmoreclan.com)\n");
   exit(1);
}

int Hex ( int ch )
{
   if (ch>='0' && ch<='9')
      return ch-'0';
   else if (ch>='A' && ch<='F')
      return ch-'A'+10;
   else if (ch>='a' && ch<='f')
      return ch-'a'+10;
   else
   {
      printf("ERROR: Invalid hex character: '%c'",ch);
      exit(1);
   }
}

char *TranslateEscapes ( char *pStr )
{
   int i,j;
   char Result[MAX_REPLACE_LEN];

   for (i=0,j=0;pStr[i];)
   {
      if (pStr[i] == '\\')
      {
         switch(pStr[i+1])
         {
            case 'n': Result[j++] = '\n'; i+=2; break;
            case 'r': Result[j++] = '\r'; i+=2; break;
            case 't': Result[j++] = '\t'; i+=2; break;
            case 'f': Result[j++] = '\f'; i+=2; break;
            case '\\': Result[j++] = '\\'; i+=2; break;
            case 'a': Result[j++] = '\a'; i+=2; break;
            //case '0': Result[j++] = '\0'; i+=2; break;
            case 'x': if (pStr[i+2]=='0' && pStr[i+3]=='0' )
                      {
                         printf("ERROR: Invalid escape sequence: \"\\x00\"\n");
                         exit(1);
                      }
                      Result[j++] = Hex(pStr[i+2])*16+Hex(pStr[i+3]); i+=4;
                      break;
            default :
               printf("ERROR: Invalid escape sequence: \"\\%c\"\n",pStr[i+1]);
               exit(1);
         }
      }
      else
      {
         Result[j++] = pStr[i++];
      }
   }
   if (j>=MAX_REPLACE_LEN)
   {
      printf("ERROR: String too long\n");
      exit(1);
   }
   Result[j] = 0;
   strcpy(pStr,Result);
   return pStr;
}

int isspecial(char ch)
{
	if (ch == '@') return 1;
	if (ch == '.') return 1;
	if (ch == '%') return 1;
	if (ch == '$') return 1;
	if (ch == '!') return 1;
	if (ch == ':') return 1;
	if (ch == '/') return 1;
	if (ch == '\\') return 1;
	if (ch == ';') return 1;
	return 0;
}

char *FindWord(char *pData,char *buffer,unsigned long Filesize)
{
   static char szWord[1024];
   char *pStart;
   int Len;
   int FindLine = 1;

   if(iscsym(*pData) || isspecial(*pData))
      FindLine = 0;

   if (FindLine)
   {
      while(isprint(*pData))
         pData--;
   }
   else
   {
      while(iscsym(*pData) || isspecial(*pData))
         pData--;
   }

   pData++;
   if (pData<buffer)
      pData = buffer;
   pStart = pData;

   if (FindLine)
   {
      while(isprint(*pData))
         pData++;
   }
   else
   {
      while(iscsym(*pData) || isspecial(*pData)) //while(isgraph(*pData))
         pData++;
   }

   if (pData>(buffer+Filesize))
      pData = buffer+Filesize;
   Len = (pData-pStart);
   if (Len>=1024)
      Len = 1024-1;
   memcpy(szWord,pStart,Len);
   szWord[Len] = 0;
   return szWord;
}

int main(int argc, char* argv[])
{
   unsigned long Filesize;
   unsigned long MallocSize;
   int repl;
   int i;

   strcpy(infilename,"");
   strcpy(outfilename,"");

   for (i=0;i<MAX_REPLACEMENTS;i++)
   {
      chg[i].oldstring[0] = 0;
      chg[i].newstring[0] = 0;
      chg[i].oldlen = 0;
      chg[i].newlen = 0;
      chg[i].OccurancesChanged = 0;
   }
   repl = 0;

   for (i=1;i<argc;i++)
   {
      if (i>=argc)
      {
         puts("ERROR: Arg error (-? for help)");
         exit(1);
      }
      if ((argv[i][0] == '-')||(argv[i][0] == '/'))
      {
         switch(toupper(argv[i][1]))
         {
            case 'V':
               verbose = 1;
               break;
            case 'O':
               strcpy(outfilename,argv[i+1]);
               i++;
               break;
            case 'R':
               if (repl >= MAX_REPLACEMENTS)
               {
                  puts("ERROR: Too many replacement strings (-? for help)");
                  exit(1);
               }
               strcpy(chg[repl].oldstring,argv[i+1]);
               strcpy(chg[repl].newstring,argv[i+2]);
               i+=2;
               strcpy(chg[repl].oldstringarg,chg[repl].oldstring);
               strcpy(chg[repl].newstringarg,chg[repl].newstring);
               TranslateEscapes(chg[repl].oldstring);
               TranslateEscapes(chg[repl].newstring);
               chg[repl].oldlen = strlen(chg[repl].oldstring);
               chg[repl].newlen = strlen(chg[repl].newstring);
               repl++;
               break;
            case '?':
               usage();
               break;
         }
      }
      else
      {
         if (!infilename[0]) 
            strcpy(infilename,argv[i]);
         else
         {
            puts("ERROR: Cannot specify more than one input filename (-? for help)");
            exit(1);
         }
      }
   }
   if (repl<1)
   {
      puts("ERROR: No replacements specified. Nothing to do (-? for help)");
      exit(1);
   }
   if (!infilename[0])
   {
      puts("ERROR: No input filename specified (-? for help)");
      exit(1);
   }
   if (!outfilename[0])
      strcpy(outfilename,infilename);

   if (verbose)
   {
      printf("InFile = \"%s\"\n",infilename);
      printf("OutFile = \"%s\"\n",outfilename);
      printf("Replacing %d string(s):\n",repl);
      for(i=0;i<repl;i++)
         printf("  %2d: \"%s\" -> \"%s\"\n",i+1,chg[i].oldstringarg,chg[i].newstringarg);
   }


   infile  = fopen(infilename,"rb");
   if (!infile) 
   {
      printf("ERROR: Cannot open infile \"%s\" (-? for help)",infilename);
      exit(1);
   }
   Filesize = _filelength(_fileno(infile));
   //printf("Filesize = %lu\n",Filesize);
   //if (Filesize > 65535)
   //{
   //   printf("ERROR: Infile too big\n");
   //   exit(1);
   //}
   MallocSize = Filesize*4;  // Data in buffer may grow in size
   buffer = (char *)malloc(MallocSize);
   if (!buffer)
   {
      printf("ERROR: Insufficient memory to allocate %lu bytes\n",MallocSize);
      exit(1);
   }

   if (verbose)
      printf("Reading %lu bytes from %s\n",Filesize,infilename);
   else
      printf("%s",infilename);
   if (fread(buffer,1,Filesize,infile) != Filesize)
   {
      printf("ERROR: Error reading infile \"%s\"\n",infilename);
      exit(1);
   }
   fclose(infile);

   for(i=0;i<repl;i++)
   {
      if (verbose)
         printf("\nReplacement %d/%d. (\"%s\" -> \"%s\")\n",i+1,repl+1,chg[i].oldstringarg,chg[i].newstringarg);
      else
         printf(" \tString %d:\t",i+1);
      test_from_ptr = buffer;
      for(;;)
      {
         if ((unsigned long)(test_from_ptr-buffer)>=Filesize)
            break;
         if (memcmp(test_from_ptr,chg[i].oldstring,chg[i].oldlen) == 0)
         {
            if (verbose)
               printf("%4ld: \"%s\" -> ",chg[i].OccurancesChanged+1,FindWord(test_from_ptr,buffer,Filesize));
            if (chg[i].oldlen!=chg[i].newlen)
            {
               memmove(test_from_ptr+chg[i].newlen,test_from_ptr+chg[i].oldlen,Filesize-(test_from_ptr-buffer)-chg[i].oldlen+1);
               Filesize += (chg[i].newlen-chg[i].oldlen);
               if (Filesize >= MallocSize) // TODO should do realloc instead
               {
                  printf("ERROR: Filesize grew too much (Max %lu bytes)\n",MallocSize);
                  exit(1);
               }
            }
            memcpy(test_from_ptr,chg[i].newstring,chg[i].newlen);
            if (verbose)
               printf("\"%s\"\n",FindWord(test_from_ptr,buffer,Filesize));
            test_from_ptr += chg[i].newlen;
            chg[i].OccurancesChanged++;
         }
         else
            test_from_ptr++;
         //if (verbose)
         //   printf("\rSearch=%ld, Changes=%ld",test_from_ptr-buffer,chg[i].OccurancesChanged);
      }
      if (!verbose)
         printf("%ld occurances replaced.",chg[i].OccurancesChanged);
   }

   //if (verbose)
   //   printf("\n");
   if (verbose)
      printf("Writing %lu bytes to %s\n",Filesize,outfilename);
   else
      printf("\n");
   outfile = fopen(outfilename,"wb");
   if (!outfile) 
   {
      printf("ERROR: Cannot open outfile (-? for help) \"%s\"\n",outfilename);
      exit(1);
   }
   fwrite(buffer,1,Filesize,outfile);
   fclose(outfile);
   free(buffer);

   return 0;
}

