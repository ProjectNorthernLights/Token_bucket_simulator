#ifndef TOKEN_BUCKET_H
#define TOKEN_BUCKET_H

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "my402list.h"

struct token_bucket_system{
    My402List Q1, Q2;
    int tokens;
    int packet_index, token_index;
    int number_of_packets;
    int B;
    float r;
    FILE *fp;
    int file_specified;
};

struct packet_attributes{
    int P;
    float lambda, mu;
};

struct packet{
    int index;
    long start_time, arrival_time; // in microseconds
    float mu;
    int P;
 };

void init(struct token_bucket_system *t, int num, int B, int r);

long gettime();

void parseInput(struct packet_attributes *pa, struct token_bucket_system *t, int argc, char *argv[]);

int isReal(char *str);
int isInteger(char *str);

void tokenizeLine(char *line, float *lamda, int *nos, float *mu);

#endif // TOKEN_BUCKET_H
