#include "stdio.h"
#include "stdlib.h"

struct data {
	char name[20];
	struct data *next;

};

//define typdefs for structure
typedef struct data PERSON;
typedef PERSON * LINK;

int main()
{
	// add an elemen after head
	//#struct person *new;
	//#struct person *head;
	LINK head=NULL;
	LINK new =NULL;
	LINK current =NULL;

	// add 1st to list

	new = (LINK) malloc(sizeof(PERSON));
	new->next = head ;
	head = new;
	strcpy(new->name ,"Abligi");

	// add elem to end of list

	current = head;
	while (current->next!=NULL)

	{ 
		current = current->next;
	}

	new = (LINK)malloc(sizeof(PERSON));
	new->next = NULL;
	current->next = new;
	strcpy ( new->name, "Catherine");



	// add element in middle 2nd position of list
	new = (LINK)malloc(sizeof(PERSON));
	//head -> next = new;  //lost head next 
	//new -> next = head->next; // always bever
	new -> next = head->next; // always bever
	head -> next = new;  //lost head next 
	strcpy (new->name, "Beaver");
	//delete 1st element
	head = head->next;

	// delete last element

	LINK cur1,cur2;
	cur1=head;
	cur2=cur1-> next;

	while (cur2->next  !=NULL)
	{
		cur1=cur2;
		cur2=cur1->next;
	}

	cur1->next = NULL;
//	if(head== cur1)
//		head = NULL;

	// print all element
	current = head;
	while(current != NULL)
	{
		printf ("\n%s" , current->name);
		current = current ->next;
	}
	printf ("\n");
	return 0;
}
