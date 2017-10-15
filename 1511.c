/* using pointer  to func to control sort order

   read lines from keyboard, sort them alphabetically . display the sorted list*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXLINE 20
char *lines [MAXLINE];

void display(char* pchar[], int n);
void sort(char * p[], int n, int sort_type);
int alpha(char * p1, char * p2);
int reverse(char *p2, char *p1);
// func pointer take argment 
int  (*compare) (char * p1, char *p2 ) ;
int getlines (char *pchar[]);


int main()
{
	int (*p) (char *p1,char p2); 
	// input lines save to arrary of char 

	puts("pls enter one line ata ltime enter a bland when donne\n");
	char * arrary[30];
	int numofline;

	numofline=getlines(lines);
	int sort_type;
	puts("pls enter sort type 0 for alpha, 1 for reverse order \n");
	printf("numof line %d", numofline);
	scanf("%d", &sort_type);
	sort (lines, numofline, sort_type);
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


void display(char* pchar[], int n)
{ 
	for (int num =0; num <n; num ++)
		printf("%s\n", pchar[num]);
	printf ("\n");

}

void sort(char * p[], int n, int sort_type)
{
	int a,b;
	char *x;

	// local fun pointor define
	int  (*compare) (char * p1, char *p2 ) ;
	// Initialize the pointer to point to proper compareisoin 
	// fun depene o te argument sort_type

	compare = (sort_type)? reverse : alpha;


	for (a=1; a<n; a++)
	{
		for(b=0; b<n-1; b++)
		{
			//			if (strcmp(p[b], p[b+1]) > 0)
			if (compare(p[b], p[b+1]) > 0)
			{
				x = p[b];
				p[b] = p[b+1];
				p[b+1]  = x;
			}
		}
	}
}
int alpha(char * p1, char* p2)
{
	return strcmp(p1,p2);
}
int reverse(char*p2, char*p1)
{
	return strcmp(p2,p1);
}


