
#include "umalloc.h"

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    memory_block_t *temp;
    temp = free_head;

    //checking if every block in free list is marked as free
    while(temp->next != NULL) {
        temp = temp->next;
        if(is_allocated(temp)) {
            return 1;
        }
    }

    // checking if every free block is on the free list
    temp = heap_head;
    while(temp->next != NULL) {
        temp = temp->next;
        if(!is_allocated(temp)) {
            memory_block_t *free_temp = free_head;
            while(free_temp->next != NULL && free_temp != temp) {
                free_temp = free_temp->next;
            }
            if(free_temp != temp) {
                return 1;
            }
        }
    }

    // checking if there are any overlaps between free blocks
    temp = free_head;
    while(temp->next != NULL && temp->next->next != NULL) {
        temp = temp->next;
        memory_block_t *next = temp->next;
        if((size_t *)&temp & 16 <= (size_t *)&next & 16) {
            return 1;
        }
    }

    // checking if there any overlaps between allocated and free blocks
    temp = heap_head;
    while(temp->next != NULL) {
        temp = temp->next;
        if(is_allocated(temp)) {
            memory_block_t *free_temp = free_head;
            while(free_temp->next != NULL) {
                free_temp = free_temp->next;
                if((size_t *)&free_temp & 16 == (size_t *)&temp & 16) {
                    return 1;
                }
            }
        }
    }
    return 0;
}