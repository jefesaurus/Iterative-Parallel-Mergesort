#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "worktree.h"


// Merges the two lists defined as [low, mid) and [mid, high)
// Uses the temp space supplied for swap space.
// This needs to have a size of at least (high - low).
inline void Merge(int* nums, int* temp, int low, int mid, int high) {
  int i = 0;
  int low_d = low;
  int mid_d = mid;

  while (low_d < mid && mid_d < high) {
    if (nums[low_d] > nums[mid_d]) {
      temp[i++] = nums[mid_d++];
    } else {
      temp[i++] = nums[low_d++];
    }
  }

  // If the rest of the mid->high range is sorted, then copy back the lower end.
  if (low_d == mid) {
    memcpy(&(nums[low]), temp, sizeof(int)*(mid_d - low));
    return;
  } else if (mid_d == high) {
    memcpy(&(temp[i]), &(nums[low_d]), sizeof(int)*(mid - low_d));
  }
  memcpy(&(nums[low]), temp, sizeof(int)*(high - low));
}

inline void MergeLevel(int* nums, int low, int high, int size) {
  int merged_size = size*2;
  int* temp_space = (int*)malloc(sizeof(int) * merged_size);
  int rem = (high - low) % merged_size;

  while (low < high - rem) {
    Merge(nums, temp_space, low, low + size, low + merged_size);
    low += merged_size;
  }
  if (rem > 0 && rem > size) {
    Merge(nums, temp_space, low, low + size, low + rem);
  }
  free(temp_space);
}

void MergeSort(int* nums, int low, int high) {
  int size;
  for (size = 1; size < high - low; size *= 2) {
    MergeLevel(nums, low, high, size);
  }
}

bool IsSorted(int* nums, int num_nums) {
  if (num_nums == 0) { return true; };
  int last = nums[0];
  int i;
  for (i = 1; i < num_nums; i++) {
    if (nums[i] < last) {
      printf("%d, %d\n", nums[i], last);
      return false;
    }
    last = nums[i];
  }
  return true;
}


inline bool WorkIsReady(WorkNode* node) {
  if (!node->is_done && !node->in_progress) {
    if (!node->left && !node->right) {
      assert(node->mid == -1);
      return true;
    }

    if (node->left->is_done && node->right->is_done) {
      return true;
    }
  }
  return false;
}

typedef struct ParallelArgs ParallelArgs;

struct ParallelArgs {
  WorkNode* work_tree;
  int* nums;
  pthread_cond_t* signal;
  pthread_mutex_t* sync;
};

bool DoJob(int* nums, WorkNode* node, pthread_mutex_t* sync, pthread_cond_t* signal) {
  if (WorkIsReady(node)) {
    pthread_mutex_lock(&(node->lock));
    if (WorkIsReady(node)) {
      node->in_progress = true;
      // Do node
      if (node->mid == -1) {
        MergeSort(nums, node->low, node->high);
      } else {
        int* temp = (int*)malloc((node->high - node->low)*sizeof(int));
        Merge(nums, temp, node->low, node->mid, node->high);
        free(temp);
      }
      node->is_done = true;
    }
    pthread_mutex_unlock(&(node->lock));
    pthread_cond_signal(signal);
    return true;
  }

  if (node->left && !node->left->is_done) {
    bool did_something = DoJob(nums, node->left, sync, signal);
    if (did_something) {
      return true;
    }
  }
  if (node->right && !node->right->is_done) {
    bool did_something = DoJob(nums, node->right, sync, signal);
    if (did_something) {
      return true;
    }
  }
  return false;
}

void* WorkLoop(void* args_i) {
  ParallelArgs* args = (ParallelArgs*) args_i;

  while (!args->work_tree->is_done) {
    while (!DoJob(args->nums, args->work_tree, args->sync, args->signal) && !args->work_tree->is_done) {
      pthread_cond_wait(args->signal, args->sync);
      pthread_mutex_unlock(args->sync);
    }
  }
  pthread_cond_signal(args->signal);
}

void ParallelMergeSort(int* nums, int num_nums, int num_workers) {
  WorkNode* tree = MakeWorkTree(num_nums, num_workers);
  pthread_mutex_t sync;
  pthread_cond_t signal;
  pthread_mutex_init(&sync, NULL);
  pthread_cond_init(&signal, NULL);

  ParallelArgs args;
  args.work_tree = tree;
  args.nums = nums;
  args.sync = &sync;
  args.signal= &signal;

  pthread_t threads[num_workers];
  int i;
  for (i = 0; i < num_workers; i++) {
    pthread_create(&threads[i], NULL, WorkLoop, (void*)&args);
  }
  for (i = 0; i < num_workers; i++) {
    pthread_join(threads[i], NULL);
  }

  FreeWorkNode(tree);
}

void SerialTest(int num_nums) {
  srand(time(NULL));
  int i;

  int* nums = (int*)malloc(sizeof(int) * num_nums);

  // Generate a bunch of random numbers
  for (i = 0; i < num_nums; i++) {
    nums[i] = rand();
  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  MergeSort(nums, 0, num_nums);
  clock_gettime(CLOCK_MONOTONIC, &end);
  assert(IsSorted(nums, num_nums));
  printf("Serial: %f\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0);

  free(nums);
}



void ParallelTest(int num_nums) {
  srand(time(NULL));
  int i;

  int* nums = (int*)malloc(sizeof(int) * num_nums);

  // Generate a bunch of random numbers
  for (i = 0; i < num_nums; i++) {
    nums[i] = rand();
  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  ParallelMergeSort(nums, num_nums, 8);
  clock_gettime(CLOCK_MONOTONIC, &end);
  assert(IsSorted(nums, num_nums));
  printf("Parallel: %f\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0);

  free(nums);
}

inline int comp (const void * elem1, const void * elem2) {
    int f = *((int*)elem1);
    int s = *((int*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}

void BuiltInTest(int num_nums) {
  srand(time(NULL));
  int i;

  int* nums = (int*)malloc(sizeof(int) * num_nums);

  // Generate a bunch of random numbers
  for (i = 0; i < num_nums; i++) {
    nums[i] = rand();
  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  ParallelMergeSort(nums, num_nums, 8);
  qsort(nums, num_nums, sizeof(int), comp);
  clock_gettime(CLOCK_MONOTONIC, &end);
  assert(IsSorted(nums, num_nums));
  printf("Built in: %f\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0);

  free(nums);
}

int main() {
  ParallelTest(pow(2, 25));
  SerialTest(pow(2, 25));
  BuiltInTest(pow(2, 25));
  return 0;
}
