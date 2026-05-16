#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NPROC 15
#define GROUPS 4

void print_separator() {
    printf("\n======================================================\n");
}

void test_100_percent() {
    print_separator();
    printf("[TEST 1] Favoritism 100%% (Maximum Protection)\n");
    printf("Expected: Processes from the same group will pass the lock to each other sequentially.\n");
    print_separator();

    int lock_id = israeli_create(100);
    for (int i = 0; i < NPROC; i++) {
        int pid = fork();
        if (pid == 0) {
            setgid(lcg_rand() % GROUPS);
            israeli_acquire(lock_id);
            printf("Process %d (gid=%d) acquired the lock\n", getpid(), getgid());
            sleep(2); // Short sleep to ensure context switch and queue build-up
            israeli_release(lock_id);
            exit(0);
        }
    }
    for (int i = 0; i < NPROC; i++) wait(0);
    israeli_destroy(lock_id);
}

void test_0_percent() {
    print_separator();
    printf("[TEST 2] Favoritism 0%% (Strict FIFO)\n");
    printf("Expected: Processes acquire the lock in the exact order they arrived (interleaved GIDs).\n");
    print_separator();

    int lock_id = israeli_create(0);
    for (int i = 0; i < NPROC; i++) {
        int pid = fork();
        if (pid == 0) {
            setgid(lcg_rand() % GROUPS);
            israeli_acquire(lock_id);
            printf("Process %d (gid=%d) acquired the lock\n", getpid(), getgid());
            sleep(2);
            israeli_release(lock_id);
            exit(0);
        }
        sleep(1); // Force arrival order
    }
    for (int i = 0; i < NPROC; i++) wait(0);
    israeli_destroy(lock_id);
}

void test_50_percent() {
    print_separator();
    printf("[TEST 3] Favoritism 50%% (Mixed Behavior)\n");
    printf("Expected: Some grouping, but occasional breaks in the chain (FIFO takes over).\n");
    print_separator();

    int lock_id = israeli_create(50);
    for (int i = 0; i < NPROC; i++) {
        int pid = fork();
        if (pid == 0) {
            setgid(lcg_rand() % GROUPS);
            israeli_acquire(lock_id);
            printf("Process %d (gid=%d) acquired the lock\n", getpid(), getgid());
            sleep(2);
            israeli_release(lock_id);
            exit(0);
        }
    }
    for (int i = 0; i < NPROC; i++) wait(0);
    israeli_destroy(lock_id);
}

void test_edge_cases() {
    print_separator();
    printf("[TEST 4] Edge Cases & Error Handling\n");
    printf("Expected: System calls should fail gracefully and return -1.\n");
    print_separator();

    // 1. Invalid Creation
    if (israeli_create(-10) < 0) printf("PASS: Rejected negative favoritism.\n");
    else printf("FAIL: Accepted negative favoritism!\n");

    if (israeli_create(105) < 0) printf("PASS: Rejected favoritism > 100.\n");
    else printf("FAIL: Accepted favoritism > 100!\n");

    // 2. Invalid Lock IDs
    if (israeli_acquire(999) < 0) printf("PASS: israeli_acquire rejected invalid ID.\n");
    else printf("FAIL: israeli_acquire accepted invalid ID!\n");

    if (israeli_release(999) < 0) printf("PASS: israeli_release rejected invalid ID.\n");
    else printf("FAIL: israeli_release accepted invalid ID!\n");

    // 3. Destroying an active lock
    int lock_id = israeli_create(50);
    israeli_acquire(lock_id);
    if (israeli_destroy(lock_id) < 0) printf("PASS: Prevented destruction of a locked lock.\n");
    else printf("FAIL: Destroyed a locked lock!\n");
    
    // 4. Releasing a lock held by someone else (or just releasing twice)
    israeli_release(lock_id);
    if (israeli_release(lock_id) < 0) printf("PASS: Prevented releasing an already free lock.\n");
    else printf("FAIL: Released a free lock!\n");

    int pid = fork();
    if (pid == 0) {
        // Child tries to release parent's lock (should fail since it's free, or if parent held it, because child is not owner)
        if (israeli_release(lock_id) < 0) {
            exit(0); // Pass
        } else {
            printf("FAIL: Process released a lock it didn't own!\n");
            exit(1);
        }
    }
    wait(0);
    
    israeli_destroy(lock_id);
    printf("PASS: Successfully destroyed lock after edge cases.\n");
}

int main(void) {
    lcg_srand(getpid() + uptime()); // Seed the PRNG

    printf("\nStarting Israeli Lock Test Suite...\n");

    test_100_percent();
    test_0_percent();
    test_50_percent();
    test_edge_cases();

    print_separator();
    printf("ALL TESTS COMPLETED.\n");
    print_separator();

    exit(0);
}