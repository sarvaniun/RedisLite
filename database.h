#ifndef DATABASE_H
#define DATABASE_H

#define MAX_CHARS 100
#define MAX_BUCKETS 26

typedef struct node{
    char key[MAX_CHARS];
    char value[MAX_CHARS];
    struct node* next;
} Node;

typedef struct{
    Node* buckets[MAX_BUCKETS];
    pthread_mutex_t dblock;
    int keys_count;
} Table;

Table* create_table();
unsigned int hash(char* key);
void get_value(Table* table, char* key, char* value);
Node* create_node(char* key, char* value);
void set_value(Table* table, char* key, char* value, int* status);
void ping_response(char* response);
void delete_key(Table* table, char* key, int* status);
int exists(Table* table, char* key);
int count(Table* table);
void get_keys(Table* table, char keys[10000][MAX_CHARS], int* no_keys);

#endif