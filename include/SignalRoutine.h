/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* Should be defined */
void sigroutine(int dunno);

void initializeSigRoutine() {
    signal(SIGHUP, sigroutine);
    signal(SIGINT, sigroutine);
    signal(SIGQUIT, sigroutine);
    signal(SIGKILL, sigroutine);
    signal(SIGSEGV, sigroutine);
}

void printSigInformation(int dunno) {
    switch (dunno) {
        case SIGHUP:
            printf("\nGet a signal -- SIGHUP\n");
            break;
        case SIGINT:
            printf("\nGet a signal -- SIGINT\n");
            break;
        case SIGQUIT:
            printf("\nGet a signal -- SIGQUIT\n");
            break;
        case SIGKILL:
            printf("\nGet a signal -- SIGKILL\n");
            break;
        case SIGSEGV:
            printf("\nSegment Fault! Core Dump!\n");
            break;
    }
}

