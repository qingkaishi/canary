/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef EVENT_HPP
#define	EVENT_HPP

#define READ 0
#define WRITE 1
#define ACQUIRE 2
#define RELEASE 3
#define WAIT 4
#define NOTIFY 5
#define FORK 6
#define JOIN 7

typedef struct Event {
    int eid;
    pthread_t tid; //unsigned long int %lu
    long mem;
    int type;
    long line;
    int * locks;
    int lock_num;
    pthread_t synctid;
    pthread_cond_t * cond;
    char srcfile[100];
} Event;


#endif	/* EVENT_HPP */

