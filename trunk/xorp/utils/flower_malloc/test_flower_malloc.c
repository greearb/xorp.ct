
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define EXECUTION_SECS 60
#define MAX_ALLOCS 10000
#undef  INTERACTIVE_EXECUTION

const uint32_t CLEANUP = 1;
const uint32_t SOME_CLEANUP = 2;


void paquitodrivera(uint32_t cleanup);
void milesdavis(uint32_t cleanup);
void kennyg(uint32_t cleanup);
void ornette(uint32_t cleanup);
void examine_report(void);

/* REFERENCE REPORT 

***********************************************
allocs/watermark  total size/watermark     avg size  tracker_id [+]
     300/300            90000/90000           300    35f6c +
       4/5             280000/350000        70000    36664 +
       0/16                 0/100800         6300    36978 +
       0/5                  0/2500            500    36984 +
       0/120                0/252000         2100    36a6c +
       2/2              12288/12288          6144    40291  
       0/1                  0/100             100    4c5f2  

*/

#define LINES_IN_REF_REPORT 7
uint32_t ref_report[LINES_IN_REF_REPORT][6] =
{ {  300,300     ,      90000,90000    ,      300 , 0},
  {    4,5       ,     280000,350000   ,    70000 , 0},
  {    0,16      ,          0,100800   ,     6300 , 0},
  {    0,5       ,          0,2500     ,      500 , 0},
  {    0,120     ,          0,252000   ,     2100 , 0},
  {    2,2       ,      12288,12288    ,     6144 , 0},
  {    0,1       ,          0,100      ,      100 , 0},};

/* exhuberance of notes, fast and throughout the entire range of the horn */
void paquitodrivera(uint32_t cleanup)
{
    static void * allocated[MAX_ALLOCS];
    static uint32_t i;  /* points to the next available slot */
    uint32_t j; 

    if (cleanup == CLEANUP) {
	for (j=0; j < i; j++)
	    free(allocated[j]);
	i = 0;
	return;
    } else if (cleanup == SOME_CLEANUP) {   // free half of allocated ptrs

	for (j= i/2; j < i; j++)
	    free(allocated[j]);
	i /= 2;
	return;
    }

    if ( i < MAX_ALLOCS) 
	allocated[i++] = (void*) malloc(300);


}

/* thinks a lot before playing, but when he does,  uses long fat notes */
void milesdavis(uint32_t cleanup)
{
    static void * allocated[MAX_ALLOCS];
    static uint32_t i;  /* points to the next available slot */
    uint32_t j; 

    if (cleanup == CLEANUP) {
	for (j=0; j < i; j++)
	    free(allocated[j]);
	i = 0;
	return;
    } else if (cleanup == SOME_CLEANUP) {   // free half of allocated ptrs
	for (j= i/2; j < i; j++)
	    free(allocated[j]);
	i /= 2;
	return;
    }

    if ( i < MAX_ALLOCS) 
	allocated[i++] = (void*) malloc(70000);

}

/* horrible endless sound, but cleans up his mess after he's done */
void kennyg(uint32_t cleanup)
{
    static void * allocated[MAX_ALLOCS];
    static uint32_t i;  /* points to the next available slot */
    uint32_t j; 

    if (cleanup == CLEANUP) {
	for (j=0; j < i; j++)
	    free(allocated[j]);
	i = 0;
	return;
    } else if (cleanup == SOME_CLEANUP) {   // free half of allocated ptrs
	for (j= i/2; j < i; j++)
	    free(allocated[j]);
	i /= 2;
	return;
    }

    if ( i < MAX_ALLOCS) 
	allocated[i++] = (void*) malloc(2100);
}

/* total chaos, loosing meaning and substance as he goes by */
void ornette(uint32_t cleanup)
{
    static void * allocated[MAX_ALLOCS];
    static uint32_t i;  /* points to the next available slot */
    uint32_t j; 

    if (cleanup == CLEANUP) {
	for (j=0; j < i; j++)
	    free(allocated[j]);
	return;
    } else if (cleanup == SOME_CLEANUP) {   // free half of allocated ptrs
	for (j= i/2; j < i; j++)
	    free(allocated[j]);
	i /= 2;
	return;
    }

    if ( i < MAX_ALLOCS) {
	allocated[i++] = (void*) calloc(2100, 3);
	if ( i%4 == 0) allocated[i/2] = (void *) realloc(allocated[i/2], 500);
    }
}


void examine_report()
{
    char procbuf[32];
    unsigned long trk_idx, count, max_count, total_size, watermark, avg_size;
    char procname[20];
    char prog_name[32];
    int fd, assig, i;
    FILE* report;
    #define MAX_REPORT_LINE_WIDTH 80
    char ignored[MAX_REPORT_LINE_WIDTH];
    char path[64];


    int pid = (int)getpid ();
    (void)sprintf (procbuf, "/proc/%ld/status", (long)pid);
    if ((fd = open (procbuf, O_RDONLY)) != -1) {
        if (read (fd, procname, sizeof (procname)) == sizeof (procname)) {
            strtok (procname, " ");
            sprintf (prog_name, "%s.%d", procname, pid);
        } else
            sprintf (prog_name, "%s.%d", "unknown", pid);
    } else
        sprintf (prog_name, "%s.%d", "unknown", pid);
    sprintf (path, "%s%s", "./flower_report.",
        prog_name);

    report = fopen (path, "r");
    if (!report) {
	fprintf(stderr, "Could not find report file %s\n", path);
	exit(-1);
    }
    do {
        assig = fscanf(report, "%8lu/%8lu %10lu/%10lu %8lu %8lx %*[+ ]\n",
            &count, &max_count, &total_size, &watermark, &avg_size, &trk_idx);
        if (assig == 6) {
	    printf("Testing line %lu/%lu %lu/%lu %lu... ", count, max_count,
		total_size, watermark, avg_size);
	    for (i = 0; i < LINES_IN_REF_REPORT; i++)
		if ( ! ref_report[i][5] ) {
		    ref_report[i][5] = ((ref_report[i][0] == count) &&
					(ref_report[i][1] == max_count) &&
					(ref_report[i][2] == total_size) &&
					(ref_report[i][3] == watermark) &&
					(ref_report[i][4] == avg_size));
		    if (ref_report[i][5]) printf("OK");
		}
	    printf("\n");
        } else {
            fgets(ignored, MAX_REPORT_LINE_WIDTH, report);
        }

    } while (assig != EOF);
    for (i = 0; i < LINES_IN_REF_REPORT; i++)
	if ( ! ref_report[i][5] ) {
	    fprintf(stderr, "This line not found in the report:\n", path);
	    fprintf(stderr, "%u/%u   %u/%u   %u\n", ref_report[i][0],
		ref_report[i][1], ref_report[i][2],  ref_report[i][3],  
		ref_report[i][4]);
	    fprintf(stderr, "FAILED\n");
	    exit (-1);
	}
}

int main(void)
{
    int i, j;
    void * ptr;

#ifdef INTERACTIVE_EXECUTION    
    printf("to talk to me:\n\t kill -SIGUSR2 %d\n", getpid());
#endif /* INTERACTIVE_EXECUTION */

    ptr = (void*) malloc(100);
    free(ptr);

    for (i=0; i<EXECUTION_SECS; i++) {
	for (j=0; j < 10; j++) {   /* j is 1/10th of a second */
#ifdef INTERACTIVE_EXECUTION    
	    usleep(95000);  
#endif /* INTERACTIVE_EXECUTION */
	    if ( !(j%2) ) paquitodrivera(0);
	    if ( !(j%5) ) kennyg(0);
	}
	if ( !(i%2) ) milesdavis(0);
	if ( !(i%5) ) milesdavis(SOME_CLEANUP);
	if ( !(i%3) ) ornette(0);
	printf(". %u", i);
	fflush(stdout);
    }
    kennyg(CLEANUP);
    /* paquitodrivera(CLEANUP); */
    /* milesdavis(CLEANUP); */
    ornette(CLEANUP); 
    printf("\n", i);
#ifndef INTERACTIVE_EXECUTION    
    raise(SIGUSR2);    /* generate flower_malloc report */
    examine_report();
    fprintf(stderr, "PASSED\n");
#endif /* INTERACTIVE_EXECUTION */
    return 0;
}
