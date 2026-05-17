#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h" // Required for O_CREATE, O_RDWR, etc.

#define NUM_GROUPS 5
#define RUNNERS_PER_GROUP 3
#define TOTAL_RUNNERS (NUM_GROUPS * RUNNERS_PER_GROUP)
#define TARGET_SCORE 30
#define SCORE_FILE "scores.dat"

// Helper function to read the current scores from the file
void read_scores(int scores[]) {
    int fd = open(SCORE_FILE, O_RDONLY);
    if (fd >= 0) {
        read(fd, scores, sizeof(int) * NUM_GROUPS);
        close(fd);
    }
}

// Helper function to write the updated scores to the file
void write_scores(int scores[]) {
    // Re-open with O_CREATE and O_WRONLY to overwrite the file
    int fd = open(SCORE_FILE, O_CREATE | O_WRONLY);
    if (fd >= 0) {
        write(fd, scores, sizeof(int) * NUM_GROUPS);
        close(fd);
    }
}

int main(int argc, char *argv[0]) {
    printf("\n*** Welcome to the xv6 Relay Race! ***\n");
    printf("Target score to win: %d\n\n", TARGET_SCORE);

    lcg_srand(getpid() + uptime());

    // Initialize the shared scores file with 0 for all teams
    int initial_scores[NUM_GROUPS] = {0};
    write_scores(initial_scores);

    // 1. Create a single Israeli lock with favoritism coefficient c (e.g., 50%)
    int baton_lock = israeli_create(20);
    if (baton_lock < 0) {
        printf("Error: Lock creation failed\n");
        exit(1);
    }

    // 2 & 3. Create multiple child processes and divide them into teams
    for (int g = 0; g < NUM_GROUPS; g++) {
        for (int r = 0; r < RUNNERS_PER_GROUP; r++) {
            
            int pid = fork();
            if (pid < 0) {
                printf("Error: Fork failed\n");
                exit(1);
            }
            
            if (pid == 0) { // Child process (Runner)
                // 4. Assign team identifier
                setgid(g); 
                int my_pid = getpid();
                int my_scores[NUM_GROUPS];

                // 6. Each runner repeatedly performs the loop
                while(1) {
                    // (a) Acquire the Israeli lock
                    israeli_acquire(baton_lock);
                    
                    // Read current shared scores
                    read_scores(my_scores);

                    // Check if any team has already won (Requirement 7)
                    int race_over = 0;
                    for(int i = 0; i < NUM_GROUPS; i++) {
                        if (my_scores[i] >= TARGET_SCORE) {
                            race_over = 1;
                            break;
                        }
                    }

                    if (race_over) {
                        // Release lock and exit if the race is already over
                        israeli_release(baton_lock);
                        exit(0);
                    }
                    
                    // (b) Increase the score of its team by 1
                    my_scores[g]++;
                    write_scores(my_scores); // Update the shared file
                    
                    // (c) Print short message
                    printf("Process ID: %d | Team ID: %d | Updated Score: %d\n", my_pid, g, my_scores[g]);
                    
                    // (d) Release the lock
                    israeli_release(baton_lock);
                    
                    // If my team just reached the target, I can finish
                    if (my_scores[g] >= TARGET_SCORE) {
                        printf("\n>>> TEAM %d WINS THE RACE! <<<\n", g);
                        exit(0);
                    }

                    // (e) Sleep briefly before trying to run again
                    sleep(2); 
                }
            }
        }
    }

    // Parent waits for all child processes to finish
    for (int i = 0; i < TOTAL_RUNNERS; i++) {
        wait(0);
    }

    // Clean up
    israeli_destroy(baton_lock);
    unlink(SCORE_FILE); // Delete the temp score file
    
    printf("\n*** Race Over! Track closed. ***\n");
    exit(0);
}