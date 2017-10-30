#include  <stdio.h>
#include <stdlib.h>
// insert elem in any position

struct Node {

	int data;
	struct  Node* next;
};
typedef struct Node Myn;
typedef Myn *  P;

P head ;
void insert(int data, int n)
{
	P tmp1  = (P) malloc(sizeof(Myn));
	tmp1 -> data = data;
	tmp1-> next  == NULL;
	//	if(n==1)
	{
		tmp1->   next = head;
		head = tmp1;
		return;
	}
	P tmp2 = head;
	for (int i  = 0;i < n-2; i++)
	{
		tmp2 = tmp2-> next;
	}
	tmp2->next = tmp1;
	tmp1 -> next =  tmp2 -> next;
}  // tmp is new node, tmp2 is positionto insert tmp1

void delet(int n)
{
	P tmp1= head;
	if(n==1)
	{
		head = tmp1->next;
		free (tmp1);
		return;
	}
	int i;
	for(i =0; i< n-2; i++)
		tmp1 = tmp1->next;
	P tmp2 = tmp1->next; // nth node
		tmp1 -> next = tmp2->next;
	free(tmp2);

}

void print() 
{
	struct Node* p= head;
	while ( p !=NULL)
		//	for (int j=0; j<n ; j++)
	{
		printf("%d \t", p->data);
		p  = p->next;
	}
	printf("\n");

}
int main ()
{
	head = NULL;
	insert(2,1);
	insert(3,2);
	insert(4,1);
	insert(5,2);
	print();
	delet(2);
	print();
	delet(1);
	print();


}
