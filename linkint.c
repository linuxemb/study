/*

insert element in 1st post of linklist
way 1--- using return node for update head
way2, --- using pass point to head to avoid return of insert
*/



#include  <stdio.h>
#include <stdlib.h>


struct Node {

	int data;
	struct  Node* next;
};
typedef struct Node Myn;
typedef Myn *  P;

/* -----way 1
   P insert (P head,int x) 

   {
// using type def
P tmp  =(P)malloc(sizeof(Myn));
// add 1st elemen insert at begining .t
// no using typedef
//	struct Node* tmp =(struct Node*)malloc(sizeof(struct Node));
tmp ->data = x;
tmp->next = NULL;
//	head = tmp; // when it is empty
// add emel when not emptybuild Need to set the next of new node to preous's next

if(head !=NULL) tmp->next = head;// let new node point to the node where old head point to 
// for null head, no need do previous step
head = tmp; // only need put head at tmp 
return head;
}
 */
// Way --2 

void insert( P * pToHead, int x)  // using type def
//void insert(struct  Node ** pToHead, int x) 
{
	P tmp = (P) malloc(sizeof(Myn));
	tmp -> data =x;
	tmp -> next = NULL;
	if( *pToHead  != NULL ) tmp-> next = *pToHead;
	*pToHead = tmp;

}
// end way-2

void print(P head) 
{
	//	struct Node* p= head;
	while ( head !=NULL)
		//	for (int j=0; j<n ; j++)
	{
		printf("%d \t", head->data);
		head  = head->next;
	}
	printf("\n");
}

int main()
{
	// create head node

	struct Node* head; 
	head = NULL;


	int n, i, x;
	scanf("%d:", &n);

	for(i =0; i <n; i++) 

	{
		printf("enter the number \n");
		scanf("%d:", &x);

		// Way 1 --- let insert return head and update it
		//	head =insert(head, x); // update head
		// Way 2 ---- pass p_to_p of head as augment. 

		insert(&head, x);
		// ===========================
		print(head);
	}

}
