#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

static uint lcg_state = 1; // default seed
static struct spinlock state_lock;

void lcg_srand(uint seed)
{
  acquire(&state_lock); //acquire the lock before modifying the state
  lcg_state = seed;
  release(&state_lock); //release the lock after modifying the state
}

uint lcg_rand(){
  acquire(&state_lock); //acquire the lock before modifying the state
  lcg_state = (lcg_state * 1664525 + 1013904223); //calculate the next random number using the linear congruential generator formula. notice that the modulus is 2^32, so we can just let it overflow and it will wrap around correctly.
  uint result = lcg_state;
  release(&state_lock); //release the lock after modifying the state
  return result;
}

void sys_lcg_srand(void){
  int seed;
  argint(0, &seed); //get the seed from the system call argument
  lcg_srand(seed); //call the lcg_srand function to set the seed
}

uint sys_lcg_rand(void){
  return lcg_rand(); //call the lcg_rand function and return the result to the user program
}