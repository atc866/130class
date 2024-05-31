#Assignment 3 directory

This directory contains source code and other files for Assignment 3.

Andrew Chu assignment 3 rwlock.c queue.c
used pseudo code from the lecture slides for queue.c
used psuedo code from Operating Systems Principles and Practice, Volume 2: Concurrency for rwlock.c

Implementation for queue.c:
*utilize cond vars to ensure thread cannot pop/push from empty/full queue
*utilize count variable to keep track of number of elements in queue
*utilize modular artihmetic to keep track of where to pop/push
*utilize cond vars and mutex lock to ensure thread-saftey

Implementaion for rwlock.c
*utilize cond vars to ensure rules for the different priorities
*utilize variable to check how many readers before a write when priority is nway
*keep track of active readers/writeres and waiting readers/writers to make sure no starvation and to ensure correct order of accessing
*use a lot of if statements to account for various scenarious that can happen when unlocking->ensure correct singal/broadcast
