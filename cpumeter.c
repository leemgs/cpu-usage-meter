/* @file  cpumeter.c
 * @brief The simple CPU usage meter to generate a tab-separated text file
 *        in order to generate a graph with MS office Excel software.
 * @note
 *  This file aims to avoid an unexpected CPU overhead while evaluating
    CPU load of the processes since 'top' command requires a big CPU usage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/times.h>

// Please re-configure the below variables for your evaluation
#define	MAX_PROCESS_COUNT	256	/* Max. 256 concurrent processes */
#define	DEFAULT_INTERVAL	500	/* 500ms */
#define	DEFAULT_SAMPLE_TIME	20	/* 20 seconds */

typedef struct {
	clock_t sample_time;
	unsigned long cpu_user, cpu_nice, cpu_system, cpu_idle;
	unsigned long cpu_iowait, cpu_irq, cpu_softirq, cpu_steal;
	int num_process;
	struct {
		int done;
		pid_t pid, pgid;
		char comm[16];
		unsigned long user, system;
		unsigned long cuser, csystem;
	} process_load[MAX_PROCESS_COUNT];
} load_record;

int g_record_count;
load_record *g_record_list;
int g_sample_count;

static const char *optstr = "to:";
static int t_flag = 0;
static int o_flag = 0;
static char output_filename[256] = "cpumeter.txt";

void write_cpu_load(FILE *fp)
{
	unsigned long delta;
	unsigned long last_user, last_nice, last_system, last_idle;
	unsigned long last_iowait, last_irq, last_softirq, last_steal;
	int i;

	last_user = g_record_list[0].cpu_user;
	last_nice = g_record_list[0].cpu_nice;
	last_system = g_record_list[0].cpu_system;
	last_idle = g_record_list[0].cpu_idle;
	last_iowait = g_record_list[0].cpu_iowait;
	last_irq = g_record_list[0].cpu_irq;
	last_softirq = g_record_list[0].cpu_softirq;
	last_steal = g_record_list[0].cpu_steal;

	//	Write CPU user time.
	fprintf(fp,"%-20s","CPU User");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_user - last_user;
		fprintf(fp,"\t%lu",delta);
		last_user = g_record_list[i].cpu_user;
		g_record_list[i].cpu_user = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU nice time.
	fprintf(fp,"%-20s","CPU Nice");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_nice - last_nice;
		fprintf(fp,"\t%lu",delta);
		last_nice = g_record_list[i].cpu_nice;
		g_record_list[i].cpu_nice = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU system time.
	fprintf(fp,"%-20s","CPU System");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_system - last_system;
		fprintf(fp,"\t%lu",delta);
		last_system = g_record_list[i].cpu_system;
		g_record_list[i].cpu_system = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU idle time.
	fprintf(fp,"%-20s","CPU Idle");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_idle - last_idle;
		fprintf(fp,"\t%lu",delta);
		last_idle = g_record_list[i].cpu_idle;
		g_record_list[i].cpu_idle = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU iowait time.
	fprintf(fp,"%-20s","CPU I/O Wait");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_iowait - last_iowait;
		fprintf(fp,"\t%lu",delta);
		last_iowait = g_record_list[i].cpu_iowait;
		g_record_list[i].cpu_iowait = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU IRQ time.
	fprintf(fp,"%-20s","CPU IRQ");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_irq - last_irq;
		fprintf(fp,"\t%lu",delta);
		last_irq = g_record_list[i].cpu_irq;
		g_record_list[i].cpu_irq = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU softirq time.
	fprintf(fp,"%-20s","CPU Softirq");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_softirq - last_softirq;
		fprintf(fp,"\t%lu",delta);
		last_softirq = g_record_list[i].cpu_softirq;
		g_record_list[i].cpu_softirq = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU steal time.
	fprintf(fp,"%-20s","CPU Steal");
	for (i = 0; i < g_record_count; i++) {
		delta = g_record_list[i].cpu_steal - last_steal;
		fprintf(fp,"\t%lu",delta);
		last_steal = g_record_list[i].cpu_steal;
		g_record_list[i].cpu_steal = delta;
	}
	fprintf(fp,"\n");
	//	Write CPU Total time.
	fprintf(fp,"%-20s","CPU Total");
	for (i = 0; i < g_record_count; i++) {
		delta =
			g_record_list[i].cpu_user +
			g_record_list[i].cpu_nice +
			g_record_list[i].cpu_system +
			g_record_list[i].cpu_idle +
			g_record_list[i].cpu_iowait +
			g_record_list[i].cpu_irq +
			g_record_list[i].cpu_softirq +
			g_record_list[i].cpu_steal;
		fprintf(fp,"\t%lu",delta);
	}
	fprintf(fp,"\n");
}

void write_process_load(FILE *fp,int record,int process)
{
	char dummy_string[256], *command_line;
	pid_t pid;
	unsigned long last_user, last_system, last_cuser, last_csystem;
	unsigned long workload;
	int i, j;

	command_line = g_record_list[record].process_load[process].comm;
	pid = g_record_list[record].process_load[process].pid;

	//	Write process name.
	sprintf(dummy_string,"%s_%d",command_line,pid);
	fprintf(fp,"%-20s",dummy_string);

	//	Insert tabs.
	for (i = 0; i < record; i++) fprintf(fp,"\t");

	//	Write this process' load info.
	last_user = g_record_list[record].process_load[process].user;
	last_system = g_record_list[record].process_load[process].system;
	last_cuser = g_record_list[record].process_load[process].cuser;
	last_csystem = g_record_list[record].process_load[process].csystem;
	for (i = record; i < g_record_count; i++) {
		j = (i == record) ? process : 0;
		for (; j < g_record_list[i].num_process; j++) {
			if (g_record_list[i].process_load[j].done) continue;
			if (g_record_list[i].process_load[j].pid != pid) continue;
			if (strcmp(g_record_list[i].process_load[j].comm, command_line) != 0)
				continue;
			workload = g_record_list[i].process_load[j].user
				- last_user
				+ g_record_list[i].process_load[j].system
				- last_system;
			fprintf(fp,"\t%lu",workload);
			g_record_list[i].process_load[j].done = 1;
			last_user = g_record_list[i].process_load[j].user;
			last_system = g_record_list[i].process_load[j].system;
			break;
		}
	}
	fprintf(fp,"\n");
}

void write_to_output_file(void)
{
//	struct timeval start_timeval, interval_timeval;
	clock_t start_timeval, interval_timeval;
	FILE *output_fp;
	int i, j;

	//	1. Open output file.
	output_fp = fopen(output_filename,"w");

	//	2. Write table header.
	fprintf(output_fp,"%-20s","Time");
	start_timeval = g_record_list[0].sample_time;
	for (i = 0; i < g_record_count; i++) {
		interval_timeval = g_record_list[i].sample_time - start_timeval;
		fprintf(output_fp,"\t%ld.%02ld",
			interval_timeval / sysconf(_SC_CLK_TCK),
			interval_timeval % sysconf(_SC_CLK_TCK));
	}
	fprintf(output_fp,"\n");

	//	3. Write global CPU load.
	write_cpu_load(output_fp);

	//	4. Start main loop to write each process load.
	for (i = 0; i < g_record_count; i++) {
		for (j = 0; j < g_record_list[i].num_process; j++) {
			if (g_record_list[i].process_load[j].done) continue;
			write_process_load( output_fp, i, j );
		}
	}

	fclose(output_fp);
}

void parse_cmdline(int argc,char *argv[])
{
	int c = 0;

	while (c != -1) {
		c = getopt(argc, argv, optstr);
		switch (c) {
		case -1: break;
		case 't':
			t_flag = 1;
			break;
		case 'o':
			o_flag = 1;
			strcpy(output_filename, optarg);
			break;
		default:
			printf("Unknown option -- %c\n",c);
		}
	}
}

int main(int argc,char *argv[])
{
	int i;
	FILE *fp;

	//	1. Initialize variables.
	g_record_count = 0;
	g_record_list = NULL;
	g_sample_count = DEFAULT_SAMPLE_TIME * 1000 / DEFAULT_INTERVAL;

	//	2. Parse command-line options.
	parse_cmdline(argc, argv);

	//	3. Allocate buffer.
	g_record_list =
		(load_record *) malloc(sizeof(load_record) * g_sample_count);
	if (!g_record_list) {
		printf("malloc() error.\n");
		return -1;
	}
	//	First, touch the memory buffer.
	memset(g_record_list, 0, sizeof(load_record) * g_sample_count);

	//	4. Start main loop.
	for (; g_record_count < g_sample_count; g_record_count++) {
		DIR *dir_proc, *dir_task;
		struct dirent *dir_entry, *dir_thread_entry;
		char string_buffer[2048], dummy_string[256], dummy_char;
		int pcount, dummy_int;
		long dummy_long;
		unsigned long long dummy_longlong;

#if 0
		if (gettimeofday(&g_record_list[g_record_count].sample_time,0)) {
			printf("gettimeofday() error.\n");
			break;
		}
#endif
		if ((g_record_list[g_record_count].sample_time = times(0)) == -1) {
			printf("times() error.\n");
			break;
		}

		//	4a. Open /proc/stat and get CPU usage time.
		fp = fopen("/proc/stat","r");
		if (!fp) {
			printf("fopen() for /proc/stat error.\n");
			break;
		}
		memset(string_buffer, 0, sizeof(string_buffer));
		if (!fgets(string_buffer, sizeof(string_buffer), fp)) {
			printf("fgets() for /proc/stat error.\n");
			break;
		}
		fclose(fp);
		if (sscanf(string_buffer,
			"cpu  %lu %lu %lu %lu %lu %lu %lu %lu",
			&g_record_list[g_record_count].cpu_user,
			&g_record_list[g_record_count].cpu_nice,
			&g_record_list[g_record_count].cpu_system,
			&g_record_list[g_record_count].cpu_idle,
			&g_record_list[g_record_count].cpu_iowait,
			&g_record_list[g_record_count].cpu_irq,
			&g_record_list[g_record_count].cpu_softirq,
			&g_record_list[g_record_count].cpu_steal) < 4) {
			printf("sscanf() for /proc/stat error.\n");
			break;
		}

		//	4b. Seek /proc/[PID] directories, open /proc/[PID]/stat,
		//	    and get process usage time.
		dir_proc = opendir("/proc");
		if (!dir_proc) {
			printf("opendir() for /proc error.\n");
			break;
		}
		for (pcount = 0; (dir_entry = readdir(dir_proc));) {
			//	Find [PID] dir entry.
			if (strcmp(".", dir_entry->d_name) == 0) continue;
			if (strcmp("..", dir_entry->d_name) == 0) continue;
			if (!isdigit(dir_entry->d_name[0])) continue;
			//	Open stat and read.
#if 0
            printf("dirent: /proc/%s\n",dir_entry->d_name);
            fflush(stdout);
#endif
			sprintf(dummy_string,"/proc/%s/stat",dir_entry->d_name);
			fp = fopen(dummy_string, "r");
			if (!fp) {
				printf("fopen() for /proc/[PID]/stat error.\n");
				break;
			}
			memset(string_buffer, 0, sizeof(string_buffer));
			if (!fgets(string_buffer, sizeof(string_buffer), fp)) {
				printf("fgets() for /proc/[PID]/stat error.\n");
				break;
			}
			fclose(fp);
	sscanf(string_buffer, "%d %s %c %d %d %d %d %d %lu %lu \
%lu %lu %lu %lu %lu %ld %ld %ld %ld %d \
%ld %llu %lu %ld %lu %lu %lu %lu %lu %lu \
%lu %lu %lu %lu %lu %lu %lu %d %d %lu \
%lu",
	&g_record_list[g_record_count].process_load[pcount].pid,
	g_record_list[g_record_count].process_load[pcount].comm,
	&dummy_char,
	&dummy_int,
	&g_record_list[g_record_count].process_load[pcount].pgid,
	&dummy_int,
	&dummy_int,
	&dummy_int,
	&dummy_long,
	&dummy_long,

	&dummy_long,
	&dummy_long,
	&dummy_long,
	&g_record_list[g_record_count].process_load[pcount].user,
	&g_record_list[g_record_count].process_load[pcount].system,
	&g_record_list[g_record_count].process_load[pcount].cuser,
	&g_record_list[g_record_count].process_load[pcount].csystem,
	&dummy_long,
	&dummy_long,
	&dummy_int,

	&dummy_long,
	&dummy_longlong,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,

	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_int,
	&dummy_int,
	&dummy_long,

	&dummy_long
			);
			//	Trim off parenthesises on both end sides.
			i = strlen(g_record_list[g_record_count].process_load[pcount].comm);
			if (i > 0)
				g_record_list[g_record_count].process_load[pcount].comm[i-1] = 0;
			memmove( g_record_list[g_record_count].process_load[pcount].comm, g_record_list[g_record_count].process_load[pcount].comm + 1, strlen(g_record_list[g_record_count].process_load[pcount].comm) );

			pcount++;
			if (!t_flag) continue;

			//	4c. Seek /proc/[PID]/task/[TID],
			//	    open /proc/[PID]/task/[TID]/stat,
			//	    and get thread usage time.
			sprintf(dummy_string,"/proc/%s/task",dir_entry->d_name);
			dir_task = opendir(dummy_string);
			if (!dir_task) {
#if 0
				printf("opendir() for /proc/%s/task error.\n", dir_entry->d_name);
#endif
				continue;
			}
			while ((dir_thread_entry = readdir(dir_task))) {
				//	Find /proc/[PID]/task/[TID] dir entry.
				if (strcmp(".", dir_thread_entry->d_name) == 0)
					continue;
				if (strcmp("..", dir_thread_entry->d_name) == 0)
					continue;
				if (strcmp(dir_entry->d_name,
					dir_thread_entry->d_name) == 0) continue;
				sprintf(dummy_string,"/proc/%s/task/%s/stat",
					dir_entry->d_name,
					dir_thread_entry->d_name);
				fp = fopen(dummy_string,"r");
				if (!fp) {
					printf("fopen() for [TID]/stat error.\n");
					break;
				}
				if (!fgets(string_buffer, sizeof(string_buffer), fp)) {
					printf("fgets() for [TID]/stat error.\n");
					break;
				}
				fclose(fp);
	sscanf(string_buffer, "%d %s %c %d %d %d %d %d %lu %lu \
%lu %lu %lu %lu %lu %ld %ld %ld %ld %d \
%ld %llu %lu %ld %lu %lu %lu %lu %lu %lu \
%lu %lu %lu %lu %lu %lu %lu %d %d %lu \
%lu",
	&g_record_list[g_record_count].process_load[pcount].pid,
	g_record_list[g_record_count].process_load[pcount].comm,
	&dummy_char,
	&dummy_int,
	&g_record_list[g_record_count].process_load[pcount].pgid,
	&dummy_int,
	&dummy_int,
	&dummy_int,
	&dummy_long,
	&dummy_long,

	&dummy_long,
	&dummy_long,
	&dummy_long,
	&g_record_list[g_record_count].process_load[pcount].user,
	&g_record_list[g_record_count].process_load[pcount].system,
	&g_record_list[g_record_count].process_load[pcount].cuser,
	&g_record_list[g_record_count].process_load[pcount].csystem,
	&dummy_long,
	&dummy_long,
	&dummy_int,

	&dummy_long,
	&dummy_longlong,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,

	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_long,
	&dummy_int,
	&dummy_int,
	&dummy_long,

	&dummy_long
					);
				pcount++;
			}
			closedir(dir_task);
		}
		closedir(dir_proc);

		g_record_list[g_record_count].num_process = pcount;

		//	4d. Sleep for a while.
		usleep(DEFAULT_INTERVAL * 1000);
	}

	//	5. Write data to output file.
	write_to_output_file();

	return 0;
}
