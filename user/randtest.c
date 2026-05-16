#include "user/user.h"



int main(void) {
    lcg_srand(12345); // Set a seed for reproducibility
    for (int i = 0; i < 10; i++) {
        printf("%l\n", (uint64)lcg_rand());// Print 10 random numbers
    }
    return 0;
}