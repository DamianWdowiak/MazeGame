#ifndef linked__list_h
#define linked__list_h

struct node_t
{
	int data,x,y;
	struct node_t* next;
};

struct linked_list_t
{
	struct node_t* head;
	struct node_t* tail;
};

struct linked_list_t* ll_create();

int ll_push_back(struct linked_list_t* ll, int value,int x,int y);
int ll_pop_back(struct linked_list_t* ll, int* err_code);
int ll_pop_front(struct linked_list_t* ll, int* err_code);

int ll_remove(struct linked_list_t* ll, unsigned int index, int* err_code);

void ll_clear(struct linked_list_t* ll);


#endif // !linked__list_h