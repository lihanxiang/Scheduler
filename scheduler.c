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

int read_line(FILE* fin);
void read_file(FILE* fin, int* arrival_time, char* command[], int* duration);
void get_command_array(char* command, char* command_array[]);
int execute_whole_job(char* command, int duration);
void execute_part_job(char* command, int duration, int lines, int round_time);
double get_time(clock_t time);
void print_chart(int total_time);

void fcfs(int* arrival_time, char* command[], int* duration, int lines);
void sjf(int* arrival_time, char* command[], int* duration, int lines);
void rr(int* arrival_time, char* command[], int* duration, int lines, int round_time);
int min(int i, int j);
int get_shortest_job(int* arrival_time, int* duration, int lines, int time_used, int* executed);

int child = 0;

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
	if (duration != -1){
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



void print_chart(int total_time){
	int i;
	printf("Gantt Chart\n");
	printf("===========\n");
	printf("  Time   | ");
	for (i = 0; i <= total_time / 10; i++){
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

void fcfs(int* arrival_time, char* command[], int* duration, int lines){
	int i;
	int total_time = arrival_time[0];
	int time_used = 0;
	int begin = 0;
	/* use the real time to replace
	 * the time in job.txt
	 */
	for (i = 0; i < lines; i++){
		if (arrival_time[i] > begin){
			sleep(arrival_time[i] - begin);
			begin = arrival_time[i];
		}
		int real_time = execute_whole_job(command[i], duration[i]);
		if (real_time < duration[i] || duration[i] == -1){
			duration[i] = real_time;
		}
		begin += duration[i];

	}
	for (i = 0; i < lines; i++){
		if (i > 0 && arrival_time[i] > time_used){
			total_time = arrival_time[i] + duration[i];
		} else {
			total_time += duration[i];
		}
		time_used = total_time;
	}
	int time[total_time];
	for (i = 0; i < total_time; i++){
		time[i] = 0;
	}

	/* calculate the mixed
	 * graph number
	 */
	begin = 0;
	for (i = 0; i < lines; i++){
		int j;
		if (arrival_time[i] > begin){
			begin = arrival_time[i];
		}
		for (j = begin; j < begin + duration[i]; j++){
			time[j] = i + 1;
		}
		begin = j;
	}
	print_chart(total_time);

	/*for (i = 0; i < lines; i++){
		int job_num = i + 1;
		printf("\n  Job %d  | ", job_num);
		int j;
		for (j = 0; j < arrival_time[i]; j++){
			printf("  ");
		}
		for (j = 0; j < total_time; j++){
			if (arrival_time[i] < j && time[j] < job_num){
				printf(". ");
			} else if (time[j] == job_num){
				printf("# ");
			}
		}
		printf("\n");
		
	}*/

	/* stop means the time when you finish this task, 
	 * it need to be used as the startpoint in next task
	 */

	int stop = arrival_time[0];
	for (i = 0; i < lines; i++){
		printf("\n  Job %d  | ", i + 1);
		int j = 0;
		for (; j < arrival_time[i]; j++){
			printf("  ");
		}
		if (i == 0){
			for (j = arrival_time[0]; j < arrival_time[0] + duration[0]; j++){
				printf("# ");
			}
			stop = j;
		} else {
			int k = arrival_time[i];
			for (; k < stop; k++){
				printf(". ");
			}
			for (k = stop; k < stop + duration[i]; k++){
				printf("# ");
			}
			stop = k;
		}
	}
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

void rr(int* arrival_time, char* command[], int* duration, int lines, int round_time){
	int i = 0;	
	printf("Gantt Chart\n");
	printf("===========\n");
	printf("  Time   | ");
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

void sjf(int* arrival_time, char* command[], int* duration, int lines){
	int i = 0;
	int used[lines];
	int mini_cost;
	int time_used = arrival_time[0];
	int index;
	int order[lines];
	int executed[lines];
	for (i = 0; i < lines; i++){
		executed[i] = 0;
	}
	for (i = 0; i < lines; i++){
		int next = get_shortest_job(arrival_time, duration, lines, time_used, executed);
		execute_whole_job(command[next], duration[next]);
		if (arrival_time[i] > time_used){
			time_used = arrival_time[i];
		}
		time_used += duration[i];
	}

        printf("Gantt Chart\n");
        printf("===========\n");
        printf("  Time   | ");
        int total_time = arrival_time[0];
	time_used = 0;
        for (; i < lines; i++){
		if (i > 0 && arrival_time[i] > time_used){
			total_time = arrival_time[i] + duration[i];
		} else {
			total_time += duration[i];
		}
		time_used = total_time;
	}              
       	for (i = 0; i <= total_time / 10; i++){
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
	for (i = 0; i < lines; i++){
		printf("%d\n", order[i] + 1);
	}
	int time[total_time];
	for (i = 0; i < total_time; i++){
		time[i] = 0;
	}

	int stop = arrival_time[0];
	for (i = 0; i < lines; i++){
		printf("\n  Job %d  | ", i + 1);
		int j = 0;
		for (; j < arrival_time[i]; j++){
			printf("  ");
		}
		if (i == 0){
			for (j = arrival_time[0]; j < arrival_time[0] + duration[0]; j++){
				printf("# ");
			}
			stop = j;
		} else {
			int k = arrival_time[i];
			for (; k < stop; k++){
				printf(". ");
			}
			for (k = stop; k < stop + duration[i]; k++){
				printf("# ");
			}
			stop = k;
		}
	}
	printf("\n  Mixed  | ");
	for (i = 0; i < arrival_time[0]; i++){
		printf("  i");
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
