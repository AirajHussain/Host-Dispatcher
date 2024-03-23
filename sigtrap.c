#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <limits.h>
#include <sys/resource.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Function prototypes
static void SignalHandler(int);
void PrintUsage(char*);   // Print program usage
char *StripPath(char*);    // Strip path from filename

#define DEFAULT_TIME 20
#define DEFAULT_NAME "sigtrap"

//codes for text colors
#define BLACK   "\033[30m"       //represents the foreground colors
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define ON_BLACK   "\033[40m"    //represents the background colors 
#define ON_RED     "\033[41m"
#define ON_GREEN   "\033[42m"
#define ON_YELLOW  "\033[43m"
#define ON_BLUE    "\033[44m"
#define ON_MAGENTA "\033[45m"
#define ON_CYAN    "\033[46m"
#define ON_WHITE   "\033[47m"

#define NORMAL	   "\033[0m"     // not bold/underline/flashing/... 

char *colours [] = { BLACK ON_WHITE, CYAN ON_RED, GREEN ON_MAGENTA,
                      BLUE ON_YELLOW, BLACK ON_CYAN, WHITE ON_RED,
                      BLUE ON_GREEN, YELLOW ON_MAGENTA, BLACK ON_GREEN, 
                      YELLOW ON_RED, BLUE ON_CYAN, MAGENTA ON_WHITE,
                      BLACK ON_YELLOW, GREEN ON_RED, BLUE ON_WHITE, 
                      CYAN ON_MAGENTA, 
                      WHITE ON_BLACK, RED ON_CYAN, MAGENTA ON_GREEN, 
                      YELLOW ON_BLUE, CYAN ON_BLACK, RED ON_WHITE, 
                      GREEN ON_BLUE, MAGENTA ON_YELLOW, GREEN ON_BLACK, 
                      RED ON_YELLOW, CYAN ON_BLUE, WHITE ON_MAGENTA,
                      YELLOW ON_BLACK, RED ON_GREEN, WHITE ON_BLUE, 
                      MAGENTA ON_CYAN };
#define N_COLOUR 32

char *colour;                        

static int signal_SIGINT = FALSE;     // flags set by signal handler
static int signal_SIGQUIT = FALSE;    
static int signal_SIGHUP = FALSE;     
static int signal_SIGTERM = FALSE;
static int signal_SIGABRT = FALSE;
static int signal_SIGCONT = FALSE;
static int signal_SIGTSTP = FALSE;

/*******************************************************************/

int main(int argc, char *argv[])
{
    pid_t pid = getpid();             //receives the process ID 
    int i, cycle, rc;    
    long clktck = sysconf(_SC_CLK_TCK);
    struct tms t;
    clock_t starttick, stoptick;
    sigset_t mask;
    
    colour = colours[pid % N_COLOUR]; // Selects the color for this process
	
    if (argc > 2 || (argc == 2 && !isdigit((int)argv[1][0])))
        PrintUsage(argv[0]);	
	
    fprintf(stdout,"%s%7d; START" BLACK NORMAL "\n", colour, (int) pid);
    fflush(stdout);	
		
    signal(SIGINT, SignalHandler);   // Hook up signal handler
    signal(SIGQUIT, SignalHandler);
    signal(SIGHUP, SignalHandler);	
    signal(SIGTERM, SignalHandler);
    signal(SIGABRT, SignalHandler);
    //  signal (SIGCONT, SignalHandler);  // do this intrinsically after return from SIGTSTP
    // due to Darwin/BSD inconsistent SIGCONT behaviour
    signal(SIGTSTP, SignalHandler);
                                        	
    rc = setpriority(PRIO_PROCESS, 0, 20); // Be nice, lower priority by 20 	
    cycle = argc < 2 ? DEFAULT_TIME : atoi(argv[1]);  // Get tick count 
    if (cycle <= 0) cycle = 1;

    for (i = 0; i < cycle;) {          // Tick 

        if (signal_SIGCONT) {
            signal_SIGCONT = FALSE;
            fprintf(stdout,"%s%7d; SIGCONT" BLACK NORMAL "\n", colour, (int) pid);
            fflush(stdout);
        }
            
        starttick = times (&t);        // Uses a timer to ascertain whether 'tick' should be
        rc = sleep(1);                 
        stoptick = times (&t);
         
        if (rc == 0 || (stoptick-starttick) > clktck/2)
            fprintf(stdout,"%s%7d; tick %d" BLACK NORMAL "\n", colour, (int) pid, ++i);
                
        if (signal_SIGINT) {
            fprintf(stdout,"%s%7d; SIGINT" BLACK NORMAL "\n", colour, (int) pid);
            exit(0);
        }
        if (signal_SIGQUIT) {
            fprintf(stdout,"%s%7d; SIGQUIT" BLACK NORMAL "\n", colour, (int) pid);
            exit(0);
        }
        if (signal_SIGHUP) {
            fprintf(stdout,"%s%7d; SIGHUP" BLACK NORMAL "\n", colour, (int) pid);
            exit(0);
        }
        if (signal_SIGTSTP) {
            signal_SIGTSTP = FALSE;
            fprintf(stdout,"%s%7d; SIGTSTP" BLACK NORMAL "\n", colour, (int) pid);
            fflush(stdout);
            sigemptyset (&mask);            // Unblock SIGSTP if necessary (BSD/OS X)
            sigaddset (&mask, SIGTSTP);
            sigprocmask (SIG_UNBLOCK, &mask, NULL);
            signal(SIGTSTP, SIG_DFL);       // Reset trap to default
            raise (SIGTSTP);                // Now suspend ourselves
            signal(SIGTSTP, SignalHandler); // Reset trap on return from suspension
            signal_SIGCONT = TRUE;          // Set flag here rather than trap signal
        }
        if (signal_SIGABRT) {
            fprintf(stdout,"%s%7d; SIGABRT" BLACK NORMAL "\n", colour, (int) pid);
            fflush(stdout);
            signal (SIGABRT, SIG_DFL);
            raise (SIGABRT);
        }
        if (signal_SIGTERM) {
            fprintf(stdout,"%s%7d; SIGTERM" BLACK NORMAL "\n", colour, (int) pid);
            exit(0);
        }                
        fflush(stdout);
    }
    exit(0);
}

  
static void SignalHandler(int sig)        // Trap signals from shell/system 
{	
    switch (sig) {
        case SIGINT:
            signal_SIGINT = TRUE;
            break;
        case SIGQUIT:
            signal_SIGQUIT = TRUE;
            break;
        case SIGHUP:
            signal_SIGHUP = TRUE;
            break;
        case SIGCONT:
            signal_SIGCONT = TRUE;
            break;
        case SIGTSTP:
            signal_SIGTSTP = TRUE;
            break;
        case SIGABRT:
            signal_SIGABRT = TRUE;
            break;
        case SIGTERM:
            signal_SIGTERM = TRUE;
            break;
    }
}

/*******************************************************************
   
  void PrintUsage(char * pgmName)
  
  Print program usage

 *******************************************************************/
 
void PrintUsage(char * pgmName)
{
    char * actualName;
    
    if (!(actualName = StripPath(pgmName))) actualName = DEFAULT_NAME;
    
    printf("\n"
           "  program: %s - trap and report process control signals\n\n"
           "    usage:\n\n"
           "      %s [seconds]\n\n"
           "      where [seconds] is the lifetime of the program - default = 20s.\n\n"
           "    the program sleeps for a second, reports process id and tick count\n"
           "    before sleeping again. any process control signals: SIGINT, SIGQUIT\n"
           "    SIGHUP, SIGTERM, SIGABRT, SIGCONT, SIGTSTP, are trapped and\n"
           "    reported before being actioned.\n\n",
           actualName, actualName );
    exit(127);
}


char * StripPath(char * pathname)
{
    char * filename = pathname;

    if (filename && *filename) {           // Non-zero length string
        filename = strrchr(filename, '/'); // Look for last '/'
        if (filename)                      // Found it
            if (*(++filename))             //  AND file name exists
                return filename;
            else
                return NULL;
        else
            return pathname;               // No '/' but non-zero length string
    }                                      // Original must be file name only
    return NULL;
}

