#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEYS 4  // Max keys in internal node (B+ Tree order)

// ======================= University Node =======================
typedef struct University {
    char name[100];
    int placementScore;
    struct University* next;
} University;

// ======================= B+ Tree Leaf Node =======================
typedef struct BPlusTreeLeaf {
    char departmentName[100];
    University* universityList;
    struct BPlusTreeLeaf* nextLeaf;
} BPlusTreeLeaf;

// ======================= B+ Tree Node =======================
typedef struct BPlusTreeNode {
    int isLeaf;
    int numKeys;
    char keys[MAX_KEYS][100];
    struct BPlusTreeNode* children[MAX_KEYS + 1];
    BPlusTreeLeaf* leaves[MAX_KEYS + 1];  // Only used for leaf pointers
} BPlusTreeNode;

BPlusTreeNode* root = NULL;

// ======================= University Creation =======================
University* createUniversity(const char* name, int score) {
    University* uni = malloc(sizeof(University));
    strcpy(uni->name, name);
    uni->placementScore = score;
    uni->next = NULL;
    return uni;
}

// ======================= University List Copy =======================
University* copyUniversityList(University* original) {
    if (!original) return NULL;
    
    University* newHead = NULL;
    University* newTail = NULL;
    University* current = original;
    
    while (current) {
        University* newUniversity = createUniversity(current->name, current->placementScore);
        
        if (!newHead) {
            newHead = newUniversity;
            newTail = newUniversity;
        } else {
            newTail->next = newUniversity;
            newTail = newUniversity;
        }
        current = current->next;
    }
    
    return newHead;
}

// ======================= University Linked List Sort =======================
University* replacementSelectionSort(University* head) {
    if (!head || !head->next) return head;

    University* sorted = NULL;

    while (head) {
        University *maxNode = head, *maxPrev = NULL, *curr = head;

        // Find the node with maximum placement score (for descending order)
        while (curr->next) {
            if (curr->next->placementScore > maxNode->placementScore) {
                maxPrev = curr;
                maxNode = curr->next;
            }
            curr = curr->next;
        }

        // Remove maxNode from original list
        if (maxPrev)
            maxPrev->next = maxNode->next;
        else
            head = head->next;

        // Add maxNode to front of sorted list
        maxNode->next = sorted;
        sorted = maxNode;
    }

    return sorted; // Already in descending order, no need to reverse
}

// ======================= Insert into Leaf Node =======================
BPlusTreeLeaf* createLeaf(char* departmentName, University* universities) {
    BPlusTreeLeaf* leaf = (BPlusTreeLeaf*)malloc(sizeof(BPlusTreeLeaf));
    strcpy(leaf->departmentName, departmentName);
    
    // Orijinal listeyi kopyala, sonra kopyayı sırala
    University* universityCopy = copyUniversityList(universities);
    leaf->universityList = replacementSelectionSort(universityCopy);
    
    leaf->nextLeaf = NULL;
    return leaf;
}

// ======================= Insert into B+ Tree =======================
BPlusTreeNode* createNode() {
    BPlusTreeNode* node = malloc(sizeof(BPlusTreeNode));
    node->isLeaf = 0;
    node->numKeys = 0;
    for (int i = 0; i < MAX_KEYS + 1; i++) {
        node->children[i] = NULL;
        if (i < MAX_KEYS + 1) node->leaves[i] = NULL;
    }
    for (int i = 0; i < MAX_KEYS; i++) {
        node->keys[i][0] = '\0';
    }
    return node;
}

void insertLeaf(BPlusTreeNode** node, char* departmentName, University* universities) {
    if (!*node) {
        *node = createNode();
        (*node)->isLeaf = 1;
    }
    
    BPlusTreeLeaf* newLeaf = createLeaf(departmentName, universities);

    int i = (*node)->numKeys - 1;
    printf("Inserting department: %s (alphabetically sorted)\n", departmentName);
    
    // Shift existing keys and leaves to make room - ALPHABETICAL ORDER
    while (i >= 0 && strcmp((*node)->keys[i], departmentName) > 0) {
        strcpy((*node)->keys[i + 1], (*node)->keys[i]);
        (*node)->leaves[i + 1] = (*node)->leaves[i];
        i--;
    }

    // Insert at correct position (alphabetically sorted)
    strcpy((*node)->keys[i + 1], departmentName);
    (*node)->leaves[i + 1] = newLeaf;
    (*node)->numKeys++;

    // Link ALL leaf nodes sequentially for range queries
    // This ensures proper linking between all leaves
    for (int j = 0; j < (*node)->numKeys - 1; j++) {
        if ((*node)->leaves[j] && (*node)->leaves[j + 1]) {
            (*node)->leaves[j]->nextLeaf = (*node)->leaves[j + 1];
        }
    }
    // Last leaf points to NULL
    if ((*node)->numKeys > 0 && (*node)->leaves[(*node)->numKeys - 1]) {
        (*node)->leaves[(*node)->numKeys - 1]->nextLeaf = NULL;
    }
    
    printf("Successfully inserted %s. Node now has %d keys.\n", departmentName, (*node)->numKeys);
}

// ======================= Print Tree (For Testing) =======================
void printTree(BPlusTreeNode* node) {
    if (!node) {
        printf("Tree is empty.\n");
        return;
    }

    printf("\n=== B+ Tree Structure ===\n");
    printf("Keys (Departments) in alphabetical order:\n");
    for (int i = 0; i < node->numKeys; i++) {
        printf("%d. %s\n", i+1, node->keys[i]);
    }
    
    printf("\n=== Leaf Nodes Content ===\n");
    for (int i = 0; i < node->numKeys; i++) {
        printf("Dept: %s -> Universities (sorted by placement score - DESCENDING): ", node->keys[i]);
        University* u = node->leaves[i]->universityList;
        while (u) {
            printf("[%s (%d)] ", u->name, u->placementScore);
            if (u->next) printf("-> ");
            u = u->next;
        }
        printf("-> NULL\n");
    }
    
    printf("\n=== Leaf Linking (for range queries) ===\n");
    if (node->numKeys > 0 && node->leaves[0]) {
        BPlusTreeLeaf* currentLeaf = node->leaves[0];
        while (currentLeaf) {
            printf("[%s] ", currentLeaf->departmentName);
            if (currentLeaf->nextLeaf) printf("-> ");
            currentLeaf = currentLeaf->nextLeaf;
        }
        printf("-> NULL\n");
    }
}

// ======================= Memory Management =======================
void freeUniversities(University* head) {
    while (head) {
        University* temp = head;
        head = head->next;
        free(temp);
    }
}

void freeLeafs(BPlusTreeLeaf* leaf) {
    while (leaf) {
        BPlusTreeLeaf* temp = leaf;
        leaf = leaf->nextLeaf;
        freeUniversities(temp->universityList);
        free(temp);
    }
}

void freeTree(BPlusTreeNode* node) {
    if (!node) return;
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            freeTree(node->children[i]);
        }
    } else {
        // Free all leaves
        for (int i = 0; i < node->numKeys; i++) {
            if (node->leaves[i]) {
                freeUniversities(node->leaves[i]->universityList);
                free(node->leaves[i]);
            }
        }
    }
    free(node);
}

// ======================= Main Function =======================
int main() {
    root = NULL;
    printf("B+ Tree Construction - Part 1\n");
    printf("Requirements:\n");
    printf("- Keys: Department names sorted alphabetically\n");
    printf("- Each leaf: Department name + university list sorted by placement score (DESCENDING)\n");
    printf("- All leaf nodes linked for range queries\n\n");
    
    // Example data - creating separate lists for each department
    printf("Creating university data...\n");
    University* csList = createUniversity("UniA", 90);
    csList->next = createUniversity("UniB", 85);
    csList->next->next = createUniversity("UniC", 95);

    University* mathList = createUniversity("UniX", 88);
    mathList->next = createUniversity("UniY", 80);
    mathList->next->next = createUniversity("UniZ", 92);
    
    // Add more departments to test alphabetical sorting
    University* physicsList = createUniversity("UniP", 87);
    physicsList->next = createUniversity("UniQ", 83);
    
    University* biologyList = createUniversity("UniB1", 89);
    biologyList->next = createUniversity("UniB2", 91);
    
    printf("Inserting departments into B+ Tree...\n");
    insertLeaf(&root, "Mathematics", mathList);
    
    insertLeaf(&root, "ComputerScience", csList);
    
    insertLeaf(&root, "Physics", physicsList);
    
    insertLeaf(&root, "Biology", biologyList);

    printTree(root);

    // Free allocated memory
    freeTree(root);
    
    // Free original lists (since we made copies)
    freeUniversities(csList);
    freeUniversities(mathList);
    freeUniversities(physicsList);
    freeUniversities(biologyList);

    printf("\nProgram completed successfully.\n");
    return 0;
}