#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "database.h"

Table* create_table(){
    //printf("inside create table()\n");
    Table* table = (Table*)malloc(sizeof(Table));
    //printf("Allocated memory to table\n");
    pthread_mutex_init(&table->dblock, NULL);
    //printf("Mutex initialized.\n");
    table->keys_count = 0;
    //printf("Keys - not yet. So keys count = 0\n");
    for(int i = 0; i < MAX_BUCKETS; i++){
        table->buckets[i] = NULL;
    }
    //printf("Initialised all buckets to NULL.\n");
    return table;
}

unsigned int hash(char* key){
    unsigned int hash = 5381;
    char c;
    while((c = *key++)){ 
        hash = hash * 33 + (int)c;
    }
    return hash;
}

void get_value(Table* table, char* key, char* value){
    int index = hash(key) % MAX_BUCKETS;
    strcpy(value, "\0");
    pthread_mutex_lock(&table->dblock);
    if(table->buckets[index] == NULL){
        //key not there cause ll itself is not there
        pthread_mutex_unlock(&table->dblock);
        return;
    }
    
    for(Node* cur = table->buckets[index]; cur; cur = cur->next){
        if(strcmp(key, cur->key) == 0){
            strcpy(value, cur->value);
            pthread_mutex_unlock(&table->dblock);
            return;
        }
    }

    pthread_mutex_unlock(&table->dblock);
    //not found
    return;
}

Node* create_node(char* key, char* value){
    Node* node = (Node*)malloc(sizeof(Node));
    strcpy(node->key, key);
    strcpy(node->value, value);
    node->next = NULL;
    return node;
}

void set_value(Table* table, char* key, char* value, int* status){
    int index = hash(key) % MAX_BUCKETS;
    pthread_mutex_lock(&table->dblock);
    if(table->buckets[index] == NULL){
        Node* node = create_node(key, value);
        table->buckets[index] = node;
        *status = 1;
        table->keys_count++;
        pthread_mutex_unlock(&table->dblock);
        return;
    } else {
        for(Node* cur = table->buckets[index]; cur; cur = cur->next){
            if(strcmp(cur->key, key) == 0){
                strcpy(cur->value, value);
                *status = 1;
                pthread_mutex_unlock(&table->dblock);
                return;
            } else if(cur->next == NULL){
                //it's not matching,and this is the last node, then insert new node.
                cur->next = create_node(key, value);
                table->keys_count++;
                *status = 1;
                pthread_mutex_unlock(&table->dblock);
                return;
            }
        }
        *status = 0;
        pthread_mutex_unlock(&table->dblock);
    }
}

void ping_response(char* response){
    char msg[] = "PONG";
    strcpy(response, msg);
    return;
}

void delete_key(Table* table, char* key, int* status){
    int index = hash(key) % MAX_BUCKETS;
    pthread_mutex_lock(&table->dblock);
    if(table->buckets[index] == NULL){
        status = 0;
        //couldn't delete 
        pthread_mutex_unlock(&table->dblock);
        return;
    }
    Node* prev = NULL;
    for(Node* cur = table->buckets[index]; cur; cur = cur->next){
        //for 1st node(head node) nothing is prev, so null.
        if(strcmp(cur->key, key) == 0){
            if(prev == NULL){
                //that means delete front
                table->buckets[index] = cur->next;
                free(cur);
                table->keys_count--;
                *status = 1;
                pthread_mutex_unlock(&table->dblock);
                return;
            } else if(prev != NULL && cur->next == NULL){
                //this means delete end
                prev->next = NULL;
                free(cur);
                table->keys_count--;
                *status = 1;
                pthread_mutex_unlock(&table->dblock);
                return;
            } else if(prev != NULL && cur->next != NULL){
                //somewhere in the middle
                prev->next = cur->next;
                table->keys_count--;
                free(cur);
                *status = 1;
                pthread_mutex_unlock(&table->dblock);
                return;
            }
        }
        prev = cur;
    }
    *status = 0;
    pthread_mutex_unlock(&table->dblock);
    //key not found
}

int exists(Table* table, char* key){
    int index = hash(key) % MAX_BUCKETS;
    pthread_mutex_lock(&table->dblock);
    if(table->buckets[index] == NULL){
        pthread_mutex_unlock(&table->dblock);
        return -1;
    } 
    for(Node* cur = table->buckets[index]; cur; cur = cur->next){
        if(strcmp(cur->key, key) == 0){
            pthread_mutex_unlock(&table->dblock);
            return 1; //found
        }
    }
    pthread_mutex_unlock(&table->dblock);
    return -1; //Not found
}

void get_keys(Table* table, char keys[10000][MAX_CHARS], int* num_keys){
    pthread_mutex_lock(&table->dblock);
    for(int i = 0; i < MAX_BUCKETS; i++){
        if(table->buckets[i] != NULL){
            for(Node* cur = table->buckets[i]; cur; cur = cur->next){
                strcpy(keys[(*num_keys)++], cur->key); //don't write *num_keys++, it results in pointer increment, not value
            }
        }
    }
    pthread_mutex_unlock(&table->dblock);
    return;
}

int count(Table* table){
    return table->keys_count;
}