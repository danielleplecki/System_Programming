/**
 * Machine Problem: Password Cracker
 * CS 241 - Fall 2017
 */

#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <unistd.h>
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <semaphore.h>
#include <pthread.h>



char name[50][100];
char hash[50][100];
char half_password[50][100];
size_t total_thread_num;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t b;
long now_hacking;
char this_hack_finished;
size_t all_cannot_find;
char this_hacked_password[20];
int hash_count;

void* crack(void* thread_id) {
	int tid = (long)thread_id;
	struct crypt_data cdata;
	cdata.initialized = 0;
	long hacking_bef = -666;
	while (1) {
		pthread_mutex_lock(&m);
		long this_hacking = now_hacking;
		size_t num_threads = total_thread_num;
		pthread_mutex_unlock(&m);

		if (this_hacking == -1000) break;
		if (this_hacking == -10 || this_hacking == hacking_bef) continue;
		hacking_bef = this_hacking;
		char hash_str[100]; memset(hash_str, 0, 100);
		strcpy(hash_str, hash[this_hacking]);

		char pass[100]; memset(pass, 0, 100);
		strcpy(pass, half_password[this_hacking]);

		char user_name[100]; memset(user_name, 0, 100);
		strcpy(user_name, name[this_hacking]);

		// FIND NUM_DOTS
		char* dot = strstr(pass, ".");
		int pass_len = strlen(pass);
		char* fin = &pass[pass_len-1];
		int num_dots = fin - dot + 1;
		
		long start_idx = 0; long count = 0;
		getSubrange(num_dots, num_threads, tid+1, &start_idx, &count);
		setStringPosition(dot, start_idx);
		
		v2_print_thread_start(tid+1, user_name, start_idx, pass);

		long it = 0;
		int mid_break = 0; 
		int find_break = 0;
		for (it = 0; it < count; it++) {
			pthread_mutex_lock(&m);
			if (this_hack_finished) {
				hash_count += (it+1);
				mid_break = 1;
			}
			pthread_mutex_unlock(&m);
			if (mid_break) {
				break;
			}	
			// 以上是判断是不是别的thread已经hack成功了
			const char *hashed = crypt_r(pass, "xx", &cdata);
			if (strcmp(hashed, hash_str) == 0) {
				pthread_mutex_lock(&m); 
				this_hack_finished = 1;
				hash_count += (it+1);
				strcpy(this_hacked_password, pass);
				pthread_mutex_unlock(&m);
				find_break = 1;
				break;
			}	
			incrementString(pass);
		}
		if (find_break + mid_break == 0) {
			pthread_mutex_lock(&m);
			all_cannot_find += 1;
			hash_count += it;
			pthread_mutex_unlock(&m);
			v2_print_thread_result(tid+1, it, 2);
		}
		else if (find_break) {
			v2_print_thread_result(tid+1, it+1, 0);
		} else if (mid_break) {
			v2_print_thread_result(tid+1, it+1, 1);
		}

		pthread_barrier_wait(&b);

		pthread_barrier_wait(&b);

		
	}
	

	return NULL;
}

int start(size_t thread_count) {
	total_thread_num = thread_count; // a global num of threads

    pthread_barrier_init(&b, NULL ,thread_count + 1);


    ssize_t len = 0;
    size_t n = 0;
    char* buffer = NULL;
    long i = 0;
    now_hacking = -10;

    while ((len = getline(&buffer, &n, stdin)) != -1) {
		if (buffer[len-1] == '\n') {
			buffer[len-1] = '\0';
		}
		//strcpy(strings[i],buffer);
		char* first_space = strstr(buffer, " ");
		*first_space = '\0';
		strcpy(name[i], buffer); // copy name;
		char* second_space = strstr(first_space+1, " ");
		*second_space = '\0';
		strcpy(hash[i], first_space+1);
		strcpy(half_password[i], second_space+1);
		free(buffer);
		buffer = NULL;
		i++;
	}

	free(buffer);

	// j is 当前cracking的line
	pthread_t workers[thread_count];
	for (unsigned long j = 0; j < thread_count; j++) {
		pthread_create(&workers[j],NULL, &crack, (void*)j);
	}

	for (long j = 0; j < i; j++) {
		double normal_t = getTime();
		double cput_t = getCPUTime();
		pthread_mutex_lock(&m);
		now_hacking = j;
		hash_count = 0;
		all_cannot_find = 0;
		this_hack_finished = 0;
		memset(this_hacked_password, 0, 20);
		v2_print_start_user(name[j]);
		pthread_mutex_unlock(&m);
		
		while (1) {
			pthread_mutex_lock(&m);
			char finish = this_hack_finished;
			size_t this_cannot_find = all_cannot_find;
			pthread_mutex_unlock(&m);
			if (finish == 0 && this_cannot_find != thread_count) {
				continue;
			} else {
				break;
			}
		}
		
		pthread_barrier_wait(&b);

		pthread_mutex_lock(&m);
		v2_print_summary(name[j], this_hacked_password, hash_count, getTime()-normal_t, getCPUTime()-cput_t, all_cannot_find == thread_count);
		pthread_mutex_unlock(&m);
		
		pthread_barrier_wait(&b);
	}
	pthread_mutex_lock(&m);
	now_hacking = -1000;
	pthread_mutex_unlock(&m);

	for (unsigned long j = 0; j < thread_count; j++) {
		pthread_join(workers[j], NULL);
	}

	pthread_barrier_destroy(&b);
	pthread_mutex_destroy(&m);
    return 0; 
}
