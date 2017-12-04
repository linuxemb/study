#include  <stdio.h>
#include <stdlib.h>
// insert elem in any position
// reverse a link list
// recursive print
// reverse recursive print

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


P  reverse()
{

	P prev,current, next;

	current =head;
	prev = NULL;
	while (current != NULL)
	{

		next = current->next;
		current->next = prev;
		// update itrater to working node
		prev = current; // prev save last node

		current = next;
	}
	//update head
	head = prev;
	return head;
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


void   recersive_reverse(struct Node*  p)
{
	if(p->next == NULL) 
	{
		head=p;
		return ;
	} 
	recersive_reverse(p-> next) ;
	struct Node* q = p -> next;
	q->next = p;
        p -> next = NULL;
}


void reverse_print(P p)
{
	if(p==NULL) return;
	reverse_print ( p-> next); // recursive call

	printf("%d \t",p->data);
}
// not use lot of stack, becase it called printf ahead of recruseive
void recursive_print(P p)
{
	if(p==NULL) return;
	printf("%d",p->data);
	recursive_print ( p-> next); // recursive call
}
int main ()
{
	head = NULL;
	insert(2,1);
	insert(3,2);
	insert(4,1);
	insert(5,2);
	print();
	//	recursive_print(head);
	//	reverse(head);
	recersive_reverse(head);

	print();
	//reverse_print(head);
	//	recursive_print(head);
}
