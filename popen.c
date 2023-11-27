#include <stdio.h>

#define  BUF_SIZE   1024

int main( void)
{
   char  buf[BUF_SIZE];
   FILE *fp;

   fp = popen("gcc test2.c","r");
   if (NULL == fp)
   {
      perror("popen() error");
      return -1;
   }
   
   while(fgets(buf, BUF_SIZE, fp))
      printf("%s", buf);

   int a = pclose(fp);
	printf("\n**%d**\n",a); 
   return 0;
}
