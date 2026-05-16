#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "israeli_lock.h"

static struct israeli_lock locks[MAX_LOCKS]; // Array to hold all locks
static struct spinlock locks_lock;


void
israeli_lockinit(void)
{
    initlock(&locks_lock, "israeli_lock_table");
}

static struct israeli_lock *
lock_lookup(int lock_id)
{
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (locks[i].used && locks[i].id == lock_id) {
            return &locks[i];
        }
    }
    return 0;
}

int israeli_create(int favoritism){
    if (favoritism < 0 || favoritism > 100) {
        return -1;
    }

    acquire(&locks_lock);
    for (int i = 0; i < MAX_LOCKS; i++) {
        if (!locks[i].used) {
            struct israeli_lock *lock = &locks[i];
            lock->used = 1;
            lock->id = lcg_rand(); // Assign a random ID to the lock
            lock->locked = 0;
            lock->favoritism = favoritism;
            lock->owner = 0;
            lock->queue_size = 0;
            lock->start_index = 0;
            initlock(&lock->lk, "israeli_internal");
            memset(lock->waiting_queue, 0, sizeof(lock->waiting_queue));
            release(&locks_lock);
            return lock->id;
        }
    }
    release(&locks_lock);
    return -1;
}

int
israeli_acquire(int lock_id)
{
    struct proc *p = myproc();

    acquire(&locks_lock);
    struct israeli_lock *lock = lock_lookup(lock_id);
    // check if the lock exists and is not already held by another process
    release(&locks_lock);
    if (lock == 0) { 
        return -1;
    }

    acquire(&lock->lk);

    if (!lock->locked) {
        //lock is not held by any process, we can acquire it
        lock->locked = 1;
        lock->owner = p;
    } else {
        //lock is held by another process, we need to enqueue ourselves and sleep.
        int next_index = (lock->start_index + lock->queue_size) % MAX_PROC_ACQUIRED;
        lock->waiting_queue[next_index] = p;
        lock->queue_size++;

        //sleeping until the lock is released and it's our turn to acquire it. we will be woken up when the lock is released and it's our turn to acquire it.
        while(lock->locked && lock->owner->pid != p->pid) {
            sleep(p, &lock->lk);
        }
    }

    release(&lock->lk);
    return 0;
}

int
israeli_release(int lock_id)
{
    struct proc *p = myproc();

    acquire(&locks_lock);
    struct israeli_lock *lock = lock_lookup(lock_id);
    release(&locks_lock);

    if (lock == 0) { 
        return -1;
    }
    acquire(&lock->lk);
    if (!lock->locked || lock->owner->pid != p->pid) {
        release(&lock->lk);
        return -1;
    }

    lock->locked = 0;
    lock->owner = 0;

    if (lock->queue_size > 0) {

        int precents = lcg_rand() % 100; //get the prcent
        if(precents < lock->favoritism){ //if the prcent is less than the favoritism, we will wake up a process from the same group
            int index = lock->start_index;
            int found = 0;
            for (int i = 0; i < lock->queue_size; i++) {
                struct proc *next_proc = lock->waiting_queue[index];
                if (next_proc->gid == p->gid) {
                    found = 1;
                    break;
                }
                index = (index + 1) % MAX_PROC_ACQUIRED;
            }
            if (found) {
                // Move the selected process to the front of the queue
                struct proc *selected_proc = lock->waiting_queue[index];
                for (int i = index; i != lock->start_index; i = (i - 1 + MAX_PROC_ACQUIRED) % MAX_PROC_ACQUIRED) {
                    lock->waiting_queue[i] = lock->waiting_queue[(i - 1 + MAX_PROC_ACQUIRED) % MAX_PROC_ACQUIRED];
                }
                lock->waiting_queue[lock->start_index] = selected_proc;
            }
        }



        int next_index = lock->start_index;
        struct proc *next_proc = lock->waiting_queue[next_index];
        lock->start_index = (lock->start_index + 1) % MAX_PROC_ACQUIRED;
        lock->queue_size--;
        lock->locked = 1; 
        lock->owner = next_proc;

        wakeup(next_proc);
    }

    release(&lock->lk);
    return 0;

}

int
israeli_destroy(int lock_id)
{
    acquire(&locks_lock);
    struct israeli_lock *lock = lock_lookup(lock_id);
    if (lock == 0 || lock->locked || lock->queue_size != 0) {
        release(&locks_lock);
        return -1;
    }

    lock->used = 0;
    lock->id = 0;
    lock->favoritism = 0;
    lock->owner = 0;
    lock->queue_size = 0;
    lock->start_index = 0;
    memset(lock->waiting_queue, 0, sizeof(lock->waiting_queue));
    release(&locks_lock);
    return 0;
}

uint64
sys_israeli_acquire(void)
{
    int lock_id;
    argint(0, &lock_id);
    return israeli_acquire(lock_id);
}

uint64
sys_israeli_release(void)
{
    int lock_id;
    argint(0, &lock_id);
    return israeli_release(lock_id);
}

uint64
sys_israeli_destroy(void)
{
    int lock_id;
    argint(0, &lock_id);
    return israeli_destroy(lock_id);
}

// אל תשכח להוסיף גם את create אם לא הוספת!
uint64
sys_israeli_create(void)
{
    int favoritism;
    argint(0, &favoritism);
    return israeli_create(favoritism);
}
