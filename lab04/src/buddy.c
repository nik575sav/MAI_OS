#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

typedef struct BuddyNode {
    int size;             
    int free;             
    struct BuddyNode *left;  
    struct BuddyNode *right; 
} BuddyNode;

typedef struct Allocator {
    BuddyNode *root;      
    void *memory;         
    int totalSize;        
    int offset;           
} Allocator;

// Проверка, является ли число степенью двойки
int is_power_of_two(unsigned int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

// Создание нового узла
BuddyNode *create_node(Allocator *allocator, int size) {
    if ((size_t)(allocator->offset + sizeof(BuddyNode)) > (size_t)(allocator->totalSize)) {
        return NULL;
    }

    BuddyNode *node = (BuddyNode *)((char *)allocator->memory + allocator->offset);
    allocator->offset += sizeof(BuddyNode);

    node->size = size;
    node->free = 1;
    node->left = NULL;
    node->right = NULL;

    return node;
}

// Создание аллокатора
EXPORT Allocator *allocator_create(void *mem, size_t size) {
    if (!is_power_of_two(size)) {
        const char msg[] = "This allocator requires a power of two size\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    Allocator *allocator = (Allocator *)mem;
    allocator->memory = (char *)mem + sizeof(Allocator);
    allocator->totalSize = size - sizeof(Allocator);
    allocator->offset = 0;

    allocator->root = create_node(allocator, size);
    return allocator->root ? allocator : NULL;
}

// Разделение узла на два "двойника"
void split_node(Allocator *allocator, BuddyNode *node) {
    int newSize = node->size / 2;

    node->left = create_node(allocator, newSize);
    node->right = create_node(allocator, newSize);
}

// Рекурсивное выделение памяти
BuddyNode *allocate_recursive(Allocator *allocator, BuddyNode *node, int size) {
    if (!node || node->size < size || !node->free) {
        return NULL;
    }

    if (node->size == size) {
        node->free = 0;
        return node;
    }

    if (!node->left) {
        split_node(allocator, node);
    }

    BuddyNode *allocated = allocate_recursive(allocator, node->left, size);
    if (!allocated) {
        allocated = allocate_recursive(allocator, node->right, size);
    }

    node->free = (node->left && node->left->free) || (node->right && node->right->free);
    return allocated;
}

// Выделение памяти
EXPORT void *allocator_alloc(Allocator *allocator, size_t size) {
    if (!allocator || size <= 0) {
        return NULL;
    }

    while (!is_power_of_two(size)) {
        size++;
    }

    return allocate_recursive(allocator, allocator->root, size);
}

// Освобождение памяти
EXPORT void allocator_free(Allocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return;
    }

    BuddyNode *node = (BuddyNode *)ptr;
    if (!node) {
        return;
    }

    node->free = 1;

    if (node->left && node->left->free && node->right && node->right->free) {
        allocator_free(allocator, node->left);
        allocator_free(allocator, node->right);
        node->left = NULL;
        node->right = NULL;
    }
}

// Уничтожение аллокатора
EXPORT void allocator_destroy(Allocator *allocator) {
    if (!allocator) {
        return;
    }

    allocator_free(allocator, allocator->root);

    if (munmap((void *)allocator, allocator->totalSize + sizeof(Allocator)) == -1) {
        exit(EXIT_FAILURE);
    }
}
