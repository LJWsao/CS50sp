#include <sys/mman.h> //mmap, munmap, PROT_READ, PROT_WRITE 
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

size_t MAX_NO_MMAP = 4096; //Max size with no mmap 

const uint64_t HEAP_1 = 0x1000000; 
const uint64_t HEAP_2 = 0x280001000; //10 GB heap offset
const uint64_t HEAP_3 = 0x500001000; //10 GB heap offset
const uint64_t HEAP_4 = 0x780001000; //10 GB heap offset
const uint64_t HEAP_5 = 0xA00001000; //10 GB heap offset
const uint64_t HEAP_6 = 0xC80001000; //10 GB heap offset
const uint64_t HEAP_7 = 0xF00001000; //10 GB heap offset
const uint64_t HEAP_8 = 0x1180001000; //10 GB heap offset
const uint64_t HEAP_9 = 0x1400001000; //10 GB heap offset

//Initialize with addresses of where each heap will start
uint64_t TOP_HEAP_ARRY[9] = {HEAP_1, HEAP_2, HEAP_3, HEAP_4, HEAP_5, HEAP_6, HEAP_7, HEAP_8, HEAP_9};

//Flag to see how many pages are in each heap
size_t HEAP_TRACKER[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

//Linked list for non-mmaped free chunks
struct node {
	struct node *next;
	struct node *prev;
};

//Array with head of each linked list
struct node *FREE_ARRAY[9] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

//HELPER FUNCTIONS
// Calculate log base 2 of a number (floor)
size_t my_log_2(size_t i) {
    if (i == 0) return 0;  // Handle edge case

    size_t count = 0;
    while (i > 1) {  // Changed from i != 0 to i > 1
        i >>= 1;
        count++;
    }
    return count;  // Removed the -1
}

// Check if a number is a perfect power of two
int is_pow2(size_t i) {
    // Handle edge case: 0 is not a power of 2
    if (i == 0) return 0;

    // A power of 2 has exactly one bit set
    // This is a simpler way to check: (i & (i-1)) == 0
    return (i & (i-1)) == 0;
}

// Get the next power of two, or return the number if it's already a power of 2
size_t get_pow2(size_t i) {
    // Handle edge cases
    if (i == 0) return 1;
    if (i <= 16) return 16;
    if (is_pow2(i)) return i;  // Already a power of 2

    // Find the next power of 2
    size_t log2_val = my_log_2(i);
    //Just left shift by the number of places
    return ((size_t)1) << (log2_val + 1);
}

//mmap wrappers
size_t *mmap_wrap_NULL(size_t size) {
	size_t *const result = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);	  
	return result;
}

size_t *mmap_wrap_addr(size_t size, size_t address) {
	size_t *const result = mmap((void *)(address), size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);	  
	return result;
}
//Slice new mmaped page into free chunks + add to linked list 
//Only for malloc <= 4096
void slice_page(size_t chunk_size, size_t *page_addr) {
	//Add new allocations to linked list of free chunks
	size_t num_of_chunks = MAX_NO_MMAP / chunk_size;
	struct node *head = FREE_ARRAY[my_log_2(chunk_size) - 4]; //First node in linked list
	for (size_t i = 0; i < num_of_chunks; i++) {
		//Must cast size_t address into a pointer
		struct node *new_node = (void *)((size_t)(page_addr) + (i * chunk_size));
		new_node->next = head; //End of list is when the next node is null
		if (head != NULL) {
			head->prev = new_node;
		}
		new_node->prev = NULL;
		head = new_node; //Insert at the start, cheaper to do so
	}
	//Put head address into node * array to have a starting point for free
	FREE_ARRAY[my_log_2(chunk_size) - 4] = head;			
}

//Helper function to unlink a chunk from the free list for non-munmap malloc
void unlink_chunk(struct node *chunk) {
	//Head case
	if (((chunk->prev) == NULL) && ((chunk->next) != NULL)) {
		chunk->next->prev = NULL;
	} 
	//Tail case
	else if (((chunk->next) == NULL) && ((chunk->prev) != NULL)) {
		chunk->prev->next = NULL;	
	}
	//Middle Case
	else if (((chunk->prev) != NULL) && ((chunk->next) != NULL)){
		//Make the next prev into chunk's prev 
		chunk->next->prev = chunk->prev; 
		//Make previous next into chunk next
		chunk->prev->next = chunk->next;
	}
	//Remove fully 
	chunk->next = NULL;
	chunk->prev = NULL;
	return;
}

//Size helper for realloc and free
int size_helper(size_t address) {	
	if ((address >= 0x1000000) && (address < 0x280001000)) { 
		return 16;
	}
	else if ((address >= 0x280001000) && (address < 0x500001000)) { 
		return 32;
	}
	else if ((address >= 0x500001000) && (address < 0x780001000)) { 
		return 64;
	}
	else if ((address >= 0x780001000) && (address < 0xA00001000)) { 
		return 128;
	}
	else if ((address >= 0xA00001000) && (address < 0xC80001000)) { 
		return 256;
	}
	else if ((address >= 0xC80001000) && (address < 0xF00001000)) { 
		return 512;
	}
	else if ((address >= 0xF00001000) && (address < 0x1180001000)) { 
		return 1024;
	}
	else if ((address >= 0x1180001000) && (address < 0x1400001000)) { 
		return 2048;
	}
	else if ((address >= 0x1400001000) && (address < (0x1400001000 + 0x280000000))) { 
		return 4096;
	}
	//Indicates chunk has been purely mmaped
	else { 
		return 0;
	}
	
}

void *malloc(size_t n) {
	//Get rounded size
	//TODO: Account for exact factor of 2 case (no rounding needed)
	if (n == 0) {
		n = 1;
	}
	
	size_t round_n = 0;
	
	if (n <= MAX_NO_MMAP) {
		round_n = get_pow2(n);	
	}

	//Check if above cutoff
	if (n > MAX_NO_MMAP) { 
    		//Add 8 to store the size
		n += 8;
		size_t *const result = mmap_wrap_NULL(n);
	  	// Map failed
		if (result == MAP_FAILED) { 
			return NULL;
		}	
		result[0] = n; // Store the allocation size, for later use by munmap
	    	return result + 1; // Return pointer to just after the stored size.
	}
	//Check if linked list of frees is empty 
	else if(FREE_ARRAY[my_log_2(round_n) - 4] == NULL) {
		//mmap a new page at the address of the top of the heap for the given size
		size_t index = my_log_2(round_n) - 4;
		//mmap a page
		size_t *const result = mmap_wrap_addr(round_n, (TOP_HEAP_ARRY[index] + (MAX_NO_MMAP * HEAP_TRACKER[index])));
		//Call slice new page to add chunks to linked list of frees
		slice_page(round_n, result);
		HEAP_TRACKER[index]++; //Increment top of heap tracker
	}
	//Provide address of free chunk, always the head in this case for ease
	struct node *free_chunk = FREE_ARRAY[my_log_2(round_n) - 4];
	FREE_ARRAY[my_log_2(round_n) - 4] = free_chunk->next;
	unlink_chunk(free_chunk);
	return free_chunk;
}

void *calloc(size_t n, size_t size) {
	// Just malloc and memset with some error checking
	size_t n_size = n * size;
	void *result = malloc(n_size);
	if (result == MAP_FAILED) {
		return NULL;
	}
	memset(result, 0, n_size);
	return result;
}

void free(void *const p) {
	//NULL case
	if (p == NULL) {
		return;
	}
	//TODO: For some reason the size helper is not returning as intended 
	size_t address = (size_t)(p);
	int size = size_helper(address);	
	//Munmap case	
	if (size == 0) {
		size_t *const size_ptr = (size_t *)p - 1;
    		munmap(size_ptr, size_ptr[0]);
	}
	//TODO: Account for case where all of memory is in the free linked list, so you can munmap pages
	//Non munmap, add to linked list
	else {
		struct node *free_head = FREE_ARRAY[my_log_2(size) - 4];	
		struct node *chunk_to_add = (struct node *)(p);
		//Add to head of list if head is not null
		if (free_head != NULL) {
			free_head->prev = p;
			chunk_to_add->next = free_head;
		}
		chunk_to_add->prev = NULL;
		//Make new addition to the linked list the head
		FREE_ARRAY[my_log_2(size) - 4] = chunk_to_add;		
	}
}

void *realloc(void *p, size_t size) {
	// Checks for p
	if (p == NULL) {
		return malloc(size);
	}

	if (size == 0) {
		free(p);
		return NULL;
	}	

	//Could this potentially lead to bytes getting cut off?
	//CHECK
	void *result = malloc(size);

	if (result == NULL) {
		return NULL;
	}

	size_t original_size = size_helper((size_t)(p)); 
	//mmaped memory case
	if (original_size == 0) {
		size_t *size_ptr = (size_t *)p - 1; //-8 bytes to account for size at start		
		//Original allocation size
		original_size = *size_ptr;
	}

	//Calculate size to copy, find minimum of original and new size	
	size_t cpy_size = (original_size < size + 1) ? original_size - 1 : size;

	// Copy the relevant bytes from p to result
	memcpy(result, p, cpy_size); 	
	//Free the old location of the bytes
	free(p);
	return result;
}

void *reallocarray(void *p, size_t n, size_t size) {
	return realloc(p, n * size);
}
