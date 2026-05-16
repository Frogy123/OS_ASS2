#ifndef ISRAELI_LOCK_H
#define ISRAELI_LOCK_H




#define MAX_PROC_ACQUIRED 16
#define MAX_LOCKS 32

struct israeli_lock {
    int id;           // Unique identifier for the lock
    int used;         // Whether this table slot is in use
    uint locked;       // Is the lock held?
    struct spinlock lk; // Spinlock to protect this lock's data
    int favoritism;    // c. c% for group GID, and (100-c)% for others

    //process releated field
    struct proc *owner;        // The process that currently holds the lock

    //queue related fields
    struct proc *waiting_queue[MAX_PROC_ACQUIRED]; // Queue of processes waiting for the lock
    int queue_size;            // Number of processes in the waiting queue
    int start_index;           // Index of the first process in the waiting queue
};

#endif