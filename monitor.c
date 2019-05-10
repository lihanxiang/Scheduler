#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<time.h>
#include<sys/times.h>
#include<sys/types.h>

int child = 0;

void print_time(char* s, clock_t time);

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
	        char* str;
		char* array[argc];
		signal(SIGTERM, term_handler);
		signal(SIGTSTP, tstp_handler);
		signal(SIGCONT, cont_handler);
		if (argc == 1){
			printf("Please input the command!\n");
			exit(0);
		} else {
			for (int i = 0; i < argc - 1; i++){
				array[i] = argv[i + 1];
			}
			array[argc] = NULL;
		}
		char* path = array[0];
		clock_t start, end;
		struct tms tmp;
		start = times(&tmp);
		child = fork();
		if (child == 0){
			execvp(path, array);
			exit(EXIT_SUCCESS);
		}
		wait(NULL);
		end = times(&tmp);
		if (child != 0){
			printf("Process %d : ", child);
			print_time("\ttime elapsed: ", end - start);
			print_time("\t\tuser time   : ", tmp.tms_cutime);
			print_time("\t\tsystem time : ", tmp.tms_cstime);
			exit(0);
		}
}

void print_time(char* s, clock_t time){
	long t = sysconf(_SC_CLK_TCK);
	printf("%s%6.4f\n", s, (double)time/t);
}
