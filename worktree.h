#ifndef WORKTREE_H
#define WORKTREE_H

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

#include <sys/queue.h>


typedef struct WorkNode WorkNode;

struct WorkNode {
  int low, high, mid;
  bool is_done;
  bool in_progress;
  pthread_mutex_t lock;

  WorkNode* left;
  WorkNode* right;
};

void PrintNode(WorkNode* node) {
  printf("[%d, %d, %d)\n", node->low, node->mid, node->high);
  if (node->left) {
    PrintNode(node->left);
  }
  if (node->right) {
    PrintNode(node->right);
  }
}

inline void InitLeafNode(WorkNode* node, int low, int high) {
  node->low = low;
  node->high = high;
  node->mid = -1; // If this is -1, it needs a full sort. Otherwise, just a merge.

  node->is_done = false;
  node->in_progress = false;
  node->left = NULL;
  node->right = NULL;
  pthread_mutex_init(&node->lock, NULL);
}

inline void InitBranchNode(WorkNode* node, WorkNode* left, WorkNode* right) {
  node->low = left->low;
  node->mid = right->low; 
  node->high = right->high;
  node->left = left;
  node->right = right;
}

WorkNode* MakeWorkTree(int num_nums, int num_jobs) {
  int base_job_size = num_nums / num_jobs;
  int larger_job_size = base_job_size + 1;
  int num_larger_jobs = num_nums % num_jobs;
  int i;

  WorkNode* base[num_jobs];

  // Make the base nodes
  int current = 0;
  for (i = 0; i < num_jobs; i++) {
    base[i] = (WorkNode*)malloc(sizeof(WorkNode));
    if (i < num_larger_jobs) {
      InitLeafNode(base[i], current, current + larger_job_size);
      current += larger_job_size;
    } else {
      InitLeafNode(base[i], current, current + base_job_size);
      current += base_job_size;
    }
  }

  WorkNode* temp_space[num_jobs/2];

  int num_source = num_jobs;
  WorkNode** source_level = base;
  int num_next = 0;
  WorkNode** next_level = temp_space;

  while (num_source >= 2) {
    for (i = 0; i < num_source/2; i++) {
      next_level[i] = (WorkNode*)malloc(sizeof(WorkNode));
      InitBranchNode(next_level[i], source_level[i*2], source_level[i*2+1]);
      num_next++;
    }

    if (num_source % 2 == 1) {
      next_level[num_next] = source_level[num_source - 1];
      num_next++;
    }

    num_source = num_next;
    num_next = 0;
    WorkNode** temp_ptr = next_level;
    next_level = source_level;
    source_level = temp_ptr;
  }

  return source_level[0];
}

void FreeWorkNode(WorkNode* node) {
  WorkNode* left = node->left;
  WorkNode* right = node->right;
  free(node);
  if (left) {
    FreeWorkNode(left);
  }
  if (right) {
    FreeWorkNode(right);
  }
}

#endif // WORKTREE_H
