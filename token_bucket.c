#include "token_bucket.h"

void init(struct token_bucket_system *t, int num, int B, int r)
{
    t->tokens = 0;
    t->token_index = t->packet_index = 0;
    My402ListInit(&t->Q1);
    My402ListInit(&t->Q2);
    t->B = B;
    t->number_of_packets = num;
    t->r = r;
    t->fp = 0;
    t->file_specified = 0;
}

long gettime()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

void parseInput(struct packet_attributes *pa, struct token_bucket_system *t, int argc, char *argv[])
{
    if(argc == 1)
        return;

    if(!(argc & 1))
        // number of arguments should be odd
    {
        fprintf(stderr, "Invalid syntax! The syntax is : \nwarmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num]" \
                "[-t tsfile]\n");
        exit(1);
    }

    int i;
    short flag = 0;
    for(i = 1;i < argc;i+=2)
    {
        if(argv[i][0] != '-')
        {
            fprintf(stderr, "Invalid syntax! The syntax is : \nwarmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num]" \
                    "[-t tsfile]\n");
            exit(1);
        }

        if(!strncmp(argv[i], "-lambda", 7))
        {
            if(flag & 1)
                // check if you have already encountered -lambda
            {
                fprintf(stderr, "Duplicate argument -lambda!\n");
                exit(1);
            }// end if
            flag |= 0x01;
            if(!isReal(argv[i+1]))
                // invalid argument
            {
                fprintf(stderr, "Lambda should be a real number.\n");
                exit(1);
            }
            pa->lambda = (float)strtod(argv[i+1], 0);
            if(errno == ERANGE)
            {
                fprintf(stderr, "Lambda is out of range.\n");
                exit(1);
            }
        }// end if -lambda

        else if(!strncmp(argv[i], "-mu", 3))
        {
            if(flag & 2)
                // check if you have already encountered -mu
            {
                fprintf(stderr, "Duplicate argument -mu!\n");
                exit(1);
            }// end if
            flag |= 0x02;
            if(!isReal(argv[i+1]))
                // invalid argument
            {
                fprintf(stderr, "Mu should be a real number.\n");
                exit(1);
            }
            pa->mu = (float)strtod(argv[i+1], 0);
            if(errno == ERANGE)
            {
                fprintf(stderr, "Mu is out of range.\n");
                exit(1);
            }
        }// end if -mu

        else if(!strncmp(argv[i], "-r", 2))
        {
            if(flag & 4)
                // check if you have already encountered -r
            {
                fprintf(stderr, "Duplicate argument -r!\n");
                exit(1);
            }// end if
            flag |= 0x04;
            if(!isReal(argv[i+1]))
                // invalid argument
            {
                fprintf(stderr, "r should be a real number.\n");
                exit(1);
            }
            t->r = (float)strtod(argv[i+1], 0);
            if(errno == ERANGE)
            {
                fprintf(stderr, "r is out of range.\n");
                exit(1);
            }
        }// end if -r

        else if(!strncmp(argv[i], "-B", 2))
        {
            if(flag & 8)
                // check if you have already encountered -B
            {
                fprintf(stderr, "Duplicate argument -B!\n");
                exit(1);
            }// end if
            flag |= 0x08;
            if(!isInteger(argv[i+1]))
                // invalid argument
            {
                fprintf(stderr, "B should be an integer.\n");
                exit(1);
            }
            t->B = atoi(argv[i+1]);
            if(t->B > 0x7fffffff)
            {
                fprintf(stderr, "B is out of range.\n");
                exit(1);
            }
        }// end if -B

        else if(!strncmp(argv[i], "-P", 2))
        {
            if(flag & 16)
                // check if you have already encountered -P
            {
                fprintf(stderr, "Duplicate argument -P!\n");
                exit(1);
            }// end if
            flag |= 0x10;
            if(!isInteger(argv[i+1]))
                // invalid argument
            {
                fprintf(stderr, "P should be an integer.\n");
                exit(1);
            }
            pa->P = atoi(argv[i+1]);
            if(pa->P > 0x7fffffff)
            {
                fprintf(stderr, "P is out of range.\n");
                exit(1);
            }
        }// end if -P

        else if(!strncmp(argv[i], "-n", 2))
        {
            if(flag & 32)
                // check if you have already encountered -n
            {
                fprintf(stderr, "Duplicate argument -n!\n");
                exit(1);
            }// end if
            flag |= 0x20;
            if(!isInteger(argv[i+1]))
                // invalid argument
            {
                fprintf(stderr, "n should be an integer.\n");
                exit(1);
            }
            t->number_of_packets = atoi(argv[i+1]);
            if(t->number_of_packets > 0x7fffffff)
            {
                fprintf(stderr, "n is out of range.\n");
                exit(1);
            }
        }// end if -n

        else if(!strncmp(argv[i], "-t", 2))
        {
            if(!(t->fp = fopen(argv[i+1], "r")))
            {
                fprintf(stderr, "Cannot open the file specified!\n");
                exit(1);
            }

            t->file_specified = 1;
            char line[11];
            fgets(line, 11, t->fp);
            line[strlen(line)-1] = '\0';
            if(!isInteger(line))
            {
                fprintf(stderr, "Invalid syntax in input file\n");
                exit(1);
            }
            t->number_of_packets = atoi(line);
            if(t->number_of_packets > 0x7fffffff)
            {
                fprintf(stderr, "n is out of range.\n");
                exit(1);
            }
        }// end if -t

        else
        {
            fprintf(stderr, "Invalid syntax! The syntax is : \nwarmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num]" \
                    "[-t tsfile]\n");
            exit(1);
        }
    }// end for
}

int isReal(char *str)
{
    int seenPoint = 0, i;

    for(i = 0;str[i] != '\0';i++)
    {
        if(!isdigit(str[i]))
        {
            // it should be only '.', and it should occur only once
            if(str[i] != '.' || seenPoint)
                return 0;
            seenPoint = 1;
        }
    }// end for
    return 1;
}

int isInteger(char *str)
{
    int i;

    for(i = 0;str[i] != '\0';i++)
        if(!isdigit(str[i]))
            return 0;

    return 1;
}

void tokenizeLine(char *line, float *lamda, int *nos, float *mu)
{
    char *p = strtok(line, " \t\n");

    if(!isInteger(p))
    {
        fprintf(stderr, "Error in the trace file. Exiting...\n");
        exit(1);
    }
    *lamda = 1000.0f / (float)atoi(p);

    p = strtok(0, " \t\n");
    if(!isInteger(p))
    {
        fprintf(stderr, "Error in the trace file. Exiting...\n");
        exit(1);
    }
    *nos = atoi(p);

    p = strtok(0, " \t\n");
    if(!isInteger(p))
    {
        fprintf(stderr, "Error in the trace file. Exiting...\n");
        exit(1);
    }
    *mu = 1000.0f / (float)atoi(p);
}
