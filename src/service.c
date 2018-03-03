/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#include <syslog.h> /* Use of logging facilities */
#include <signal.h> /* Signal Handling */
#include <unistd.h> /* Needed for sleep */
#include <sys/mman.h> /* mmap and munmap */
#include <sys/stat.h>   /* Defines mode constants */
#include <fcntl.h>  /* Defines file descriptor constants: O_ */
#include <mqueue.h>   /* Required to implement POSIX message queues */
#include <semaphore.h> /* Needed for semaphore */

#include "caesar.h" /* Defines Caesar Cipher functions */
#include "errors.h" /* Custom Error functions */

/* Used for color in Linux terminal output */
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"

/* For Shared Memory and Message Queues */
#define SHM_NAME "/shm_caesar"
#define REG_MQ_NAME "/mq_registration"
#define SEM_MUTEX_NAME "/sem_mutex"
#define BUFSIZE 256
mqd_t registration_mqd;

struct shared_memory {
  char message[BUFSIZE+1];
  int shift;
};

/* API Declarations */
int daemonize(void);

/* Handle CTRL+C SIGINT signal */
void interrupt_handler(int signo);

/* Cleans up shared memory and message queues, and closes syslog */
void clean_up(void);
void clean_up(void)
{
    if (shm_unlink(SHM_NAME) == -1)
      error_exit("shm_unlink in clean_up");

    mq_close(registration_mqd);
    if (mq_unlink(REG_MQ_NAME) == -1)
      error_exit("mq_unlink in clean_up");

    if (sem_unlink(SEM_MUTEX_NAME) == -1)
      error_exit("sem_unlink in clean_up");

    closelog();
}

/**
* interrupt_handler() - registered as handler for SIGINT
*
* This function was written to ensure shared memory and message queues
* are cleaned up when the service is interrupted with a CTRL+C from the
* terminal.
*
*/
void interrupt_handler(int signo)
{
    char c;
    char message[35] = "Do you really want to quit? [y/n] ";
    ssize_t bytes;
    signal(signo, SIG_IGN);
    bytes = write(STDOUT_FILENO, message, sizeof(message));
    c = getchar();
    if (( c == 'y' || c == 'Y')) {
        clean_up();
        exit(EXIT_SUCCESS);
    } else {
        signal(SIGINT, interrupt_handler);
    }
    getchar(); // Get new line character
}

/**
* daemonize() - daemonize the service
*
* This function daemonizes the process if the -d flag is passed
* to the program from the command-line.
*
*/
int
daemonize(void)
{
    pid_t pid;
    int fd;

    /* 1) Fork off the parent process */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE); /* fork error */
    if (pid > 0)
        exit(EXIT_SUCCESS); /* parent exits */
    /* child (daemon) continues */

    /* 2) Child calls setsid() */
    if(setsid() < 0) /* obtain a new process group */
        exit(EXIT_FAILURE);

    /* Fork off for the second time
     * 3) Ensures that (grand)child does not become session leader */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE); /* fork error */
    if (pid > 0)
        exit(EXIT_SUCCESS); /* parent exits */

    /* 4) Set new file permissions */
    umask(0);

    /* 5) Ensures that sub-directories can be unmounted */
    if (chdir("/") < 0)
        exit(EXIT_FAILURE);

    /* 6) Close all open file descriptors */
    for (fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--)
    {
        close(fd);
    }
    close(STDIN_FILENO);

    /* 7) Reopen file descriptors 0, 1, 2, to /dev/null */
    fd = open("/dev/null", O_RDWR);

    if (fd != STDIN_FILENO)
        return -1;
    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
        return -1;
    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
        return -1;

    openlog ("daemon", LOG_PID, LOG_DAEMON);
    return 0;
}

int
main(int argc, char **argv)
{
    /* For shared memory */
    struct shared_memory *shared_mem_ptr;
    int fd_shm, fd_log;
    char msg_buffer[256];

    /* Semaphore */
    sem_t *mutex_sem;

    /* For registration queue */
    void *reg_buffer;
    struct mq_attr reg_attr, *reg_attrp;
    int reg_flags;
    mode_t reg_perms;
    ssize_t numRead;
    ssize_t bytes;
    unsigned int reg_prio;// on Linux the max priority is 32,768; see sysconf(_SC_MQ_PRIO_MAX);

    /* For client queues */
    mqd_t client_mqd;
    struct mq_attr cli_attr, *cli_attrp;
    int cli_flags;
    void *cli_buffer;
    mode_t cli_perms;
    unsigned int cli_prio;
    char *client_q_receive_name;
    char *client_q_send_name;
    char client_q_name[BUFSIZE];

    int opt;

    // Register interrupt_handler to catch SIGINT from CTRL+C interrupts
    signal(SIGINT, interrupt_handler);

    /* Parse Command-Line Multiple-character Arguments */
    if (argc > 1 && !strcmp(argv[1],"--help")) {
      usage_error(argv[0], SERVICE);
      exit(EXIT_SUCCESS);
    } else if (argc > 1 && !strcmp(argv[1],"--version")) {
      printf("Caesar Service v0.1\n");
      exit(EXIT_SUCCESS);
    }

    /* Parse Command-Line Flag Arguments */
    while ((opt = getopt(argc, argv, "hd")) != -1) {
        switch (opt) {
            case 'h':
                usage_error(argv[0], SERVICE);
                return EXIT_SUCCESS;
                break;
            case 'd': /* daemonize */
                daemonize();
                break;
            default:
                usage_error(argv[0], SERVICE);
                return EXIT_FAILURE;
        }
    }

    // Semaphore for mutual exclusion - initial value is 0
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
      error_exit("sem_open");

    /* Creates a shared memory object in /dev/shm on Linux and maps into shared memory */
    fprintf(stderr, RED"**Service:"RESET" Creating POSIX Shared Memory named '%s' at /dev/shm (on Linux)\n", SHM_NAME);
    if ((fd_shm = shm_open (SHM_NAME, O_CREAT | O_RDWR, 0660)) == -1)
      error_exit("shm_open");

    if (ftruncate (fd_shm, sizeof (struct shared_memory)) == -1)
      error_exit("ftruncate");

    if (( shared_mem_ptr = mmap(NULL, sizeof (struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
      error_exit("mmap");
    fprintf(stderr, "Shared memory address is %p\n", (void *)shared_mem_ptr);

    /* Initialize shift value to 0 (no shifting occurs) */
    shared_mem_ptr -> shift = 0;

    /* Unlock mutex semaphore now that shared memory set up is complete and ready for use */
    if (sem_post (mutex_sem) == -1)
      error_exit ("sem_post: mutex_sem");

    /* Create a message queue for clients to register with the service */
    fprintf(stderr, RED"**Service:"RESET" Creating POSIX Message Queue named '%s' at /dev/mqueue (on Linux)\n", REG_MQ_NAME);
    reg_attrp = NULL;
    reg_attr.mq_maxmsg = 10;
    reg_attr.mq_msgsize = 2048;
    reg_attrp = &reg_attr;
    reg_flags = O_CREAT | O_RDWR;
    reg_perms = S_IRUSR | S_IWUSR;
    registration_mqd = mq_open(REG_MQ_NAME, reg_flags, reg_perms, &reg_attr);
    if (registration_mqd == (mqd_t) -1)
      error_exit("mq_open (registration) line 221");

    /* Set up a registration buffer for mq_receive() */
    if (mq_getattr(registration_mqd, &reg_attr) == -1)
      error_exit("mq_getattr (registration)");
    reg_buffer = malloc(reg_attr.mq_msgsize);
    if (reg_buffer == NULL)
      error_exit("malloc (reg_buffer)");

    /* Set up Client message queue variables */
    cli_attrp = NULL;
    cli_attr.mq_maxmsg = 10;
    cli_attr.mq_msgsize = 2048;
    cli_attrp = &cli_attr;
    cli_flags = O_RDWR;
    cli_perms = S_IRUSR | S_IWUSR;
    cli_prio=0; // on Linux the max priority is 32,768; see sysconf(_SC_MQ_PRIO_MAX);

    fprintf (stderr, RED"**Service:"RESET" Entering main event loop.\n");
    /* Main Event Loop */
    while (1)
    {
        /* 1) Check Registration Queue for new Clients - this is a blocking call */
        numRead = mq_receive(registration_mqd, reg_buffer, reg_attr.mq_msgsize, &reg_prio);
        if (numRead == -1)
          error_exit("mq_receive (registration queue)");

        fprintf(stderr, GREEN"++%s Queue:"RESET" Read %ld bytes; priority = %u\n", REG_MQ_NAME, (long) numRead, reg_prio);
        if ((bytes = write(STDOUT_FILENO, reg_buffer, numRead)) == -1)
          error_exit("write (registration reg_buffer)");
        bytes = write(STDOUT_FILENO, "\n", 1);

        memset(client_q_name, '\0', BUFSIZE);
        strncpy(client_q_name, (char *)reg_buffer, BUFSIZE);

        /* 2) Set client message queue names by the name provided on the registration queue */
        client_q_send_name = malloc(BUFSIZE);
        client_q_receive_name = malloc(BUFSIZE);
        strncpy(client_q_receive_name, "/mq_received_by_", strlen("/mq_received_by_"));
        strcat(client_q_receive_name, client_q_name);
        strncpy(client_q_send_name, "/mq_sent_from_", strlen("/mq_sent_from_"));
        strcat(client_q_send_name, client_q_name);

        fprintf(stderr, RED"**Service:"RESET" Opening client queue, '%s'\n", client_q_receive_name);
        /* Open received by client queue to send an ack */
        client_mqd = mq_open(client_q_receive_name, cli_flags, cli_perms, cli_attrp);
        if (client_mqd == (mqd_t) -1)
          error_exit("mq_open (client)");

        /* 3) Send 'ack' reply on client receive queue */
        fprintf(stderr, GREEN"++%s Queue:"RESET" Sending ack\n", client_q_receive_name);
        if(mq_send(client_mqd, "ack", sizeof("ack"), reg_prio) == -1)
          error_exit("mq_send ack (client receive queue)");
        mq_close(client_mqd);
        fprintf(stderr, RED"**Service:"RESET" Closed client queue, '%s'\n", client_q_receive_name);

        fprintf(stderr, RED"**Service:"RESET" Opening client queue, '%s'\n", client_q_send_name);
        /* Open sent by client queue to receive 'caesar' instruction */
        client_mqd = mq_open(client_q_send_name, cli_flags, cli_perms, cli_attrp);
        if (client_mqd == (mqd_t) -1)
          error_exit("mq_open (client)");

        if (mq_getattr(client_mqd, &cli_attr) == -1)
          error_exit("mq_getattr (client)");
        cli_buffer = malloc(cli_attr.mq_msgsize);
        if (cli_buffer == NULL)
          error_exit("malloc (cli_buffer)");

        numRead = mq_receive(client_mqd, cli_buffer, cli_attr.mq_msgsize, &cli_prio);
        if (numRead == -1)
          error_exit("mq_receive (registration queue)");
        fprintf(stderr, GREEN"++%s Queue:"RESET" Read %ld bytes; priority = %u\n", client_q_send_name, (long) numRead, cli_prio);
        if ((bytes = write(STDOUT_FILENO, cli_buffer, numRead)) == -1)
          error_exit("write (registration reg_buffer)");
        bytes = write(STDOUT_FILENO, "\n", 1);
        mq_close(client_mqd);
        fprintf(stderr, RED"**Service:"RESET" Closed client queue, '%s'\n", client_q_send_name);

        if(strncmp("caesar", cli_buffer, strlen(reg_buffer)) == 0) {
            /* 5) Process Data in shared memory (cipher/plaintext and shift value) */
            printf(RED"**Service:"RESET" rotx entered with: %s\n", shared_mem_ptr->message);

            /* Critical Section */
            rotx(shared_mem_ptr->message, shared_mem_ptr->shift);
            memcpy(shared_mem_ptr->message, shared_mem_ptr->message, strlen(shared_mem_ptr->message));
            /* End Critical Section */

            fprintf(stderr, RED"**Service:"RESET" rotx returned with: %s\n", shared_mem_ptr->message);

            /* 5) Send 'fin' message on received by client queue to let client know the data is ready */
            client_mqd = mq_open(client_q_receive_name, cli_flags, cli_perms, cli_attrp);
            if (client_mqd == (mqd_t) -1)
              error_exit("mq_open (client)");
            printf(GREEN"++%s Queue:"RESET" Sending fin\n", client_q_receive_name);
            if(mq_send(client_mqd, "fin", strlen("fin"), cli_prio) == -1)
              error_exit("mq_send fin (client queue)");

            /* 6) Close client message queue */
            mq_close(client_mqd);
            free(client_q_send_name);
            free(client_q_receive_name);
            free(cli_buffer);
            client_q_send_name = NULL;
            client_q_receive_name = NULL;
            cli_buffer = NULL;
        }
    }
    fprintf(stderr, RED"**Service:"RESET" Leaving main event loop and calling cleanup.\n");

    clean_up();
    return EXIT_SUCCESS;
}
