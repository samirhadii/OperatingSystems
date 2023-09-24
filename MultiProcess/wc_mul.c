/*************************************************
 * C program to count no of lines, words and 	 *
 * characters in a file
 * using multi process programming.              *
 *************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h> // needed for the kill command

#define MAX_PROC 100
#define MAX_FORK 1000

typedef struct count_t
{
	int linecount;
	int wordcount;
	int charcount;
} count_t;

// struct to keep track of processes and their word count information
typedef struct plist_t
{
	int index;
	int pid;
	long start; // this means size idk why i called it start
	long offset;
	int pipefd[2];
} plist_t;

count_t word_count(FILE *fp, long offset, long size)
{
	char ch;
	long rbytes = 0;

	count_t count;
	// Initialize counter variables
	count.linecount = 0;
	count.wordcount = 0;
	count.charcount = 0;

	printf("[pid %d] reading %ld bytes from offset %ld\n", getpid(), size, offset);

	if (fseek(fp, offset, SEEK_SET) < 0)
	{
		printf("[pid %d] fseek error!\n", getpid());
	}

	while ((ch = getc(fp)) != EOF && rbytes < size)
	{
		// Increment character count if NOT new line or space
		if (ch != ' ' && ch != '\n')
		{
			++count.charcount;
		}

		// Increment word count if new line or space character
		if (ch == ' ' || ch == '\n')
		{
			++count.wordcount;
		}

		// Increment line count if new line character
		if (ch == '\n')
		{
			++count.linecount;
		}
		rbytes++;
	}
	return count;
}

// child process function to better encapsulate logic
void child_process(int i, char **argv, plist_t *process_info, long start, long offset)
{
	// Open the file separately for each child process
	FILE *fp_child = fopen(argv[2], "r");
	if (fp_child == NULL)
	{
		printf("File open error: %s\n", argv[2]);
		exit(1);
	}

	count_t process_count = word_count(fp_child, start, offset);

	// Send count result through the pipe
	write(process_info[i].pipefd[1], &process_count, sizeof(count_t));
	close(process_info[i].pipefd[1]);
	fclose(fp_child);

	// Exit the child process normally
	exit(0);
}

int main(int argc, char **argv)
{
	long fsize;
	FILE *fp;
	int numJobs;
	plist_t process_info[MAX_PROC]; // array of type plist_t to keep track of processes and their information
	count_t total, count;
	int i, pid, status;
	int nFork = 0;

	if (argc < 3)
	{
		printf("usage: wc_mul <# of processes> <filname>\n");
		return 0;
	}

	numJobs = atoi(argv[1]);
	if (numJobs > MAX_PROC)
		numJobs = MAX_PROC;

	total.linecount = 0;
	total.wordcount = 0;
	total.charcount = 0;

	// Open file in read-only mode
	fp = fopen(argv[2], "r");

	if (fp == NULL)
	{
		printf("File open error: %s\n", argv[2]);
		printf("usage: wc <# of processes> <filname>\n");
		return 0;
	}

	fseek(fp, 0L, SEEK_END);
	fsize = ftell(fp); // use this fsize value to calculate file offset and size to read for each child

	fclose(fp);

	for (i = 0; i < numJobs; i++)
	{
		// set pipe;

		if (nFork++ > MAX_FORK)
			return 0;

		// create a pipe for child process
		if (pipe(process_info[i].pipefd) == -1)
		{
			perror("pipe");
			return 1;
		}

		pid = fork();
		if (pid < 0)
		{
			printf("Fork failed.\n");
		}
		else if (pid == 0)
		{
			// Child process
			close(process_info[i].pipefd[0]);							  // close the read end of current pipe
			long chunk_size = fsize / numJobs;							  // Calculate the size of each process's chunk
			long start = i * chunk_size;								  // Calculate the start position for this process
			long end = (i == numJobs - 1) ? fsize : (i + 1) * chunk_size; // Calculate the end position for this process
			long offset = end - start;
			process_info[i].start = start;
			process_info[i].offset = offset;
			// call child process function and store current pid in array
			process_info[i].index = i;
			child_process(i, argv, process_info, start, offset);
		}
		process_info[i].index = i;
	}

	// Parent
	// wait for all children
	// check their exit status
	// read the result of normalliy terminated child
	// close pipe
	for (i = 0; i < numJobs; i++)
	{
		int child_status;
		int child_pid = waitpid(-1, &child_status, 0);
		process_info[i].pid = child_pid;

		if (WIFEXITED(child_status))
		{
			printf("[pid %d] terminated normally with status %d.\n", child_pid, WEXITSTATUS(child_status));
		}
		else
		{
			printf("[pid %d] terminated with unknown status.\n", child_pid);
		}

		close(process_info[i].pipefd[1]); // Close the write end of the pipe

		count_t child_count;
		read(process_info[i].pipefd[0], &child_count, sizeof(count_t));
		close(process_info[i].pipefd[0]); // Close the read end of the pipe

		// Accumulate counts from child processes
		total.linecount += child_count.linecount;
		total.wordcount += child_count.wordcount;
		total.charcount += child_count.charcount;
	}

	printf("\n========== Final Results ================\n");
	printf("Total Lines : %d \n", total.linecount);
	printf("Total Words : %d \n", total.wordcount);
	printf("Total Characters : %d \n", total.charcount);
	printf("=========================================\n");

	return (0);
}
