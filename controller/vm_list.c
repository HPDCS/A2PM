#include <stdlib.h>
#include "vm_list.h"

void add_vm(struct virtual_machine * vm, struct vm_list_elem **vm_list) {
	struct vm_list_elem * new_vm_list_elem = (struct vm_list_elem *) malloc(sizeof(struct vm_list_elem));
	new_vm_list_elem->vm= vm;
	new_vm_list_elem->next=NULL;

	if (*vm_list == NULL) {
		*vm_list = new_vm_list_elem;
		//printf("Vm list is void. Added vm %s\n", vm_list->vm.ip);
	} else {
		struct vm_list_elem* vm_list_temp = *vm_list;
		while (vm_list_temp->next) {
			vm_list_temp = vm_list_temp->next;
		}
		vm_list_temp->next = new_vm_list_elem;
		//printf("Vm list is not void. Added vm %s\n", vm_list_temp->vm.ip);

	}
}

void remove_vm_by_ip(char ip[], struct vm_list_elem **vm_list) {
	if (*vm_list == NULL) {
		return;
	} else {
		if (strcmp((*vm_list)->vm->ip, ip) == 0) {
			struct vm_list_elem *to_delete = *vm_list;
			*vm_list = (*vm_list)->next;
			free(to_delete);
		} else {
			struct vm_list_elem *vm_temp = *vm_list;

			while (vm_temp->next) {
				if (strcmp(vm_temp->next->vm->ip, ip) == 0) {
					struct vm_list_elem *to_delete = vm_temp->next;
					vm_temp->next = vm_temp->next->next;
					free(to_delete);
					return;
				}
				vm_temp = vm_temp->next;
			}
			return;
		}
	}
}

int vm_list_size(struct vm_list_elem *vm_list) {
	if (vm_list == NULL) {
		return 0;
	} else {
		int count = 1;
		struct vm_list_elem *vm_temp = vm_list;
		while (vm_temp->next != NULL) {
			count++;
			vm_temp = vm_temp->next;
		}
		return count;
	}
}

void print_vm_list(struct vm_list_elem *vm_list) {
	if (vm_list == NULL) {
		printf("Vms list is void\n");
	} else {
		struct vm_list_elem *vm_temp = vm_list;
		int pos = 0;
		printf("Vm[%i]: %s - state: %i\n", pos, vm_temp->vm->ip,vm_temp->vm->state);
		while (vm_temp->next != NULL) {
			vm_temp = vm_temp->next;
			pos++;
			printf("Vm[%i]: %s - state: %i\n", pos, vm_temp->vm->ip, vm_temp->vm->state);
		}

	}
}

struct virtual_machine *get_vm_by_position(int position,
		struct vm_list_elem *vm_list) {
	if (vm_list == NULL) {
		return NULL;
	} else {
		int current_position = 0;
		struct vm_list_elem *vm_temp = vm_list;
		while (current_position < position && vm_temp->next != NULL) {
			current_position++;
			vm_temp = vm_temp->next;
		}
		if (current_position == position) {
			//printf("Returning %s\n", vm_temp->vm.ip);
			return &vm_temp->vm;
		} else {
			return NULL;
		}
	}
}
