/*
 * vm_list.h
 *
 *  Created on: Jul 9, 2015
 *      Author: io
 */

#ifndef VM_LIST_H_
#define VM_LIST_H_

#include "data_structures.h"



#endif /* VM_LIST_H_ */

struct virtual_machine{
	char ip[16];                    // vm's ip address
	enum vm_state state;
    int socket;
    int port;
    system_features last_features;
    int last_system_features_stored;
    float mttf;
};

struct vm_list_elem {
	struct virtual_machine *vm;
	struct vm_list_elem *next;
};

void add_vm(struct virtual_machine * vm, struct vm_list_elem **vm_list);
void remove_vm_by_ip(char ip[], struct vm_list_elem **vm_list);
int vm_list_size(struct vm_list_elem *vm_list);
void print_vm_list(struct vm_list_elem *vm_list);
struct virtual_machine *get_vm_by_position(int position, struct vm_list_elem *vm_list);


