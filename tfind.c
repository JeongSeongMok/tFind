#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <error.h>
#define NANO 1000000000LL
long long diff;
struct timespec begin, end;

pthread_t w_tid[16];
pthread_t m_tid;

pthread_cond_t w_cond=PTHREAD_COND_INITIALIZER;
pthread_cond_t m_cond = PTHREAD_COND_INITIALIZER;
int num_keyword;
int busy_count = 0;
int num_of_th ;
int num_of_reg =0;
int num_of_line =0;

pthread_mutex_t w_mutex, m_mutex;

typedef struct _node{
    char path[512];
    struct _node *next;
}node;

typedef struct _queue{
    node *head, *tail;
    pthread_mutex_t h_lock, t_lock;
}queue;

queue task_q;


void print_result(){
     clock_gettime(CLOCK_MONOTONIC, &end);
	diff = NANO * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
	diff /= 1000000;
    printf("\n-------TERMINATED-------\nFOUND LINES : %d\nFOUND FILES : %d\nTIME : %lldms\n------------------------\n",num_of_line, num_of_reg,diff);
}
void q_init(){
    node *temp = malloc(sizeof(node));
    strcpy((*temp).path,"NULL");
    temp->next = NULL;
    task_q.head = temp;
    task_q.tail = temp;
    pthread_mutex_init(&task_q.h_lock,NULL);
    pthread_mutex_init(&task_q.t_lock,NULL);
}

void q_push(char *path){
    
    node *temp = malloc(sizeof(node));
    strcpy((*temp).path,path);
    temp->next = NULL;
    
    pthread_mutex_lock(&task_q.t_lock);
    
    task_q.tail->next = temp;
    task_q.tail = temp;
    pthread_mutex_unlock(&task_q.t_lock);
}

char* q_pop(){
    
    char *path = malloc(sizeof(char)*512);
    pthread_mutex_lock(&task_q.h_lock);
   
    node *old_head = task_q.head;
    node *new_head = old_head->next;
    
    if(new_head==NULL && strcmp(old_head->path,"NULL")==0){
       pthread_mutex_unlock(&task_q.h_lock);
        return "NULL";
    }
    
    if(strcmp(old_head->path,"NULL")==0){
        
        task_q.head = task_q.head->next;
        free(old_head);
    }
    
    old_head = task_q.head;
    new_head = old_head->next;
    if(new_head==NULL){
        strcpy(path,old_head->path);
        strcpy(old_head->path,"NULL");
        pthread_mutex_unlock(&task_q.h_lock);
        return path;
    }
    task_q.head = new_head;
    strcpy(path,old_head->path);
    free(old_head);
    pthread_mutex_unlock(&task_q.h_lock);
    return path;
    
}


bool q_empty(){
    if(task_q.head==NULL) return true;
    if(strcmp(task_q.head->path,"NULL")==0 && task_q.head->next == NULL) return true;
    return false;
}

void find(char **keyword, char* path) {
	FILE* pFile = NULL;
	pFile = fopen(path, "r");
	if (pFile != NULL) {
		char result_temp[512];
		int findcheck = 0;
		char linetemp[512];
		int linecount = 0;
		while (!feof(pFile))
		{
			linecount++;
			fgets(linetemp, sizeof(linetemp), pFile);
			for(int i=3;i<num_keyword;i++){
				char* found_check=NULL;
				found_check = strstr(linetemp, keyword[i]);
				if(found_check==NULL){
					break;
				}
                if(i==num_keyword-1){
                    num_of_line++;
                    printf("%s:%d:%s\n",path,linecount,linetemp);
                }
			}
		}
		fclose(pFile);
	}
	return ;
}

void *manager(void *arg){
    printf("-----------------[Manger Start : %ld]------------------\n",pthread_self());
    while(1){
        pthread_mutex_lock(&m_mutex);
        if(!q_empty()){ 
            pthread_cond_signal(&m_cond);
        }
        else{
            pthread_cond_wait(&w_cond, &m_mutex);
            if(busy_count==0 && q_empty() ) {
                for(int i=0; i<num_of_th;i++){
                    pthread_cancel(w_tid[i]);
                }
                printf("-----------------[Manger Finish : %ld]-----------------\n",pthread_self());
                print_result();
                
                exit(0);
                
            }
        }
        pthread_mutex_unlock(&m_mutex);
    }
    
    return 0x0;
}

void *worker(void* arg){
    char **keyword = (char**)arg;
    while(1){
        pthread_mutex_lock(&w_mutex);
        pthread_cond_wait(&m_cond, &w_mutex);
        printf("-----------------[Worker Start : %ld]------------------\n",pthread_self());
        if(!q_empty()) {
            char *temp_src = q_pop();
            busy_count++;
            struct stat buf;
            DIR *dir_info;
            dir_info = opendir(temp_src);
            struct dirent* dir_entry;
            int k=0;
            while ((dir_entry = readdir(dir_info)) != NULL) {
                if(dir_entry->d_name[0]=='.') continue;
                char temp_path[512];
                sprintf(temp_path, "%s/%s", temp_src, dir_entry->d_name);
                lstat(temp_path, &buf);
                if (S_ISDIR(buf.st_mode)) {
                    char push_temp[512];
                    sprintf(push_temp,"%s/%s",temp_src,dir_entry->d_name);
                    q_push(push_temp);

                }
                if (S_ISREG(buf.st_mode)) {
                    num_of_reg++;
                    find(keyword, temp_path);
                }
            }
        }
        pthread_cond_signal(&w_cond);
        busy_count--;
        pthread_mutex_unlock(&w_mutex);
        printf("-----------------[Worker Finish : %ld]-----------------\n",pthread_self());
    }
   
}



void handler(){
    print_result();
    kill(getpid(),SIGKILL);
}

int main(int argc, char *argv[]){
    if(atoi(argv[1])>16 || atoi(argv[1])<1) {
        printf("Number of thread should be in 1~16\n");
        return 0;
    }
    if(argc>11) {
        printf("Number of keyword should be lower than 9\n");
        return 0;
    }
    pthread_mutex_init(&w_mutex,NULL);
    pthread_mutex_init(&m_mutex,NULL);

    clock_gettime(CLOCK_MONOTONIC, &begin);
    signal(SIGINT, handler);
    num_of_th = atoi(argv[1]);
    num_keyword = argc;
    char *src =argv[2];
    q_init();
    q_push(src);
    
    pthread_cond_init(&w_cond,NULL);
    pthread_cond_init(&m_cond,NULL);
    pthread_create(&m_tid, NULL, manager, NULL);
     
    for(int i=0; i<num_of_th;i++){
        pthread_create(&w_tid[i], NULL, worker,argv);
    }
    pthread_join(m_tid, NULL);
    
    for(int i=0; i<num_of_th;i++){
        pthread_join(w_tid[i],NULL);
    }
    
    
    return 0;
}