#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// read lines from keyboard, sort them alphabetically . display the sorted list


#define MAXLINE 20
char *lines [MAXLINE];

void display(char* pchar[], int n)
{ 
	for (int num =0; num <n; num ++)
		printf("%s\n", pchar[num]);
	printf ("\n");

}

sort(char * p[], int n)
{
	int a,b;
	char *x;
	for (a=1; a<n; a++)
	{
		for(b=0; b<n-1; b++)
		{
			if (strcmp(p[b], p[b+1]) > 0)
			{
				x = p[b];
				p[b] = p[b+1];
				p[b+1]  = x;
			}
		}
	}
}


int getlines (char *pchar[]);


int main()
{
	// input lines save to arrary of char 

	puts("pls enter one line ata ltime enter a bland when donne\n");
	char * arrary[30];
	int numofline;

	numofline=getlines(lines);
	printf("numof line %d", numofline);
	sort (lines, numofline);
	display(lines, numofline);

	return 0;
}


int getlines (char *pchar[])
{
	int n=0;
	char *buffer [20];
	while ((n< MAXLINE) && (gets(buffer) !=0 )  && (buffer[0] !='\0'))
	{
		if ((lines[n] = (char*) malloc(strlen(buffer)+1))  == NULL)
			return -1;
		strcpy (lines[n++] , buffer);
	}

	return n;
}


