#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Jehil Mehta jjm5794" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next_free(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * get_next_block - given a block, returns the next block in memory
 */
memory_block_t *next_block(memory_block_t *block) {
    assert(block != NULL);
    return (memory_block_t *)(block) + HEADER_SIZE + get_size(block);
}

/*
 * remove_free - removes block from free list
 */
void remove_free(memory_block_t *block) {
    memory_block_t* temp = free_head;
    memory_block_t* prev;
   
    while(temp != block && temp != NULL) {
        prev = temp;
        temp = temp->next;
    }
    if(temp == free_head) {
        free_head = get_next_free(block);
    }
    else {
        prev->next = get_next_free(block);
    }
}
/*
 * address_insert_free - inserts a newly freed block into freed
 * list in order of memory address
 * 
*/
void address_insert_free(memory_block_t *block) {
    memory_block_t* temp = free_head;
    if(block < temp) {
        block->next = temp;
        free_head = block;
        return;
    }
    memory_block_t* prev = free_head;
    while(block > temp && temp != NULL) {
        prev = temp;
        temp = temp->next;
    }
    prev->next = block;
    block->next = temp;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    memory_block_t* temp = free_head;
    while(temp != NULL) {
        if(size == get_size(temp)) {
            return temp;
        }
        else if(size < get_size(temp)) {
            return split(temp, size);
        }
        temp = temp->next;
    }
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    memory_block_t *hp;

    hp = (memory_block_t *) csbrk(16 * PAGESIZE);
    if(hp == NULL) {
        return NULL;
    }

    put_block(hp, 16 * PAGESIZE, 0);
    memory_block_t* temp = free_head;

    if(free_head == NULL) {
        free_head = hp;
        return hp;
    } 

    while(temp->next != NULL) {
        temp = temp->next;
    }
    
    temp->next = hp;
    return hp;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t* split(memory_block_t *block, size_t size) {
    size_t block_size = get_size(block);
    put_block(block, size-HEADER_SIZE, 1);
    remove_free(block);
    block = next_block(block);
    put_block(block, block_size - size, 0);
    return block;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    memory_block_t *next = next_block(block);
    size_t size = get_size(block);
    memory_block_t *temp = free_head;
    memory_block_t *prev = free_head;
    while(temp != block && temp != NULL) {
        prev = temp;
        temp = temp->next;
    }

    if(temp == NULL) {
        return NULL;
    }

    while(next_block(prev) != block) {
        prev = next_block(prev);
    }

    bool prev_allocated = is_allocated(prev);
    bool next_allocated = is_allocated(next);

    if(prev_allocated && !next_allocated) {
        size += get_size(next);
        remove_free(next);
        put_block(block, size, 0);
    }
    else if(!prev_allocated && next_allocated) {
        size += get_size(prev);
        block = prev;
        remove_free(block);
        put_block(block, size, 0);
    }
    else if(!prev_allocated && !next_allocated) {
        size += get_size(next) + get_size(prev);
        remove_free(prev);
        remove_free(next);
        block = prev;
        put_block(block, size, 0);
    }

    address_insert_free(block);
    return block;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    free_head = (void *) csbrk(16 * PAGESIZE);
    put_block(free_head, 16 * PAGESIZE, 0);
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    if(size <= 0) {
        return NULL;
    }
    size_t adjusted_size = ALIGN(size);
    
    memory_block_t *hp = find(adjusted_size);

    if(hp) {
        return hp;
    }

    hp = extend(adjusted_size);
    if(hp == NULL) {
        return NULL;
    }

    split(hp, adjusted_size);

    return hp;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    size_t size = get_size((memory_block_t *) ptr);
    put_block((memory_block_t *) ptr, size, 0);
    coalesce((memory_block_t *) ptr);
}