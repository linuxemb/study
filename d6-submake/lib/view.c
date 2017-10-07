#include "view.h"
#include "stdio.h"
void view(int i)
{
	int value = i%10;
	int l;
	
	if(TYPE == NUMBER)
	{
		printf("Value is %d\n",value);
	}
	else
	{
		printf("Result is : ");
		for(l=0;l<value;l++)
			printf("-");
		printf("\n");
	}
}

