#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "list.h"

void initList(list *l) {
	l->head=NULL;
	l->tail=NULL;
}

void insertFront(list *l,list_link temp) {
	if((l->head)==NULL) {
		l->head=temp;
		l->tail=temp;
	} else {
		temp->next=l->head;
		l->head=temp;
	}
}

void insertRear(list *l,list_link temp) {
	if((l->head)==NULL) {
		l->head=temp;
		l->tail=temp;
	} else {
		l->tail->next=temp;
		l->tail=temp;
	}
}

void insertNext(list *l,list_link temp,int x) {
	list_link t=l->head;
	bool found = false;
	t=l->head;

	while((t!=NULL) && (!found)) {
		if((t->key)==x) {
			found = true;
			break;
		} else {
			t=t->next;
		}
	}

	if(found) {
		if((t->next)==NULL) {
			t->next=temp;
			l->tail=temp;
		} else {
			temp->next=t->next;
			t->next=temp;
		}
	}
}

void deleteNode(list *l,int x) {
	list_link t=l->head,tp=l->head;
	bool found = false;

	while((t!=NULL) && (!found)) {
		if((t->key)==x) {
			found = true;
			break;
		} else {
			tp=t;
			t=t->next;
		}
	}

	if(found) {
		if(t==tp) {
			if((l->head->next)==NULL){
				l->head=NULL;
				l->tail=NULL;
			} else {
				l->head=t->next;
			}
		} else {
			if((t->next)==NULL) {
				l->tail=tp;
				tp->next=NULL;
			} else {
				tp->next=t->next;
			}
		}

		free(t);
	}
}
