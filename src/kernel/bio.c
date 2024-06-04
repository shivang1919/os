// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
    struct spinlock lock;
    struct buf buf[NBUF];
    
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;
void 
binit(void){
    struct buf *b;
    initlock(&bcache.lock,"bcache");
    // create linkedlist of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for(b=bcache.buf;b<bcache.buf+NBUF;b++){
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        initsleeplock(&b->lock,"buffer");
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

// look through buffer cache for block on device dev;
// if not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev,uint blockno)
{
    struct buf *b;
    acquire(&bcache.lock);
    // Is block already cached?
    for(b = bcache.head.next ; b!=&bcache.head;b=b->next){
        if(b->dev == dev && b->blockno == blockno){
            b->refcnt++;
            release(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    // Not cached
    // Recycle the least recently used (LRU) unused buffer.
    for(b = bcache.head.prev;b!=&bcache.head;b = b->prev){
        if(b->refcnt == 0){
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    panic("bget:no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}



// The provided code implements a buffer cache for a file system. The buffer cache is a memory-resident structure that stores copies of disk blocks to reduce the number of disk reads and writes, which are generally slow operations. This cache also ensures that multiple processes can access disk blocks in a synchronized manner. Here's an overview and explanation of the code:


// Key Structures and Variables
// bcache:

// A global structure that holds the buffer cache.
// It contains a spinlock (lock) for synchronizing access to the cache.
// An array of buffers (buf), each representing a cached disk block.
// A linked list (head) that manages the buffers in a least recently used (LRU) order.
// struct buf:

// Represents an individual buffer.
// Contains information about the device (dev), block number (blockno), validity (valid), reference count (refcnt), and the buffer's lock (lock).
// Also includes pointers for linking buffers in the LRU list (prev and next).
// Functions
// binit:

// Initializes the buffer cache.
// Sets up the linked list of buffers and initializes their locks.
// All buffers are initially linked into the LRU list with the head as both the first and last element.
// bget:

// Looks for a buffer corresponding to a specific device and block number.
// If found, it locks and returns the buffer.
// If not found, it selects an unused buffer (using the LRU policy) and assigns it to the requested block.
// If all buffers are in use (refcnt != 0), it triggers a panic.
// bread:

// Retrieves a buffer for a specific block from the cache.
// If the buffer is not valid (i.e., data not yet read from disk), it reads the block from the disk.
// Returns the buffer locked for use.
// bwrite:

// Writes the contents of a buffer to disk.
// The buffer must be locked before calling this function.
// Uses the virtio_disk_rw function to perform the actual disk write operation.
// brelse:

// Releases a locked buffer, making it available for other processes.
// Decrements the buffer's reference count.
// If the reference count reaches zero, it moves the buffer to the head of the LRU list.
// bpin and bunpin:

// Increment (bpin) or decrement (bunpin) the reference count of a buffer.
// Used to manage the buffer’s reference count explicitly without affecting the LRU order.
// Explanation of Buffer Management
// LRU List:

// The LRU list is maintained by linking buffers via prev and next pointers.
// binit sets up this list initially with all buffers linked.
// When a buffer is used (bget finds or allocates it), it is locked and marked as recently used.
// When a buffer is released (brelse), it is moved to the front of the list if no process is using it (i.e., refcnt == 0).
// Reference Counting:

// refcnt tracks how many processes are using the buffer.
// When a buffer is obtained (bget), its refcnt is incremented.
// When a buffer is released (brelse), its refcnt is decremented.
// If refcnt is zero, the buffer can be reused for other blocks.
// Synchronization:

// Spinlocks and sleep locks are used to synchronize access to the buffer cache and individual buffers, respectively.
// acquire and release functions manage spinlocks for the entire cache.
// acquiresleep and releasesleep manage sleep locks for individual buffers.
// Overall Flow
// Initialization:

// binit initializes the buffer cache and sets up the LRU list.
// Buffer Retrieval:

// bread is called to get a buffer for a specific block. It uses bget to find or allocate the buffer.
// If the buffer data is not valid, it reads from the disk.
// Buffer Usage:

// Once a buffer is locked, processes can read or write to it.
// bwrite is called to write modified buffer data back to the disk.
// Buffer Release:

// brelse releases the buffer, updating its position in the LRU list if it is no longer in use.
// This implementation ensures efficient disk I/O operations and proper synchronization among multiple processes accessing the buffer cache.