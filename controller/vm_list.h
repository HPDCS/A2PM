/*
 * vm_list.h
 *
 *  Created on: Jul 9, 2015
 *      Author: io
 */

#ifndef VM_LIST_H_
#define VM_LIST_H_



#endif /* VM_LIST_H_ */

struct virtual_machine{
	char ip[16];                    // vm's ip address
};

struct vm_list_elem {
	struct virtual_machine vm;
	struct vm_list_elem *next;
};


