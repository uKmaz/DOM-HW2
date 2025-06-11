#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 4
#define MAX_LINE 512

// Üniversite bilgileri bağlı listede tutulur
typedef struct UniversityNode {
    char name[100];
    float score;
    struct UniversityNode* next;
} UniversityNode;

// B+ Tree düğümü
typedef struct BPlusNode {
    int isLeaf;
    int numKeys;
    char* keys[ORDER - 1];
    UniversityNode* universityLists[ORDER - 1];
    struct BPlusNode* children[ORDER];
    struct BPlusNode* next; // leaf node bağlantısı
} BPlusNode;

char* strdup_safe(const char* s) {
    char* copy = malloc(strlen(s) + 1);
    strcpy(copy, s);
    return copy;
}

// Üniversiteyi azalan sırada listeye ekle
UniversityNode* insertUniversitySorted(UniversityNode* head, const char* name, float score) {
    UniversityNode* newNode = malloc(sizeof(UniversityNode));
    strcpy(newNode->name, name);
    newNode->score = score;
    newNode->next = NULL;

    if (!head || head->score < score) {
        newNode->next = head;
        return newNode;
    }

    UniversityNode* current = head;
    while (current->next && current->next->score >= score)
        current = current->next;

    newNode->next = current->next;
    current->next = newNode;
    return head;
}

// B+ düğümü oluştur
BPlusNode* createNode(int isLeaf) {
    BPlusNode* node = malloc(sizeof(BPlusNode));
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    node->next = NULL;

    for (int i = 0; i < ORDER; i++) node->children[i] = NULL;
    for (int i = 0; i < ORDER - 1; i++) {
        node->keys[i] = NULL;
        node->universityLists[i] = NULL;
    }

    return node;
}

// Anahtar yerini bul
int findIndex(char* keys[], int num, const char* key) {
    int i = 0;
    while (i < num && strcmp(key, keys[i]) > 0) i++;
    return i;
}

int insertToExistingDepartment(BPlusNode* root, const char* dept, const char* univ, float score) {
    if (!root) return 0;

    // Yapraklara in
    while (!root->isLeaf)
        root = root->children[0];

    // Tüm yapraklarda tara
    BPlusNode* leaf = root;
    while (leaf) {
        for (int i = 0; i < leaf->numKeys; i++) {
            if (strcmp(leaf->keys[i], dept) == 0) {
                leaf->universityLists[i] = insertUniversitySorted(leaf->universityLists[i], univ, score);
                return 1;
            }
        }
        leaf = leaf->next;
    }

    return 0;
}

// Insert recursive (sadece yeni bölüm için çağrılır)
BPlusNode* insertRecursive(BPlusNode* node, const char* dept, const char* univ, float score, char** newKey, BPlusNode** newChild) {
    if (node->isLeaf) {
        int pos = findIndex(node->keys, node->numKeys, dept);

        if (node->numKeys < ORDER - 1) {
            for (int i = node->numKeys; i > pos; i--) {
                node->keys[i] = node->keys[i - 1];
                node->universityLists[i] = node->universityLists[i - 1];
            }
            node->keys[pos] = strdup_safe(dept);
            node->universityLists[pos] = insertUniversitySorted(NULL, univ, score);
            node->numKeys++;
            return NULL;
        }

        // Split
        char* tempKeys[ORDER];
        UniversityNode* tempLists[ORDER];
        for (int i = 0; i < ORDER - 1; i++) {
            tempKeys[i] = node->keys[i];
            tempLists[i] = node->universityLists[i];
        }

        for (int i = ORDER - 1; i > pos; i--) {
            tempKeys[i] = tempKeys[i - 1];
            tempLists[i] = tempLists[i - 1];
        }

        tempKeys[pos] = strdup_safe(dept);
        tempLists[pos] = insertUniversitySorted(NULL, univ, score);

        int mid = ORDER / 2;
        node->numKeys = 0;
        for (int i = 0; i < mid; i++) {
            node->keys[i] = tempKeys[i];
            node->universityLists[i] = tempLists[i];
            node->numKeys++;
        }

        BPlusNode* newNode = createNode(1);
        for (int i = mid; i < ORDER; i++) {
            newNode->keys[newNode->numKeys] = tempKeys[i];
            newNode->universityLists[newNode->numKeys] = tempLists[i];
            newNode->numKeys++;
        }

        newNode->next = node->next;
        node->next = newNode;

        *newKey = strdup_safe(newNode->keys[0]);
        *newChild = newNode;
        return newNode;
    }

    // Internal node
    int i = findIndex(node->keys, node->numKeys, dept);
    char* childKey = NULL;
    BPlusNode* childSplit = NULL;

    insertRecursive(node->children[i], dept, univ, score, &childKey, &childSplit);

    if (!childSplit) return NULL;

    if (node->numKeys < ORDER - 1) {
        for (int j = node->numKeys; j > i; j--) {
            node->keys[j] = node->keys[j - 1];
            node->children[j + 1] = node->children[j];
        }
        node->keys[i] = childKey;
        node->children[i + 1] = childSplit;
        node->numKeys++;
        return NULL;
    }

    // Internal split
    char* tempKeys[ORDER];
    BPlusNode* tempChildren[ORDER + 1];
    for (int j = 0; j < ORDER - 1; j++) tempKeys[j] = node->keys[j];
    for (int j = 0; j < ORDER; j++) tempChildren[j] = node->children[j];

    for (int j = ORDER - 1; j > i; j--) {
        tempKeys[j] = tempKeys[j - 1];
        tempChildren[j + 1] = tempChildren[j];
    }

    tempKeys[i] = childKey;
    tempChildren[i + 1] = childSplit;

    int mid = ORDER / 2;
    node->numKeys = 0;
    for (int j = 0; j < mid; j++) {
        node->keys[j] = tempKeys[j];
        node->children[j] = tempChildren[j];
        node->numKeys++;
    }
    node->children[mid] = tempChildren[mid];

    BPlusNode* newNode = createNode(0);
    for (int j = mid + 1, k = 0; j < ORDER; j++, k++) {
        newNode->keys[k] = tempKeys[j];
        newNode->children[k] = tempChildren[j];
        newNode->numKeys++;
    }
    newNode->children[newNode->numKeys] = tempChildren[ORDER];

    *newKey = tempKeys[mid];
    *newChild = newNode;
    return newNode;
}

// Ana insert fonksiyonu
BPlusNode* insert(BPlusNode* root, const char* dept, const char* univ, float score) {
    if (root && insertToExistingDepartment(root, dept, univ, score))
        return root;

    if (!root) {
        root = createNode(1);
        root->keys[0] = strdup_safe(dept);
        root->universityLists[0] = insertUniversitySorted(NULL, univ, score);
        root->numKeys = 1;
        return root;
    }

    char* newKey = NULL;
    BPlusNode* newChild = NULL;
    insertRecursive(root, dept, univ, score, &newKey, &newChild);

    if (newChild) {
        BPlusNode* newRoot = createNode(0);
        newRoot->keys[0] = newKey;
        newRoot->children[0] = root;
        newRoot->children[1] = newChild;
        newRoot->numKeys = 1;
        return newRoot;
    }

    return root;
}

// Arama: bölüm + sıralama
void search(BPlusNode* root, const char* dept, int rank) {
    if (!root) {
        printf("Tree is empty\n");
        return;
    }

    // En sola in
    while (!root->isLeaf)
        root = root->children[0];

    // Tüm yaprakları sırayla tara
    while (root) {
        for (int i = 0; i < root->numKeys; i++) {
            if (strcmp(root->keys[i], dept) == 0) {
                UniversityNode* u = root->universityLists[i];
                for (int j = 1; j < rank && u; j++) u = u->next;
                if (u)
                    printf("%s with base placement score %.2f\n", u->name, u->score);
                else
                    printf("Rank %d not found for department %s\n", rank, dept);
                return;
            }
        }
        root = root->next;
    }

    printf("Department %s not found\n", dept);
}


// CSV'den veri yükle
void loadCSV(const char* filename, BPlusNode** root) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Dosya açılamadı");
        return;
    }

    char line[MAX_LINE];
    fgets(line, MAX_LINE, fp); // başlığı atla

    while (fgets(line, MAX_LINE, fp)) {
        char* uuid = strtok(line, ",");
        char* univ = strtok(NULL, ",");
        char* dept = strtok(NULL, ",");
        char* scoreStr = strtok(NULL, ",\n");

        if (!uuid || !univ || !dept || !scoreStr) continue;

        float score = atof(scoreStr);
        *root = insert(*root, dept, univ, score);
    }

    fclose(fp);
}

// Test
int main() {
    BPlusNode* root = NULL;
    loadCSV("yok_atlas.csv", &root);

    printf("Veri yüklendi. Arama testleri:\n");

    search(root, "Bilgisayar Muhendisligi", 1);
    search(root, "Tip", 3);
    search(root, "Fizik", 2);
    search(root, "Tip", 1);

    return 0;
}

