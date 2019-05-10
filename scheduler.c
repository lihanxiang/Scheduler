#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<limits.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/times.h>
#include<time.h>
#include<math.h>

// signal handler
void term_handler(int sig_num);
void tstp_handler(int sig_num);
void cont_handler(int sig_num);

// read lines from file
int read_line(FILE* fin);

// trim the lines into three parts
void read_file(FILE* fin, int* arrival_time, char* command[], int* duration);

// trim the command into an array
void get_command_array(char* command, char* command_array[]);

/* execute the whole job,
 * for FCFS and SJF
 */
int execute_whole_job(char* command, int duration);

/* execute a part of the job,
 * for RR
 */
void execute_part_job(char* command, int duration, int lines, int round_time);

// change the format from clock_t to double
double get_time(clock_t time);

// get the total elapsed time
int get_total_time(int* arrival_time, int* duration, int lines, int time_used);

// print the first part of Gantt Chart
void print_chart(int total_time);

// print the job-execute time
void print_time(int* arrival_time, int* duration, int lines, int* order);

// print the mixed time
void print_mixed_time(int* arrival_time, int total_time, int* time);

// FCFS policy
void fcfs(int* arrival_time, char* command[], int* duration, int lines);

// Shortest Job First policy
void sjf(int* arrival_time, char* command[], int* duration, int lines);

// Round Robin
void rr(int* arrival_time, char* command[], int* duration, int lines, int round_time);

// get the minimum value
int min(int i, int j);

/* get the shortest job
 * according to the elapsed time
 */
int get_shortest_job(int* arrival_time, int* duration, int lines, int time_used, int* executed);



int child = 0;

void main(int argc, char* argv[]){
	FILE *fin = fopen("job.txt", "r");
	int lines = read_line(fin);

	int arrival_time[lines];
	char* command[lines];
	int duration[lines];
	for (int i = 0; i < lines; i++){
		command[i] = (char*)malloc(255);
	}
	
	fin = fopen("job.txt", "r");
	char* token;
	char line[1024];
	int index = 0;
	read_file(fin, arrival_time, command, duration);
	sjf(arrival_time, command, duration, lines);
	//rr(arrival_time, command, duration, lines, 2);
	//fcfs(arrival_time, command, duration, lines);
}

void term_handler(int sig_num){
	if (child != 0){
		kill(child, SIGTERM);
	}
}

void tstp_handler(int sig_num){
	if (child != 0){
		kill(child, SIGTSTP);
	}
}

void cont_handler(int sig_num){
	if (child != 0){
		kill(child, SIGCONT);
	}
}

int read_line(FILE* fin){
	char line[1024];
	int lines = 0;
	while (fgets(line, 1024, fin) != NULL){
		lines++;
	}
	fclose(fin);
	return lines;
}

void read_file(FILE* fin, int* arrival_time, char* command[], int* duration){
	char* token;
	char line[1024];
	int index = 0;
	while (fgets(line, 1024, fin) != NULL){
		token = strtok(line, "\t");
		arrival_time[index] = atoi(token);
		token = strtok(NULL, "\t");
		int n = strlen(token);
		char* temp = (char*)malloc(n + 1);
		strncpy(temp, token, n + 1);
		command[index] = temp;
		token = strtok(NULL, "\t");
		duration[index++] = atoi(token);
	}
	fclose(fin);
}

void get_command_array(char* command, char* command_array[]){
	command_array[0] = "./monitor";
	int index = 1;
	char* token = strtok(command, " ");
	while (token != NULL){
		int n = strlen(token);
		char* temp = (char*)malloc(n + 1);
		strncpy(temp, token, n + 1);
		command_array[index++] = temp;
		token = strtok(NULL, " ");
	}
	command_array[index] = NULL;
}

double get_time(clock_t time){
	long t = sysconf(_SC_CLK_TCK);
	return (double)time / t;
}

int execute_whole_job(char* command, int duration){
	signal(SIGTERM, term_handler);
	signal(SIGTSTP, tstp_handler);
	signal(SIGCONT, cont_handler);
	signal(SIGALRM, term_handler);
	char* command_array[100];
	int j;
	for (j = 0; j < 100; j++){
		command_array[j] = (char*)malloc(100);
	}
	get_command_array(command, command_array);
	clock_t start, end;
	struct tms tmp;
	if (duration != -1 || duration != INT_MAX){
		alarm(duration);
	}
	start = times(&tmp);
	child = fork();
	if (child == 0){
		execvp(command_array[0], command_array);
		exit(EXIT_SUCCESS);
	}
	wait(NULL);
	end = times(&tmp);
	return (int)ceil(get_time(end - start));
}

void execute_part_job(char* command, int duration, int lines, int round_time){
	signal(SIGTERM, term_handler);
	signal(SIGTSTP, tstp_handler);
	signal(SIGCONT, cont_handler);
	signal(SIGALRM, cont_handler);
	char* command_array[100];
	int j;
	for (j = 0; j < 100; j++){
		command_array[j] = (char*)malloc(100);
	}
	get_command_array(command, command_array);
	clock_t start, end;
	struct tms tmp;
	int job[lines];
	for (j = 0; j < lines; j++){
		job[j] = fork();
		kill(job[j], SIGTSTP);
	}
	
	for (j = 0; j < lines * round_time; j++){
		start = times(&tmp);
		child = job[j];
		kill(job[j], SIGCONT);
		alarm(round_time);
		if (child == 0){
			execvp(command_array[0], command_array);
		}
		exit(EXIT_SUCCESS);
		wait(NULL);
		end = times(&tmp);
	}
	//return (int)ceil(get_time(end - start));
}

int get_total_time(int* arrival_time, int* duration, int lines, int time_used){
	int i;
	int total_time = arrival_time[0];
	for (i = 0; i < lines; i++){
		if (i > 0 && arrival_time[i] > time_used){
			total_time = arrival_time[i] + duration[i];
		} else {
			total_time += duration[i];
		}
		time_used = total_time;
	}
	return total_time;
}

int get_shortest_job(int* arrival_time, int* duration, int lines, int time_used, int* executed){
	int i;
	int min = INT_MAX;
	int index = 0;
	for (i = 0; i < lines; i++){
		if (executed[i] == 0 && arrival_time[i] <= time_used && duration[i] < min){
			min = duration[i];
			index = i;
		}
	}
	executed[index] = 1;
	return index;
}

void print_chart(int total_time){
	printf("total = %d\n", total_time);
	int i;
	printf("Gantt Chart\n");
	printf("===========\n");
	printf("  Time   | ");
	for (i = 0; i <= (total_time - 1) / 10; i++){
		printf("%d                   ", i);
	}
	printf("\n           ");
	for (i = 0; i < total_time; i++){
		printf("%d ", i % 10);
	}
	printf("\n---------+-");
	for (i = 0; i < total_time; i++){
		printf("- ");
	}
}

void print_time(int* arrival_time, int* duration, int lines, int* order){
	int i;
	int stop = arrival_time[0];
	for (i = 0; i < lines; i++){
		int job_num = order[i];
		printf("\n  Job %d  | ", job_num + 1);
		int j = 0;
		for (; j < arrival_time[job_num]; j++){
			printf("  ");
		}
		if (i == 0){
			for (j = arrival_time[job_num]; j < arrival_time[job_num] + duration[job_num]; j++){
				printf("# ");
			}
			stop = j;
		} else {
			int k = arrival_time[job_num];
			for (; k < stop; k++){
				printf(". ");
			}
			for (k = stop; k < stop + duration[job_num]; k++){
				printf("# ");
			}
			stop = k;
		}
	}	
}

void print_mixed_time(int* arrival_time, int total_time, int* time){
	int i;
	printf("\n  Mixed  | ");
	for (i = 0; i < arrival_time[0]; i++){
		printf("  ");
	}
	for (i = arrival_time[0]; i < total_time; i++){
		if (time[i] == 0){
			printf("  ");
		} else {
			printf("%d ", time[i]);
		}
	}
	printf("\n");
}

void fcfs(int* arrival_time, char* command[], int* duration, int lines){
	int i;
	int time_used = 0;
	int begin = 0;
	int order[lines];

	for (i = 0; i < lines; i++){
		if (arrival_time[i] > begin){
			sleep(arrival_time[i] - begin);
			begin = arrival_time[i];
		}
		order[i] = i;
		int real_time = execute_whole_job(command[i], duration[i]);
		if (real_time < duration[i] || duration[i] == -1){
			duration[i] = real_time;
		}
		begin += duration[i];

	}
	
	int total_time = get_total_time(arrival_time, duration, lines, time_used);
	int time[total_time];
	for (i = 0; i < total_time; i++){
		time[i] = 0;
	}
	
	begin = 0;
	for (i = 0; i < lines; i++){
		int j;
		if (arrival_time[i] > begin){
			begin = arrival_time[i];
		}
		for (j = begin; j < begin + duration[i]; j++){
			time[j] = order[i] + 1;
		}
		begin = j;
	}

	print_chart(total_time);
	print_time(arrival_time, duration, lines, order);
	print_mixed_time(arrival_time, total_time, time);
}

void rr(int* arrival_time, char* command[], int* duration, int lines, int round_time){
	int i = 0;	

	int total_time = arrival_time[0];
	int time_used = 0;
	for (; i < lines; i++){
		if (i > 0 && arrival_time[i] > time_used){
			total_time = arrival_time[i] + duration[i];
		} else {
			total_time += duration[i];
		}
		time_used = total_time;
	}		for (i = 0; i <= total_time / 10; i++){
		printf("%d                   ", i);
	}
	printf("\n           ");
	for (i = 0; i < total_time; i++){
		printf("%d ", i % 10);
	}
	printf("\n---------+-");
	for (i = 0; i < total_time; i++){
		printf("- ");
	}
	int stop = arrival_time[0];
	for (i = 0; i < lines; i++){
		printf("\n  Job %d  | ", i + 1);
		int j = 0;
		
	}
	printf("\n");
}

int min(int i, int j){
	if (i < j){
		return i;
	} else {
		return j;
	}
}

void sjf(int* arrival_time, char* command[], int* duration, int lines){
	
	int i;
	int time_used = 0;
	int begin = 0;
	int order[lines];
	int executed[lines];
	for (i = 0; i < lines; i++){
		executed[i] = 0;
		if (duration[i] == -1){
			duration[i] = INT_MAX - 1;
		}
	}

	for (i = 0; i < lines; i++){
		if (arrival_time[i] > begin){
			sleep(arrival_time[i] - begin);
			begin = arrival_time[i];
		}
		int next = get_shortest_job(arrival_time, duration, lines, begin, executed);
		order[i] = next;
		int real_time = execute_whole_job(command[next], duration[next]);
		if (real_time < duration[next]){
			duration[next] = real_time;
		}
		begin += duration[next];
	}

	int total_time = get_total_time(arrival_time, duration, lines, time_used);
	int time[total_time];
	for (i = 0; i < total_time; i++){
		time[i] = 0;
	}


	begin = 0;
	for (i = 0; i < lines; i++){
		int j;
		if (arrival_time[i] > begin){
			begin = arrival_time[i];
		}
		for (j = begin; j < begin + duration[order[i]]; j++){
			time[j] = order[i] + 1;
		}
		begin = j;
	}


	print_chart(total_time);
	print_time(arrival_time, duration, lines, order);
	print_mixed_time(arrival_time, total_time, time);



	
	
	/*int i = 0;
	int used[lines];
	int mini_cost;
	int index;
	int order[lines];
	int executed[lines];
	int begin = 0;
	int time_used = 0;
	for (i = 0; i < lines; i++){
		executed[i] = 0;
	}
	for (i = 0; i < lines; i++){
		if (arrival_time[i] > begin){
			sleep(arrival_time[i] - begin);
			begin = arrival_time[i];
		}
		int next = get_shortest_job(arrival_time, duration, lines, begin, executed);
		order[i] = next;
		int real_time = execute_whole_job(command[next], duration[next]);
		if (real_time < duration[i] || duration[i] == -1){
			duration[i] = real_time;
		}
		begin += duration[i];
	}

	int total_time = get_total_time(arrival_time, duration, lines, time_used);
	int time[total_time];
	for (i = 0; i < total_time; i++){
		time[i] = 0;
	}

	begin = 0;
	for (i = 0; i < lines; i++){
		int j;
		if (arrival_time[i] > begin){
			begin = arrival_time[i];
		}
		for (j = begin; j < begin + duration[i]; j++){
			time[j] = order[i] + 1;
		}
		begin = j;
	}


	print_chart(total_time);
	print_time(arrival_time, duration, lines, order);
	print_mixed_time(arrival_time, total_time, time);*/
}
