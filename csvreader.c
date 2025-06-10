#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char name[100];
    char department[100];
    float lowest_score;
} University;

typedef struct Node {
    University uni;
    struct Node *next;
} Node;

Node* create_node(University uni) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    new_node->uni = uni;
    new_node->next = NULL;
    return new_node;
}

void addToList(Node **head, University uni) {
    Node *new_node = create_node(uni);
    if (!new_node) return;

    if (*head == NULL) {
        *head = new_node;
    } else {
        Node *temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

void printList(Node *head) {
    Node *temp = head;
    while (temp != NULL) {
        printf("ID: %d, Name: %s, Department: %s, Lowest Score: %.2f\n",
               temp->uni.id, temp->uni.name, temp->uni.department, temp->uni.lowest_score);
        temp = temp->next;
    }
}
void freeList(Node *head) {
    Node *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

int main(){
    FILE *csvfile = fopen("yok_atlas.csv", "r");
    if (!csvfile) {
        printf("Error opening csv file.\n");
    } else {
        char line[200];
        Node *head = NULL;

        fgets(line, sizeof(line), csvfile);
        while (fgets(line, sizeof(line), csvfile)) {
            line[strcspn(line, "\n")] = 0;
            University uni;

            char *token = strtok(line, ",");
            if (token)
                uni.id = atoi(token);

            token = strtok(NULL, ",");
            if (token)
                strncpy(uni.name, token, sizeof(uni.name));

            token = strtok(NULL, ",");
            if (token)
                strncpy(uni.department, token, sizeof(uni.department));
            
            token = strtok(NULL, ",");
            if (token)
                uni.lowest_score = atof(token);

            addToList(&head, uni);
            
        }
        fclose(csvfile);
        printf("\nList of Universities:\n");
        printList(head);
        freeList(head);
    }
}