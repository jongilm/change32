#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>

FILE *infile;
FILE *outfile;
char oldstring[128];
char newstring[128];
char *buffer;
char *test_from_ptr;
int  oldlen;
int  newlen;
int display = 0;
int displayed = 0;

void usage(void);

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
      printf("Invalid hex character: '%c'",ch);
      exit(1);
   }
}

char *TranslateEscapes ( char *pStr )
{
   int i,j;
   char Result[128];

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
                         printf("Invalid escape sequence: \"\\x00\"\n");
                         exit(1);
                      }
                      Result[j++] = Hex(pStr[i+2])*16+Hex(pStr[i+3]); i+=4;
                      break;
            default :
               printf("Invalid escape sequence: \"\\%c\"\n",pStr[i+1]);
               exit(1);
         }
      }
      else
      {
         Result[j++] = pStr[i++];
      }
   }
   Result[j] = 0;
   strcpy(pStr,Result);
   return pStr;
}

void main(int argc, char *argv[])
{
   long Filesize;
   long OccurancesChanged = 0;
   if (argc < 5) usage();
   if ((argc >= 6) && (toupper(argv[5][1]) == 'D')) display = 1;

   strcpy(oldstring,argv[3]);
   strcpy(newstring,argv[4]);

   TranslateEscapes(oldstring);
   TranslateEscapes(newstring);

   oldlen = strlen(oldstring);
   newlen = strlen(newstring);

   if ((infile  = fopen(argv[1],"rb")) == NULL) usage();

   Filesize = _filelength(_fileno(infile));
   printf("Filesize = %ld\n",Filesize);
   if (Filesize > 65535)
   {
      printf("Infile too big\n");
      exit(1);
   }
   buffer = malloc((unsigned int)Filesize);
   if (!buffer)
   {
      printf("Insufficient memory to allocate %ld bytes\n",Filesize);
      exit(1);
   }


   printf("Reading %ld bytes from %s\n",Filesize,argv[1]);
   if (fread(buffer,1,(unsigned int)Filesize,infile) != Filesize)
   {
      printf("Error reading %ld bytes from infile\n",Filesize);
      exit(1);
   }
   fclose(infile);

   test_from_ptr = buffer;
   for(;;)
   {
      if (memcmp(test_from_ptr,oldstring,oldlen) == 0)
      {
         if (oldlen!=newlen)
         {
            memmove(test_from_ptr+newlen,test_from_ptr+oldlen,(unsigned int)Filesize-(test_from_ptr-buffer)-oldlen+1);
            Filesize += (newlen-oldlen);
         }
         memcpy(test_from_ptr,newstring,newlen);
         test_from_ptr += newlen;
         OccurancesChanged++;
      }
      else
         test_from_ptr++;
      if (test_from_ptr>
      printf("\rTesting byte %ld, Occurances changed = %ld",test_from_ptr-buffer,OccurancesChanged);
   }

   printf("\n");
   printf("Writing %ld bytes to %s\n",Filesize,argv[2]);
   if ((outfile = fopen(argv[2],"wb")) == NULL) usage();
   fwrite(buffer,1,(unsigned int)Filesize,outfile);
   fclose(outfile);
   free(buffer);
}

void usage(void)
{
   puts("Usage : Change <infile> <outfile> <oldstring> <newstring> [/D]");
   exit(1);
}
