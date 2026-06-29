#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "threadpool.h"
#include "database.h"

#define BUFSIZE 1000

Table* table = NULL;
int created = 0;

void* worker(void* arg){
    Threadpool* pool = (Threadpool*)arg;
    while(1){
        //always running until shutdown
        pthread_mutex_lock(&pool->mylock);
        while(pool->count == 0 && !pool->shutdown){
            pthread_cond_wait(&pool->task_available, &pool->mylock);
        }
        if(pool->shutdown && pool->count == 0){
            pthread_mutex_unlock(&pool->mylock);
            return NULL;
        }
        int conn_sock = pool->queue[pool->head];
        pool->head = (pool->head + 1) % QUEUE_SIZE;
        pool->count--;
        pthread_cond_signal(&pool->space_available);
        pthread_mutex_unlock(&pool->mylock);
        //u are worker, u have to do the work
        //client will send text commands, you have to parse them and give reply
        /*Handle client here*/
        char input[BUFSIZE];
        int recvlen;
        while((recvlen = read(conn_sock, input, BUFSIZE-1)) > 0){
            input[recvlen] = '\0';
            //printf("Received: '%s'\n", input);
            char* token = strtok(input, " \n"); //both \n and " " are the delimiters. either one of them
            char request_name[50];
            if(token == NULL) continue;
            strcpy(request_name, token);
            //printf("Command = '%s'\n", request_name);
            int arg_count = 0;
            char args[10][MAX_CHARS];
            while(token != NULL){
                token = strtok(NULL, " \n");
                if(token != NULL){
                    strcpy(args[arg_count], token);
                    arg_count++;
                }
            }

            char msg[1000];
            if(strcmp(request_name, "CREATE") == 0){
                //printf("Inside CREATE\n");
                if(arg_count == 0 && !created){
                    //printf("Table not created yet and arg_count == 0. Creating table.\n");
                    table = create_table();
                    strcpy(msg, "OK");
                    //printf("Table created\n");
                    created = 1;
                } else if(arg_count !=0){;
                    strcpy(msg, "Invalid command.\nFormat: CREATE");
                } else if(created){
                    strcpy(msg, "Table already exists.");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else if(strcmp(request_name, "PING") == 0){
                if(arg_count == 0){
                    strcpy(msg, "PONG");
                    ping_response(msg);
                } else if(arg_count != 0){
                    strcpy(msg, "Invalid command.\nFormat: PING");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else if(strcmp(request_name, "GET") == 0){
                if(arg_count == 1 && created){
                    char value[1000];
                    get_value(table, args[0], value);
                    if(value[0] != '\0'){
                        strcpy(msg, value);
                    } else {
                        strcpy(msg, "NULL"); //key doesnt exist
                    }
                } else if(arg_count !=1 && created){
                    strcpy(msg, "Invalid command.\nFormat: GET <key>");
                } else if(!created) {
                    strcpy(msg, "Table not created yet.\n To create, give command: CREATE");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));                
            } else if(strcmp(request_name, "DELETE") == 0){
                if(arg_count == 1 && created){
                    int status = 0;
                    delete_key(table, args[0], &status);
                    if(status){
                        strcpy(msg, "OK");
                    } else {
                        strcpy(msg, "ERR Key not found");
                    }
                } else if(arg_count != 1 && created) {
                    strcpy(msg, "Invalid command.\nFormat: DELETE <key>\n");
                } else if(!created) {
                    strcpy(msg, "Table not created yet.\n To create, give command: CREATE");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else if(strcmp(request_name, "SET") == 0){
                if(arg_count == 2 && created){
                    int status = 0;
                    set_value(table, args[0], args[1], &status);
                    if(status){
                        strcpy(msg, "OK");
                    } else {
                        strcpy(msg, "Something went wrong");
                    }
                } else if(arg_count != 2 && created) {
                    strcpy(msg, "Invalid command.\nFormat: SET <key> <value>");
                } else if(!created){
                    strcpy(msg, "Table not created yet.\n To create, give command: CREATE");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else if(strcmp(request_name, "EXISTS") == 0){
                if(arg_count == 1 && created){
                    if(exists(table, args[0]) > 0){
                        strcpy(msg, "YES\n");
                    } else {
                        strcpy(msg, "NO\n");
                    }
                } else if(arg_count != 1 && created){
                    strcpy(msg, "Invalid command.\nFormat: EXISTS <key>");
                } else if(!created){
                    strcpy(msg, "Table not created yet.\n To create, give command: CREATE");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else if(strcmp(request_name, "COUNT") == 0){
                if(arg_count == 0 && created){
                    snprintf(msg, sizeof(msg), "%d", count(table));
                } else if(arg_count !=0 && created){
                    strcpy(msg, "Invalid command.\nFormat: COUNT");
                } else if(!created) {
                    strcpy(msg, "Table not created yet.\n To create, give command: CREATE");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else if(strcmp(request_name, "KEYS") == 0){
                if(arg_count == 0 && created){
                    char keys[10000][MAX_CHARS];
                    int num_keys = 0;
                    get_keys(table, keys, &num_keys);
                    if(num_keys != 0){
                        for(int i = 0; i < num_keys; i++){
                            printf("Keys[%d] = %s\n", i, keys[i]);
                            strcat(msg, keys[i]);
                            strcat(msg, "\n");
                        }
                    } else {
                        strcpy(msg, "No keys in table.");
                    }
                } else if(arg_count != 0 && created){
                    strcpy(msg, "Invalid command.\nFormat: KEYS");
                } else if(!created){
                    strcpy(msg, "Table not created yet.\n To create, give command: CREATE");
                }
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            } else {
                strcpy(msg, "Unknown Command.\n");
                write(conn_sock, msg, strlen(msg));
                memset(msg, 0, sizeof(msg));
            }
        }
        if(recvlen < 0){
            perror("Reading command from client\n");
            continue;
        }
        //printf("Finished processing command.\n");
    }
}

Threadpool* pool_create(int nthreads){
    Threadpool* pool = (Threadpool*)calloc(1, sizeof(Threadpool));
    pool->nthreads = nthreads;
    pthread_cond_init(&pool->space_available, NULL);
    pthread_cond_init(&pool->task_available, NULL);
    pthread_mutex_init(&pool->mylock, NULL);
    for(int i = 0; i < nthreads; i++){
        pthread_create(&pool->threads[i], NULL, worker, pool);
    }
    return pool;
}

void pool_submit(Threadpool* pool, int conn_sock){
    pthread_mutex_lock(&pool->mylock);
    while(pool->count == QUEUE_SIZE && !pool->shutdown){
        pthread_cond_wait(&pool->space_available, &pool->mylock);
    }
    if(pool->shutdown){
        pthread_mutex_unlock(&pool->mylock);
        return;
    }
    pool->queue[pool->tail] = conn_sock;
    pool->tail = (pool->tail + 1) % QUEUE_SIZE;
    pool->count++;
    pthread_cond_signal(&pool->task_available);
    pthread_mutex_unlock(&pool->mylock);
}

void pool_destroy(Threadpool* pool){
    pthread_mutex_lock(&pool->mylock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->space_available);
    pthread_cond_broadcast(&pool->task_available);
    pthread_mutex_unlock(&pool->mylock);
    //now all threads awake, they'll compete for mutex and terminate
    for(int i = 0; i < pool->nthreads; i++){
        pthread_join(pool->threads[i], NULL);
    }
    //all threads terminated
    pthread_cond_destroy(&pool->space_available);
    pthread_cond_destroy(&pool->task_available);
    pthread_mutex_destroy(&pool->mylock);
    free(pool);
    return;
}