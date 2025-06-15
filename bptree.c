#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define ORDER 4
#define MAX_LINE_LEN 100
#define HEAP_SIZE 7500 
#define SECONDARY_STORAGE_SIZE 2500

//Linked List Node
typedef struct UniversityNode {
    char university_name[MAX_LINE_LEN];
    float score;
    struct UniversityNode* next;
} UniversityNode;

//B+Tree Node
typedef struct Node {
    bool is_leaf;
    int num_keys;
    char keys[ORDER - 1][MAX_LINE_LEN];
    void* pointers[ORDER];
    struct Node* parent;
    struct Node* next;
} Node;

Node* root = NULL;
Node* first_leaf = NULL;


typedef struct Record {
    char uni_name[MAX_LINE_LEN];
    char dept_name[MAX_LINE_LEN];
    float score;
} Record;

typedef struct MinHeapNode {
    Record record;
    int file_index;
} MinHeapNode;

int total_record = 0;
long long split_count = 0;
long long node_allocations = 0;
long long uni_node_allocations = 0;

void search_department_by_rank(const char* dept_name, int rank);


void reset_metrics();
int calculate_tree_height();
double calculate_memory_usage();
void insert(const char* dept_name, const char* uni_name, float score);
void free_tree(Node* node);
void load_data_from_csv(const char* filename);
Node* create_node(bool is_leaf);
UniversityNode* create_university(const char* name, float score);
void insert_into_sorted_list(UniversityNode** head, UniversityNode* new_uni);
void run_sequential_insertion();
void run_bulk_loading();
int create_sorted_runs_replacement_selection(const char* input_filename);
void merge_runs(int num_runs);
void build_tree_from_sorted_file(const char* sorted_filename);
Node* build_parent_level(Node** children, int count, const char (*keys)[MAX_LINE_LEN]);
int compare_records(const void* a, const void* b);
void swap_heap_nodes(MinHeapNode* a, MinHeapNode* b);
void min_heapify_merge(MinHeapNode heap_arr[], int size, int i);
void min_heapify_replacement(Record heap[], int size, int i);
void swap_records(Record* a, Record* b);
Node* find_leaf(Node* current_node, const char* dept_name);
double calculate_average_seek_time(const char* filename);


int main() {
    int choice;

    printf("Please choose a loading option:\n");
    printf("1 - Sequential Insertion\n");
    printf("2 - Bulk Loading (with external merge sort)\n>> ");
    scanf("%d", &choice);

    if (choice == 1) {
        printf("Running Sequential Insertion...\n");
        run_sequential_insertion();
        printf("Sequantial insertion completed.\n");
    } else if (choice == 2) {
        printf("Running Bulk Loading...\n");
        run_bulk_loading();
        printf("Bulk loading completed.\n");
    } else {
        printf("Invalid choice.\n");
        return 1;
    }

    int height = calculate_tree_height();
    double memory_usage = calculate_memory_usage();
    double time_taken = calculate_average_seek_time("yok_atlas.csv");
    printf("Total records: %d\n",total_record);
    
    choice = 0;
    while(choice != 3) {
        printf("Please choose an action:\n");
        printf("1 - Print Metrics\n");
        printf("2 - Search\n");
        printf("3 - Exit\n>> ");
        scanf("%d", &choice);

        if(choice == 1) {
            printf("Number of splits: %lld\n", split_count);
            printf("Memory usage: %.4f MB\n", memory_usage);
            printf("Tree height: %d\n", height);
            printf("Average seek time: %.8f sec\n\n", time_taken);
        }
        else if(choice == 2) {
            char dept_name[MAX_LINE_LEN];
            int rank;

            getchar();

            printf("Please enter the department name to search:\n>> ");
            fgets(dept_name, sizeof(dept_name), stdin);
            dept_name[strcspn(dept_name, "\n")] = 0;

            printf("Please enter the university rank in that department:\n>> ");
            scanf("%d", &rank);

            search_department_by_rank(dept_name, rank);
        }

    }
    
    free_tree(root);
    printf("\nMemory cleaned. Program terminated.\n");
    return 0;
}


//B+Tree Functions

Node* create_node(bool is_leaf) {
    node_allocations++;
    Node* new_node = (Node*)calloc(1, sizeof(Node));
    if (!new_node) { perror("Node allocation failed"); exit(1); }
    new_node->is_leaf = is_leaf;
    return new_node;
}

UniversityNode* create_university(const char* name, float score) {
    uni_node_allocations++;
    UniversityNode* new_uni = (UniversityNode*)malloc(sizeof(UniversityNode));
    if (!new_uni) { perror("UniversityNode allocation failed"); exit(1); }
    strncpy(new_uni->university_name, name, MAX_LINE_LEN - 1);
    new_uni->university_name[MAX_LINE_LEN - 1] = '\0';
    new_uni->score = score;
    new_uni->next = NULL;
    return new_uni;
}

void insert_into_parent(Node* old_node, const char* key, Node* new_node);

void insert_into_leaf(Node* leaf, const char* dept_name, const char* uni_name, float score) {
    int i = 0;
    while (i < leaf->num_keys && strcmp(leaf->keys[i], dept_name) != 0) i++;
    if (i < leaf->num_keys) {
        insert_into_sorted_list((UniversityNode**)&leaf->pointers[i], create_university(uni_name, score));
        return;
    }
    int insertion_point = 0;
    while (insertion_point < leaf->num_keys && strcmp(leaf->keys[insertion_point], dept_name) < 0) insertion_point++;
    for (int j = leaf->num_keys; j > insertion_point; j--) {
        strcpy(leaf->keys[j], leaf->keys[j - 1]);
        leaf->pointers[j] = leaf->pointers[j - 1];
    }
    strcpy(leaf->keys[insertion_point], dept_name);
    UniversityNode* new_list_head = NULL;
    insert_into_sorted_list(&new_list_head, create_university(uni_name, score));
    leaf->pointers[insertion_point] = new_list_head;
    leaf->num_keys++;
    if (leaf->num_keys == ORDER - 1) {
        split_count++;
        Node* new_leaf = create_node(true);
        new_leaf->parent = leaf->parent;
        int split_point = (ORDER - 1) / 2;
        new_leaf->num_keys = (ORDER - 1) - split_point;
        leaf->num_keys = split_point;
        for (int j = 0; j < new_leaf->num_keys; j++) {
            strcpy(new_leaf->keys[j], leaf->keys[j + split_point]);
            new_leaf->pointers[j] = leaf->pointers[j + split_point];
        }
        new_leaf->next = leaf->next;
        leaf->next = new_leaf;
        insert_into_parent(leaf, new_leaf->keys[0], new_leaf);
    }
}

void insert_into_parent(Node* old_node, const char* key, Node* new_node) {
    if (old_node->parent == NULL) {
        root = create_node(false);
        strcpy(root->keys[0], key);
        root->pointers[0] = old_node;
        root->pointers[1] = new_node;
        root->num_keys = 1;
        old_node->parent = root;
        new_node->parent = root;
        return;
    }
    Node* parent = old_node->parent;
    int i = 0;
    while (i < parent->num_keys && strcmp(parent->keys[i], key) < 0) i++;
    for (int j = parent->num_keys; j > i; j--) {
        strcpy(parent->keys[j], parent->keys[j - 1]);
        parent->pointers[j + 1] = parent->pointers[j];
    }
    strcpy(parent->keys[i], key);
    parent->pointers[i + 1] = new_node;
    new_node->parent = parent;
    parent->num_keys++;
    if (parent->num_keys == ORDER - 1) {
        split_count++;
        Node* new_internal = create_node(false);
        new_internal->parent = parent->parent;
        int split_point = (ORDER - 1) / 2;
        char key_to_promote[MAX_LINE_LEN];
        strcpy(key_to_promote, parent->keys[split_point]);
        new_internal->num_keys = (ORDER - 1) - (split_point + 1);
        for (int j = 0; j < new_internal->num_keys; j++) {
            strcpy(new_internal->keys[j], parent->keys[j + split_point + 1]);
            new_internal->pointers[j] = parent->pointers[j + split_point + 1];
            ((Node*)new_internal->pointers[j])->parent = new_internal;
        }
        new_internal->pointers[new_internal->num_keys] = parent->pointers[ORDER-1];
        if (new_internal->pointers[new_internal->num_keys] != NULL)
            ((Node*)new_internal->pointers[new_internal->num_keys])->parent = new_internal;
        parent->num_keys = split_point;
        insert_into_parent(parent, key_to_promote, new_internal);
    }
}

void insert(const char* dept_name, const char* uni_name, float score){
     if (root == NULL) {
        root = create_node(true);
        first_leaf = root;
    }
    Node* leaf = find_leaf(root, dept_name);
    insert_into_leaf(leaf, dept_name, uni_name, score);
}

void free_tree(Node* node){
    if (node == NULL) return;
    if (node->is_leaf) {
        for (int i = 0; i < node->num_keys; i++) {
            UniversityNode* head = (UniversityNode*)node->pointers[i];
            UniversityNode* tmp;
            while (head != NULL) { tmp = head; head = head->next; free(tmp); }
        }
    } else {
        for (int i = 0; i <= node->num_keys; i++) free_tree(node->pointers[i]);
    }
    free(node);
}

void load_data_from_csv(const char* filename){
    FILE* file = fopen(filename, "r");
    if (!file) { perror("Could not open file"); return; }
    char line[512];
    fgets(line, sizeof(line), file);
    while (fgets(line, sizeof(line), file)) {
        char id[MAX_LINE_LEN], uni_name[MAX_LINE_LEN], dept_name[MAX_LINE_LEN];
        float score = 0.0;
        sscanf(line, "%[^,],%[^,],%[^,],%f", id, uni_name, dept_name, &score);
        if (strlen(dept_name) > 0 && strlen(uni_name) > 0) {
            insert(dept_name, uni_name, score);
        }
    }
    fclose(file);
}

void insert_into_sorted_list(UniversityNode** head, UniversityNode* new_uni){
    if (*head == NULL || (*head)->score < new_uni->score) {
        new_uni->next = *head;
        *head = new_uni;
        return;
    }
    UniversityNode* current = *head;
    while (current->next != NULL && current->next->score >= new_uni->score) {
        current = current->next;
    }
    new_uni->next = current->next;
    current->next = new_uni;
}

Node* find_leaf(Node* current_node, const char* dept_name){
    if (current_node == NULL) return NULL;
    while (!current_node->is_leaf) {
        int i = 0;
        while (i < current_node->num_keys && strcmp(dept_name, current_node->keys[i]) >= 0) {
            i++;
        }
        current_node = (Node*)current_node->pointers[i];
    }
    return current_node;
}

void run_sequential_insertion() {
    load_data_from_csv("yok_atlas.csv");
}

void run_bulk_loading() {
    int num_runs = create_sorted_runs_replacement_selection("yok_atlas.csv");
    if (num_runs <= 0) { printf("Error: Could not create sorted runs.\n"); return; }
    
    merge_runs(num_runs);
    
    build_tree_from_sorted_file("sorted_data.csv");

    for (int i = 0; i < num_runs; i++) {
        char fname[20];
        sprintf(fname, "run_%d.tmp", i);
        remove(fname);
    }
    remove("sorted_data.csv");
}

//Bulk Loading Functions

int compare_records(const void* a, const void* b){
    Record* rec1 = (Record*)a;
    Record* rec2 = (Record*)b;
    int dept_cmp = strcmp(rec1->dept_name, rec2->dept_name);
    if (dept_cmp != 0) return dept_cmp;
    else if (dept_cmp == 0)
        return 0;
    return rec2->score - rec1->score;
}
int create_sorted_runs_replacement_selection(const char* input_filename){
    FILE* in = fopen(input_filename, "r");
    if (!in) { perror("Could not open input file"); return -1; }
    Record* primary_heap = (Record*)malloc(HEAP_SIZE * sizeof(Record));
    Record* secondary_storage = (Record*)malloc(SECONDARY_STORAGE_SIZE * sizeof(Record));
    if (!primary_heap || !secondary_storage) {
        perror("Memory allocation error"); fclose(in); return -1;
    }
    char line[512];
    fgets(line, sizeof(line), in);
    int run_count = 0;
    bool more_input = true;
    int current_heap_size = 0;
    for (int i = 0; i < HEAP_SIZE && more_input; i++) {
        if (!fgets(line, sizeof(line), in)) {
            more_input = false;
            break;
        }
        char id[MAX_LINE_LEN];
        sscanf(line, "%[^,],%[^,],%[^,],%f", id, primary_heap[i].uni_name, primary_heap[i].dept_name, &primary_heap[i].score);
        current_heap_size++;
    }
    while (current_heap_size > 0) {
        char out_fname[20];
        sprintf(out_fname, "run_%d.tmp", run_count);
        FILE* out = fopen(out_fname, "w");
        if (!out) { perror("Could not open temp file"); free(primary_heap); free(secondary_storage); fclose(in); return -1; }
        for (int i = (current_heap_size / 2) - 1; i >= 0; i--) {
            min_heapify_replacement(primary_heap, current_heap_size, i);
        }
        int secondary_count = 0;
        while (current_heap_size > 0) {
            Record min_record = primary_heap[0];
            fprintf(out, "%s,%s,%f\n", min_record.uni_name, min_record.dept_name, min_record.score);
            Record new_record;
            bool got_new_record = false;
            if (more_input) {
                if (fgets(line, sizeof(line), in)) {
                    char id[MAX_LINE_LEN];
                    sscanf(line, "%[^,],%[^,],%[^,],%f", id, new_record.uni_name, new_record.dept_name, &new_record.score);
                    got_new_record = true;
                } else { more_input = false; }
            }
            if (got_new_record) {
                if (compare_records(&new_record, &min_record) >= 0) {
                    primary_heap[0] = new_record;
                } else {
                    if (secondary_count < SECONDARY_STORAGE_SIZE) {
                        secondary_storage[secondary_count++] = new_record;
                    }
                    primary_heap[0] = primary_heap[current_heap_size - 1];
                    current_heap_size--;
                }
            } else {
                primary_heap[0] = primary_heap[current_heap_size - 1];
                current_heap_size--;
            }
            if (current_heap_size > 0) {
                min_heapify_replacement(primary_heap, current_heap_size, 0);
            }
        }
        fclose(out);
        run_count++;
        current_heap_size = secondary_count;
        for (int i = 0; i < secondary_count; i++) {
            primary_heap[i] = secondary_storage[i];
        }
    }
    free(primary_heap);
    free(secondary_storage);
    fclose(in);
    return run_count;
}
void merge_runs(int num_runs){
    if (num_runs <= 0) return;
    FILE* in_files[num_runs];
    for (int i = 0; i < num_runs; i++) {
        char fname[20];
        sprintf(fname, "run_%d.tmp", i);
        in_files[i] = fopen(fname, "r");
        if (!in_files[i]) { perror("Could not open run file"); return; }
    }
    FILE* out_file = fopen("sorted_data.csv", "w");
    if (!out_file) { perror("Could not open output file"); return; }
    MinHeapNode* heap_arr = (MinHeapNode*)malloc(num_runs * sizeof(MinHeapNode));
    if(!heap_arr) { perror("Heap memory allocation error"); return; }
    int heap_size = 0;
    for (int i = 0; i < num_runs; i++) {
        char line[512];
        if (fgets(line, sizeof(line), in_files[i])) {
            sscanf(line, "%[^,],%[^,],%f", heap_arr[heap_size].record.uni_name, heap_arr[heap_size].record.dept_name, &heap_arr[heap_size].record.score);
            heap_arr[heap_size].file_index = i;
            heap_size++;
        }
    }
    for (int j = (heap_size / 2) - 1; j >= 0; j--) {
        min_heapify_merge(heap_arr, heap_size, j);
    }
    while(heap_size > 0) {
        MinHeapNode root_node = heap_arr[0];
        fprintf(out_file, "%s,%s,%f\n", root_node.record.uni_name, root_node.record.dept_name, root_node.record.score);
        char line[512];
        if(fgets(line, sizeof(line), in_files[root_node.file_index])) {
            sscanf(line, "%[^,],%[^,],%f", heap_arr[0].record.uni_name, heap_arr[0].record.dept_name, &heap_arr[0].record.score);
        } else {
            heap_arr[0] = heap_arr[heap_size - 1];
            heap_size--;
        }
        if (heap_size > 0) {
            min_heapify_merge(heap_arr, heap_size, 0);
        }
    }
    free(heap_arr);
    for (int j = 0; j < num_runs; j++) fclose(in_files[j]);
    fclose(out_file);
}
void build_tree_from_sorted_file(const char* sorted_filename){
    FILE* file = fopen(sorted_filename, "r");
    if (!file) { perror("Could not open sorted file"); return; }
    Node* current_leaf = create_node(true);
    root = current_leaf;
    first_leaf = current_leaf;
    Node* leaf_list[10000];
    char promoted_keys[10000][MAX_LINE_LEN];
    int leaf_count = 0;
    leaf_list[leaf_count++] = current_leaf;
    char line[512];
    char last_dept_name[MAX_LINE_LEN] = "";
    UniversityNode* current_uni_list = NULL;
    while (fgets(line, sizeof(line), file)) {
        char uni_name[MAX_LINE_LEN], dept_name[MAX_LINE_LEN];
        float score;
        sscanf(line, "%[^,],%[^,],%f", uni_name, dept_name, &score);
        if (strcmp(dept_name, last_dept_name) != 0 && strlen(last_dept_name) > 0) {
            if (current_leaf->num_keys == ORDER - 1) {
                split_count++;
                Node* new_leaf = create_node(true);
                current_leaf->next = new_leaf;
                current_leaf = new_leaf;
                strcpy(promoted_keys[leaf_count - 1], last_dept_name);
                leaf_list[leaf_count++] = current_leaf;
            }
            strcpy(current_leaf->keys[current_leaf->num_keys], last_dept_name);
            current_leaf->pointers[current_leaf->num_keys] = current_uni_list;
            current_leaf->num_keys++;
            current_uni_list = NULL;
        }
        UniversityNode* new_uni = create_university(uni_name, score);
        insert_into_sorted_list(&current_uni_list, new_uni);
        strcpy(last_dept_name, dept_name);
    }
    if (current_uni_list != NULL) {
         if (current_leaf->num_keys == ORDER - 1) {
            split_count++;
            Node* new_leaf = create_node(true);
            current_leaf->next = new_leaf;
            current_leaf = new_leaf;
            strcpy(promoted_keys[leaf_count - 1], last_dept_name);
            leaf_list[leaf_count++] = current_leaf;
        }
        strcpy(current_leaf->keys[current_leaf->num_keys], last_dept_name);
        current_leaf->pointers[current_leaf->num_keys] = current_uni_list;
        current_leaf->num_keys++;
    }
    fclose(file);
    if (leaf_count > 1) {
        root = build_parent_level(leaf_list, leaf_count, promoted_keys);
    }
}
Node* build_parent_level(Node** children, int count, const char (*keys_from_below)[MAX_LINE_LEN]){
    if (count <= 1) {
        if (count == 1) children[0]->parent = NULL;
        return children[0];
    }

    Node* parent_list[5000];
    char new_promoted_keys[5000][MAX_LINE_LEN];
    int parent_count = 0;
    int promoted_count = 0;
    
    int child_idx = 0;
    while(child_idx < count) {
        Node* current_parent = create_node(false);
        parent_list[parent_count++] = current_parent;
        
        int keys_in_parent = 0;
        
        current_parent->pointers[keys_in_parent] = children[child_idx];
        children[child_idx]->parent = current_parent;
        child_idx++;

        while(keys_in_parent < ORDER - 1 && child_idx < count) {
            strcpy(current_parent->keys[keys_in_parent], keys_from_below[child_idx - 1]);
            current_parent->pointers[keys_in_parent + 1] = children[child_idx];
            children[child_idx]->parent = current_parent;
            keys_in_parent++;
            child_idx++;
        }
        current_parent->num_keys = keys_in_parent;

        if(child_idx < count) {
             split_count++;
             strcpy(new_promoted_keys[promoted_count++], keys_from_below[child_idx - 1]);
        }
    }
    
    return build_parent_level(parent_list, parent_count, new_promoted_keys);
}
void swap_records(Record* a, Record* b){
    Record temp = *a; *a = *b; *b = temp;
}
void min_heapify_replacement(Record heap[], int size, int i){
    int smallest = i; int left = 2 * i + 1; int right = 2 * i + 2;
    if (left < size && compare_records(&heap[left], &heap[smallest]) < 0) smallest = left;
    if (right < size && compare_records(&heap[right], &heap[smallest]) < 0) smallest = right;
    if (smallest != i) {
        swap_records(&heap[i], &heap[smallest]);
        min_heapify_replacement(heap, size, smallest);
    }
}
void swap_heap_nodes(MinHeapNode* a, MinHeapNode* b){
    MinHeapNode temp = *a; *a = *b; *b = temp;
}
void min_heapify_merge(MinHeapNode heap_arr[], int size, int i){
    int smallest = i; int left = 2 * i + 1; int right = 2 * i + 2;
    if (left < size && compare_records(&heap_arr[left].record, &heap_arr[smallest].record) < 0) smallest = left;
    if (right < size && compare_records(&heap_arr[right].record, &heap_arr[smallest].record) < 0) smallest = right;
    if (smallest != i) {
        swap_heap_nodes(&heap_arr[i], &heap_arr[smallest]);
        min_heapify_merge(heap_arr, size, smallest);
    }
}


//Search and Calculation Functions

void reset_metrics() {
    split_count = 0;
    node_allocations = 0;
    uni_node_allocations = 0;
    root = NULL;
    first_leaf = NULL;
}

int calculate_tree_height() {
    int height = 0;
    if (root == NULL) return 0;
    Node* ptr = root;
    while (ptr && !ptr->is_leaf) {
        height++;
        ptr = (Node*)ptr->pointers[0];
    }
    height++; // Add leaf level
    return height;
}

double calculate_memory_usage() {
    double total_memory = (double)(node_allocations * sizeof(Node)) + 
                          (double)(uni_node_allocations * sizeof(UniversityNode));
    return total_memory / (1024.0 * 1024.0); // In MB
}

void search_department_by_rank(const char* dept_name, int rank) {
    Node* leaf = find_leaf(root, dept_name);
    if (leaf == NULL) {
        printf("Department '%s' not found.\n", dept_name);
        return;
    }

    for (int i = 0; i < leaf->num_keys; i++) {
        if (strcmp(leaf->keys[i], dept_name) == 0) {
            UniversityNode* current = (UniversityNode*)leaf->pointers[i];
            int current_rank = 1;
            while(current != NULL && current_rank < rank) {
                current = current->next;
                current_rank++;
            }

            if (current != NULL && current_rank == rank) {
                printf("%s with the base placement score %.2f.\n\n", current->university_name, current->score);
            } else {
                printf("Rank %d not found in department '%s'.\n", rank, dept_name);
            }
            return;
        }
    }
    printf("Department '%s' not found.\n", dept_name);
}

void search_university(const char* uni_name, const char* dept_name) {
    Node* leaf = find_leaf(root, dept_name);
    if (leaf == NULL) {
        //printf("Department '%s' not found.\n", dept_name);
        return;
    }

    for (int i = 0; i < leaf->num_keys; i++) {
        if (strcmp(leaf->keys[i], dept_name) == 0) {
            UniversityNode* current = (UniversityNode*)leaf->pointers[i];
            while(current != NULL) {
                if (strcmp(current->university_name, uni_name) == 0) {
                    //printf("%s with the base placement score %.2f.\n\n", current->university_name, current->score);
                    return;
                }
                current = current->next;
            }
            //printf("University '%s' not found in department '%s'.\n", uni_name, dept_name);
            return;
        }
    }
    //printf("Department '%s' not found.\n", dept_name);
}

double calculate_average_seek_time(const char* filename) {
    if (root == NULL) {
        printf("Tree is empty. No seek time to calculate.\n");
        return 0;
    }
    clock_t start, end;
    FILE* file = fopen(filename, "r");
    if (!file) { perror("Could not open file"); return 0; }
    char line[512];
    fgets(line, sizeof(line), file);

    start = clock();
    while (fgets(line, sizeof(line), file)) {
        char id[MAX_LINE_LEN], uni_name[MAX_LINE_LEN], dept_name[MAX_LINE_LEN];
        float score = 0.0;
        sscanf(line, "%[^,],%[^,],%[^,],%f", id, uni_name, dept_name, &score);
        if (strlen(dept_name) > 0 && strlen(uni_name) > 0) {
            search_university(uni_name, dept_name);
            total_record++;
        }
    }
    end = clock();
    fclose(file);
    return ((double)(end - start)) / CLOCKS_PER_SEC / total_record;
}