#include "condvar.h"
#include "semaphore.h"
// A barrier is a synchronization primitive used to block a group of processes or threads until all members of the group reach a certain point. The fields are:

// flag: A flag to signal the state of the barrier.
// left_processes: The number of processes that still need to reach the barrier.
// num_processes: The total number of processes that need to reach the barrier.
// instance: An instance counter to track the number of times the barrier has been used.
// cv: A condition variable to wait and notify processes.
// lk: A sleep lock to protect the barrier's state.
struct barrier{
    int flag;
    int left_processes;
    int num_processes;
    int instance;
    struct cond_t cv;
    struct sleeplock lk;
};

// A bounded_buffer_semaphore implements a producer-consumer problem using semaphores. The fields are:

// size: The maximum number of elements the buffer can hold.
// empty: A semaphore to track empty slots in the buffer.
// full: A semaphore to track filled slots in the buffer.
// prod: A semaphore for mutual exclusion for producers.
// cons: A semaphore for mutual exclusion for consumers.
// buffer: An array to hold the buffer elements.
// nextp: The index for the next position to be produced into.
// nextc: The index for the next position to be consumed from.

struct bounded_buffer_semaphore{
    int size;
    struct semaphore empty;
    struct semaphore full;
    struct semaphore prod;
    struct semaphore cons;

    int buffer[20];
    int nextp;
    int nextc;
};

// A buffer_elem is an element in a bounded buffer managed with condition variables. The fields are:

// x: The actual value stored in the buffer element.
// full: A flag indicating if the buffer element is full.
// lock: A sleep lock for protecting this element's state.
// inserted: A condition variable to signal when an element is inserted.
// deleted: A condition variable to signal when an element is deleted.
struct buffer_elem {
    int x;
    int full;
    struct sleeplock lock;
    stuct cond_t inserted;
    struct cond_t deleted;
};

// A bounded_buffer_condition is a bounded buffer using condition variables. The fields are:

// buffer: An array of buffer_elem to hold the buffer elements.
// tail: The index for the next position to insert.
// head: The index for the next position to delete.
// lock_delete: A lock for managing deletion operations.
// lock_insert: A lock for managing insertion operations.
// lock_print: A lock for managing printing operations (or possibly any other critical section).
struct bounded_buffer_condition{
    struct buffer_elem buffer[20];
    int tail;
    int head;
    struct sleeplock lock_delete;
    struct sleeplock lock_insert;
    struct sleeplock lock_print;

};
// These extern declarations specify that these structures are defined in another file. They provide external linkage so that other parts of the program can access and manipulate these structures.

// buffer_condition: An instance of bounded_buffer_condition for managing a buffer with condition variables.
// buffer_sem: An instance of bounded_buffer_semaphore for managing a buffer with semaphores.
// barrier_array: An array of barrier structures to manage multiple barrier instances.
extern struct bounded_buffer_condition buffer_condition;
extern struct bounded_buffer_semaphore buffer_sem;
extern struct barrier barrier_array[10];


// In summary, this code defines and manages two types of bounded buffers (one using semaphores and the other using condition variables) and barriers to synchronize multiple processes or threads. The external declarations indicate that these structures are used across different parts of the program.






