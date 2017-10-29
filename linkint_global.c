#include  <stdio.h>
#include <stdlib.h>

struct Node {

	int data;
	struct  Node* next;
};
typedef struct Node Myn;
typedef Myn *  P;

struct Node* head; //global 

void insert(int x) 

{
	// using type def
	P tmp  =(p)malloc(sizeof(Myn));
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
}
void print() 
{
	struct Node* p= head;
	while ( p !=NULL)
		//	for (int j=0; j<n ; j++)
	{
		printf("%d", p->data);
		p = p->next;
	}
	printf("/n");

}

int main()
{
	// create head node

	head = NULL;

	int n, i, x;
	scanf("%d:", &n);

	for(i =0; i <n; i++) 

	{

		printf("enter the number \n");
		scanf("%d:", &x);
		insert(x);

		print();
	}

}
