#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 10000
#define MAX_CHAR 63

#define QUEUE_SIZE 256
#define DEFAULT_TAPE_SIZE 256
#define ACCEPTED_SIZE 100

// Turing Machine Tape
typedef struct {
  char *leftTape;
  char *rightTape;
  int leftSize;
  int rightSize;
  int maxLeft;
  int maxRight;
  int count;
} tape;

// Transitions
typedef struct _trans {
  int destState;
  char writeChar;
  int tapeMove;
  struct _trans *next;
} transition;

// Queue
typedef struct {
  int state;
  unsigned long int transNum;
  int headPos;
  tape *currTape;
} queue_elem;

typedef struct {
  int head;
  int tail;
  queue_elem queueElems[QUEUE_SIZE];
} queue;

// Accepted States
typedef struct {
  int acceptedList[ACCEPTED_SIZE];
  int acceptedNum;
} accepted;

// Global Variables

unsigned long int maxTransitions;

transition ***adjList;
accepted accStates;

// Transitions Functions

/**
 * Returns the index of hash table from read char.
 */
int get_hash_table_index(char readChar) {
  if (readChar >= '0' && readChar <= '9') {
    return readChar - '0';
  }
   if (readChar >= 'A' && readChar <= 'Z') {
    return readChar - 'A' + 10;
  }
   if (readChar == '_') {
    return readChar - '_' + 36;
  }
   if (readChar >= 'a' && readChar <= 'z') {
    return readChar - 'a' + 37;
  }
   return -1;
}

/**
 * Adds transisition to adjacency list
 */
void add_transition(int sourceState, int destState, char readChar, char writeChar, int tapeMove) {
  if (adjList[sourceState] == NULL) { // New State
    adjList[sourceState] = calloc(MAX_CHAR, sizeof(transition*));
  }

  transition *newTrans = malloc(sizeof(transition));

  int charIndex = get_hash_table_index(readChar);
  if (adjList[sourceState][charIndex] == NULL) { // First transition for readChar
    adjList[sourceState][charIndex] = newTrans;
    newTrans->next = NULL;
  } else {
    newTrans->next = adjList[sourceState][charIndex];
    adjList[sourceState][charIndex] = newTrans;
  }

  newTrans->destState = destState;
  newTrans->writeChar = writeChar;
  newTrans->tapeMove = tapeMove;
}

/**
 * Returns a transitions list from the source state and read char.
 */
transition *get_transitions(int sourceState, char readChar) {
  if (adjList[sourceState] == NULL) {
    return NULL;
  }

  int charIndex = get_hash_table_index(readChar);
  if (adjList[sourceState][charIndex] == NULL) { // No transition from sourceState with readChar
    return NULL;
  }

  return adjList[sourceState][charIndex];
}

// Tape Functions

/**
 * Returns a tape instance from input string.
 * Used for initializing the tape with the test string.
 */
tape *tape_init(char *string) {
  tape *newTape = malloc(sizeof(tape));

  newTape->leftTape = NULL;
  newTape->leftSize = 0;
  newTape->maxLeft = 0;

  newTape->rightTape = string;
  newTape->rightSize = (int)strlen(string);
  newTape->maxRight = (int)strlen(string);

  newTape->count = 1;

  return newTape;
}

/**
 * Returns a copy of the tape instance passed.
 * Used for creating a copy when need to write on tape.
 */
tape *copy_tape(tape *currTape) {
  tape *tapeCopy = malloc(sizeof(tape));
  size_t leftSize;
  size_t rightSize;

  rightSize = (1 + strlen(currTape->rightTape)) * sizeof(char);
  if (currTape->leftTape != NULL) {
    leftSize = (1 + strlen(currTape->leftTape)) * sizeof(char);
  } else {
    leftSize = 0;
  }

  tapeCopy->rightTape = malloc(rightSize);
  memcpy(tapeCopy->rightTape, currTape->rightTape, rightSize);
  tapeCopy->rightSize = currTape->rightSize;
  tapeCopy->maxRight = currTape->maxRight;

  if (leftSize > 0) {
    tapeCopy->leftTape = malloc(leftSize);
    memcpy(tapeCopy->leftTape, currTape->leftTape, leftSize);
  } else {
    tapeCopy->leftTape = NULL;
  }

  tapeCopy->leftSize = currTape->leftSize;
  tapeCopy->maxLeft = currTape->maxLeft;

  tapeCopy->count = 1;

  return tapeCopy;
}

/**
 * Returns the char on the tape in a certain position.
 */
char get_char_on_tape(tape *currTape, int pos) {
  if (pos >= 0) {
    if (pos < currTape->maxRight) {
      return currTape->rightTape[pos];
    } else {
      return '_';
    }
  } else {
    pos = -pos - 1;

    if (pos < currTape->maxLeft) {
      return currTape->leftTape[pos];
    } else {
      return '_';
    }
  }
}

/**
 * Returns 1 if the computation will enter in a loop, 0 otherwise.
 */
int is_looped(tape *currTape, int pos, int tapeMove) {
  if (pos >= 0) {
    if (pos >= currTape->maxRight && tapeMove == 1) {
      return 1;
    } else {
      return 0;
    }
  } else {
    pos = -pos - 1;

    if (pos >= currTape->maxLeft && tapeMove == -1) {
      return 1;
    } else {
      return 0;
    }
  }
}

/**
 * Returns a modified tape instance. It creates a new tape instance if the
 * number of tape owners is more than 1 and enlarge the array of the tape if the
 * requested tape cell is in a memory cell not allocated.
 */
tape *modify_tape(tape *currTape, int headPos, char writeChar) {
  tape *modifiedTape;

  if (currTape->count > 1) {
    modifiedTape = copy_tape(currTape);
  } else {
    modifiedTape = currTape;
    modifiedTape->count++;
  }

  if (headPos >= 0) { // Right Side
    if (headPos >= currTape->rightSize) { // Need to enlarge array
      modifiedTape->rightSize = currTape->rightSize * 4;
      modifiedTape->rightTape = realloc(modifiedTape->rightTape, modifiedTape->rightSize);
    }

    if (headPos == modifiedTape->maxRight) {
      modifiedTape->maxRight++;
    }

    modifiedTape->rightTape[headPos] = writeChar;
  } else { // Left Side
    headPos = -headPos - 1;

    if (headPos >= currTape->leftSize) { // Need to enlarge array
      if (modifiedTape->leftSize > 0) {
        modifiedTape->leftSize = modifiedTape->leftSize * 4;
      } else {
        modifiedTape->leftSize = DEFAULT_TAPE_SIZE;
      }
      modifiedTape->leftTape = realloc(modifiedTape->leftTape, modifiedTape->leftSize);
    }

    if (headPos == modifiedTape->maxLeft) {
      modifiedTape->maxLeft++;
    }

    modifiedTape->leftTape[headPos] = writeChar;
  }

  return modifiedTape;
}

// Queue Functions

/**
 * Creates and enqueues a queue element to the end of the queue.
 */
void enqueue(queue *currQueue, int state, int headPos, unsigned long int transNum, tape *currTape) {
  int tail = currQueue->tail;
  currQueue->queueElems[tail].state = state;
  currQueue->queueElems[tail].transNum = transNum;
  currQueue->queueElems[tail].headPos = headPos;
  currQueue->queueElems[tail].currTape = currTape;

  if (tail == QUEUE_SIZE - 1) {
    currQueue->tail = 0;
  } else {
    currQueue->tail++;
  }
}

/**
 * Dequeues and returns a queue element from the start of the queue.
 */
queue_elem dequeue(queue *currQueue) {
  queue_elem elem = currQueue->queueElems[currQueue->head];

  if (currQueue->head == QUEUE_SIZE - 1) {
    currQueue->head = 0;
  } else {
    currQueue->head++;
  }

  return elem;
}

/**
 * Returns 1 if the queue is empty, 0 otherwise.
 */
int is_queue_empty(queue *currQueue) {
  if (currQueue->head == currQueue->tail) {
    return 1;
  } else {
    return 0;
  }
}

// Miscellaneous Functions

/**
 * Returns 1 if the state is accepted, 0 otherwise.
 * Uses binary search for searching the state in the array of accepted states.
 */
int is_state_accepted(int state) {
  int low = 0;
  int high = accStates.acceptedNum;
  int middle;

  while (low <= high && low < accStates.acceptedNum) {
    middle = low + (high - low) / 2;
    if (state == accStates.acceptedList[middle]) {
      return 1;
    } else if (state > accStates.acceptedList[middle]) {
      low = middle + 1;
    } else {
      high = middle - 1;
    }
  }
  return 0;
}

/**
 * Returns the difference between two integers passed.
 * Used for sorting accepted states with qsort.
 */
int compare(const void *a, const void *b) { return (*(int *)a - *(int *)b); }

/**
 * Frees transitions
 */
void free_transitions() {
  for (int i = 0; i < MAX_STATES; ++i) {
    if (adjList[i] != NULL) {
      for (int j = 0; j < MAX_CHAR; ++j) {
        if (adjList[i][j] != NULL) {
          transition *temp1 = adjList[i][j];
          transition *temp2 = temp1;

          while (temp1 != NULL) {
            temp1 = temp1->next;
            free(temp2);
            temp2 = temp1;
          }
        }
      }
      free(adjList[i]);
    }
  }
  free(adjList);
}

// Simulation Function

/**
 * Simulates the Turing Machine with the passed string as input.
 * The simulation is done using BFS algorithm.
 */
void execute_string(char *string) {
  queue *currQueue = malloc(sizeof(queue));
  currQueue->head = 0;
  currQueue->tail = 0;

  int pathNotTerminated = 0;

  tape *tempTape = tape_init(string);

  enqueue(currQueue, 0, 0, maxTransitions, tempTape);
  while (!is_queue_empty(currQueue)) {
    queue_elem queueElem = dequeue(currQueue);
    char currReadChar = get_char_on_tape(queueElem.currTape, queueElem.headPos);
    transition *trans = get_transitions(queueElem.state, currReadChar);

    while (trans != NULL) {
      if (is_state_accepted(trans->destState)) { // Accepted string
        queueElem.currTape->count--;

        if (!queueElem.currTape->count) {
          free(queueElem.currTape->leftTape);
          free(queueElem.currTape->rightTape);
          free(queueElem.currTape);
        }

        while (!is_queue_empty(currQueue)) { // Freeing remaining tapes
          queueElem = dequeue(currQueue);

          queueElem.currTape->count--;

          if (!queueElem.currTape->count) {
            free(queueElem.currTape->leftTape);
            free(queueElem.currTape->rightTape);
            free(queueElem.currTape);
          }
        }

        free(currQueue);
        printf("1\n");
        return;
      }

      if ((queueElem.state == trans->destState) && // Loop Checker
          ((!(trans->tapeMove)) ||
          ((currReadChar == '_') && is_looped(queueElem.currTape, queueElem.headPos, trans->tapeMove)))) {
          pathNotTerminated++;

          trans = trans->next;
          continue;
      }

      if (trans->writeChar != currReadChar) {
        tempTape = modify_tape(queueElem.currTape, queueElem.headPos, trans->writeChar);
      } else {
        tempTape = queueElem.currTape;
        tempTape->count++;
      }

      if (queueElem.transNum > 1) {
        int newHeadPos = queueElem.headPos + trans->tapeMove;

        enqueue(currQueue, trans->destState, newHeadPos, queueElem.transNum - 1, tempTape);
      } else {
        tempTape->count--;

        if (!tempTape->count) {
          free(tempTape->leftTape);
          free(tempTape->rightTape);
          free(tempTape);
        }

        pathNotTerminated++;
      }

      trans = trans->next;
    }

    queueElem.currTape->count--;
    if (!queueElem.currTape->count) {
      free(queueElem.currTape->leftTape);
      free(queueElem.currTape->rightTape);
      free(queueElem.currTape);
    }
  }

  if (pathNotTerminated == 0) { // No path went out of max transition
    printf("0\n");
  } else {
    printf("U\n");
  }

  free(currQueue);
}

int main(int argc, char *argv[]) {
  adjList = calloc(MAX_STATES, sizeof(transition**));

  int source, destination, acceptedState;
  char read, write, move;
  char *string;

  // Discard "tr"
  while (getchar() != '\n');

  while (scanf("%d %c %c %c %d", &source, &read, &write, &move, &destination) >= 5) {
    int dMove;

    switch (move) {
    case 'R':
      dMove = 1;
      break;
    case 'L':
      dMove = -1;
      break;
    case 'S':
      dMove = 0;
    }

    add_transition(source, destination, read, write, dMove);
  }

  // Discard "acc"
  while (getchar() != '\n');

  accStates.acceptedNum = 0;
  while (scanf("%d", &acceptedState) >= 1) {
    if (accStates.acceptedNum < ACCEPTED_SIZE - 1) {
      accStates.acceptedList[accStates.acceptedNum] = acceptedState;
      accStates.acceptedNum++;
    }
  }

  qsort(accStates.acceptedList, accStates.acceptedNum, sizeof(int), compare);

  // Discard "max"
  while (getchar() != '\n');

  scanf("%lu", &maxTransitions);

  // Discard newline
  while (getchar() != '\n');
  // Discard "run"
  while (getchar() != '\n');

  while (scanf("%ms", &string) >= 1) {
    execute_string(string);
  }

  //free_transitions();

  return 0;
}
