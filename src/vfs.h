#ifndef VFS_H
#define VFS_H

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

// https://fr.wikipedia.org/wiki/Virtual_File_System
struct superblock_t;
typedef struct superblock_t superblock_t;

typedef struct {
} super_operations_t;

typedef struct {

} inode_operations_t;
typedef struct {

} file_operations_t;

#define VFS_DIRECTORY 0b1000
#define VFS_WRITE     0b0100
#define VFS_READ      0b0010
#define VFS_EXECUTE   0b0001

typedef struct {
  int number;
  int attr;
  void* data;
  superblock_t* sb;
  inode_operations_t* op;
} inode_t;

typedef struct {
  superblock_t* sb;
  inode_t* inode;
  file_operations_t* op;
} file_t;

struct superblock_t {
  int id;                  // device ID
  inode_t* root;           // root node
  super_operations_t* op;  // superblock methods
};

// returns a free inode
int get_inode();


#endif
