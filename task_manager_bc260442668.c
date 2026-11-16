/*
Name: Asad Niaz Malik
ID  : BC260442668
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_TASKS 3
#define ID_LEN    64


static int read_positive_int(const char *prompt)
{
    char line[100];
    char *end;
    long value;

    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            fprintf(stderr, "Input error.\n");
            exit(EXIT_FAILURE);
        }
        value = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\n')
            end++;
        if (end != line && *end == '\0' && value > 0 && value <= 1000000000L)
            return (int)value;
        printf("Invalid input. Please enter a positive integer.\n");
    }
}

int main(void)
{
    int pipe1[2];              
    int pipe2[2];              
    char student_id[ID_LEN];
    int times[NUM_TASKS];
    pid_t pid;
    int i;

    /* ---------- Input (parent) ---------- */
    do {
        printf("Enter Student ID: ");
        fflush(stdout);
        if (fgets(student_id, sizeof(student_id), stdin) == NULL) {
            fprintf(stderr, "Input error.\n");
            return EXIT_FAILURE;
        }
        student_id[strcspn(student_id, "\r\n")] = '\0';
    } while (strlen(student_id) == 0);

    for (i = 0; i < NUM_TASKS; i++) {
        char prompt[50];
        snprintf(prompt, sizeof(prompt), "Enter Task %d Execution Time: ", i + 1);
        times[i] = read_positive_int(prompt);
    }

    /* ---------- Create both pipes ---------- */
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* ---------- Create child ---------- */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ================= CHILD PROCESS ================= */
        int child_times[NUM_TASKS];
        int result[3];         
        int total = 0, longest = 1, max_time;

        close(pipe1[1]);        
        close(pipe2[0]);        

        if (read(pipe1[0], child_times, sizeof(child_times)) != sizeof(child_times)) {
            perror("child read");
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]);

        max_time = child_times[0];
        for (i = 0; i < NUM_TASKS; i++) {
            total += child_times[i];
            /* strict '>' keeps the smaller task number on a tie */
            if (child_times[i] > max_time) {
                max_time = child_times[i];
                longest = i + 1;
            }
        }

        result[0] = total;
        result[1] = longest;
        result[2] = max_time;

        if (write(pipe2[1], result, sizeof(result)) != sizeof(result)) {
            perror("child write");
            exit(EXIT_FAILURE);
        }
        close(pipe2[1]);
        exit(EXIT_SUCCESS);
    }

    /* ================= PARENT PROCESS ================= */
    {
        int result[3];
        int status;
        char filename[ID_LEN + 20];
        char report[1024];
        int len, fd;

        close(pipe1[0]);        
        close(pipe2[1]);        

        /* Send all three times to the child */
        if (write(pipe1[1], times, sizeof(times)) != sizeof(times)) {
            perror("parent write");
            return EXIT_FAILURE;
        }
        close(pipe1[1]);

        /* Receive results from the child */
        if (read(pipe2[0], result, sizeof(result)) != sizeof(result)) {
            perror("parent read");
            return EXIT_FAILURE;
        }
        close(pipe2[0]);

        waitpid(pid, &status, 0);

        /* Build the report text once; use it for screen and file */
        snprintf(filename, sizeof(filename), "%s_report.txt", student_id);

        len = snprintf(report, sizeof(report),
                       "--- Task Processing Report ---\n"
                       "Student ID: %s\n"
                       "Task 1: %d units\n"
                       "Task 2: %d units\n"
                       "Task 3: %d units\n"
                       "Total Execution Time: %d units\n"
                       "Longest Task: Task %d\n"
                       "Maximum Execution Time: %d units\n",
                       student_id, times[0], times[1], times[2],
                       result[0], result[1], result[2]);

        /* Display results */
        printf("\n%s", report);

        /* Save report using Linux file operations */
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            return EXIT_FAILURE;
        }
        if (write(fd, report, len) != len) {
            perror("write report");
            close(fd);
            return EXIT_FAILURE;
        }
        close(fd);

        printf("Report saved to: %s\n", filename);
    }

    return EXIT_SUCCESS;
}
