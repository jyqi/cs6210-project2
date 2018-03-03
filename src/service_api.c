/*************************************************************************\
*                     Copyright (C) Nathan Hicks, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/
#include "service_api.h"

/**
* service_rotate() - request caesar encode/decode from caesar service
* @client_q_name:  The base name of the client
* @message: a character array containing the message to be encoded/decoded
* @shift: a positive or negative direction to shift the message, where
*         positive values shift the message right, and negative values shift
*         the message left.
*
* Implements protocol following initial client registration.
*
*/
void service_rotate(const char client_q_name[], char message[], int shift)
{
    /* For shared memory */
    struct shared_memory *shared_mem_ptr;
    struct stat sb;
    int fd_shm;

    /* Semaphore */
    sem_t *mutex_sem;

    /* Client queue variables */
    mqd_t mqd_send, mqd_receive;
    void *buffer;
    struct mq_attr attr;
    ssize_t numRead;
    unsigned int priority;
    ssize_t bytes;

    /* Client Queue send/receive queue names */
    char client_q_send_name[256];
    char client_q_receive_name[256];

    strncpy(client_q_receive_name, "/mq_received_by_", strlen("/mq_received_by_"));
    strcat(client_q_receive_name, client_q_name);
    strncpy(client_q_send_name, "/mq_sent_from_", strlen("/mq_sent_from_"));
    strcat(client_q_send_name, client_q_name);

    /* First write message and shift to shared memory. */
    fprintf(stderr, RED"**Service API (service_rotate):"RESET" Writing '%s' with shift of '%d' to %s.\n", message, shift, SHM_NAME);

    /* Get shared memory */
    if ((fd_shm = shm_open (SHM_NAME, O_RDWR, 0)) == -1)
      error_exit("shm_open");

    if (( shared_mem_ptr = mmap(NULL, sizeof (struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
      error_exit("mmap");
    fprintf(stderr, "Shared memory virtual address mapping is at %p for %s\n", (void *)shared_mem_ptr, SHM_NAME);

    // Semaphore for mutual exclusion - initial value is 0
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED)
      error_exit("sem_open");

    if (sem_wait (mutex_sem) == -1)
      error_exit ("sem_wait: mutex_sem");

    /* Critical Section */
    snprintf(shared_mem_ptr->message, strlen(message)+1, "%s", message);
    shared_mem_ptr->shift = shift;

    /* Release mutex sem */
    if (sem_post (mutex_sem) == -1)
      error_exit ("sem_post: mutex_sem");

    /* Now open the client queue registered with the service and send a 'caesar' instruction to the service. */
    mqd_send = mq_open(client_q_send_name, O_RDWR);
    if (mqd_send == (mqd_t) -1)
        error_exit("mq_open");

    priority = 0;
    if(mq_send(mqd_send, "caesar", strlen("caesar"), priority))
        error_exit("mq_send");
    fprintf(stderr, GREEN"++ %s Queue:"RESET" Sent 'caesar' instruction to service.\n", client_q_send_name);

    mqd_receive = mq_open(client_q_receive_name, O_RDONLY);
    if (mqd_receive == (mqd_t) -1)
        error_exit("mq_open");

    /* Now receive 'fin' response saying the text has been encoded */
    if (mq_getattr(mqd_receive, &attr) == -1)
      error_exit("mq_getattr (service_rotate)");

    buffer = malloc(attr.mq_msgsize);
    if (buffer == NULL)
      error_exit("malloc (service_rotate buffer)");

    numRead = mq_receive(mqd_receive, buffer, attr.mq_msgsize, &priority);
    if (numRead == -1)
        error_exit("mq_receive (registration queue)");
    fprintf(stderr, GREEN"++ %s Queue:"RESET" Read %ld bytes; priority = %u\n", client_q_receive_name, (long) numRead, priority);

    /* Encoded data received, now output result */
    if ((bytes = write(STDOUT_FILENO, buffer, numRead)) == -1)
      error_exit("write (client buffer from service_rotate)");
    bytes = write(STDOUT_FILENO, "\n", 1);

    if(strncmp(buffer, "fin", strlen(buffer)) == 0) {

        if (sem_wait (mutex_sem) == -1)
          error_exit ("sem_wait: mutex_sem");

        /* Get shared memory */
        if ((fd_shm = shm_open (SHM_NAME, O_RDONLY, 0)) == -1)
          error_exit("shm_open");

        if (fstat(fd_shm, &sb) == -1)
            error_exit("fstat");

        if (( shared_mem_ptr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
          error_exit("mmap");
        fprintf(stderr, "Shared memory virtual address mapping is at %p for %s\n", (void *)shared_mem_ptr, SHM_NAME);
        fprintf(stderr, RED"**Service API (service_rotate):"RESET" Encoded/Decoded message is: %s\n", shared_mem_ptr->message);

        /* Release mutex sem */
        if (sem_post (mutex_sem) == -1)
          error_exit ("sem_post: mutex_sem");

        if (close(fd_shm) == -1)
            error_exit("close");

        if (munmap (shared_mem_ptr, sizeof (struct shared_memory)) == -1)
          error_exit("munmap");
    }

    free(buffer);
    mq_close(mqd_send);
    mq_close(mqd_receive);
}

/**
* service_register() - register client queues with service
* @client_q_name:  The base name of the client
* @priority_arg: defaults to 0, priority provided optionally as a command-line argument
*
* Implements registration protocol with service by first sending the client base name,
* and waiting for an "ack" from service.
*/
void service_register(const char client_q_name[], int priority_arg)
{
    unsigned int priority;
    ssize_t numRead;
    void *buffer;
    struct mq_attr attr, *attrp;
    ssize_t bytes;
    mqd_t mqd;

    char client_q_send_name[256];
    char client_q_receive_name[256];
    strncpy(client_q_receive_name, "/mq_received_by_", sizeof("/mq_received_by_"));
    strcat(client_q_receive_name, client_q_name);
    strncpy(client_q_send_name, "/mq_sent_from_", sizeof("/mq_sent_from_"));
    strcat(client_q_send_name, client_q_name);

    attrp = NULL;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 2048;

    /* Create client send queue and close for now. */
    mqd = mq_open(client_q_send_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    if (mqd == (mqd_t) -1)
        error_exit("mq_open");
    mq_close(mqd);

    /* Open Registration queue to register client */
    mqd = mq_open(REG_MQ_NAME, O_RDWR, S_IRUSR | S_IWUSR, attrp);
    if (mqd == (mqd_t) -1)
        error_exit("mq_open");

    fprintf(stderr, RED"**Service API (service_register):"RESET" Registering '%s' with the service.\n", client_q_name);

    // First Stage of QoS -- setting priority for registration
    if(priority_arg > 0) {
        priority = priority_arg;
    } else {
        priority = 0;
    }

    if(mq_send(mqd, client_q_name, strlen(client_q_name), priority) == -1)
        error_exit("mq_send");
    mq_close(mqd);
    fprintf(stderr, GREEN"++%s Queue:"RESET" Sent '%s'\n", REG_MQ_NAME, client_q_name);

    /* Now open/creat client receive queue to receive ack from service */
    fprintf(stderr, GREEN"++%s Queue:"RESET" Listening...\n", client_q_receive_name);
    mqd = mq_open(client_q_receive_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    if (mqd == (mqd_t) -1)
        error_exit("mq_open");

    if(mq_getattr(mqd, &attr) == -1)
        error_exit("mq_getattr");

    buffer = malloc(attr.mq_msgsize);
    if (buffer == NULL)
        error_exit("mq_getattr");

    priority = 0;
    numRead = mq_receive(mqd, buffer, attr.mq_msgsize, &priority);
    if (numRead == -1)
        error_exit("mq_receive");

    fprintf(stderr, GREEN"++%s Queue:"RESET" Read %ld bytes; priority = %u\n", client_q_receive_name, (long) numRead, priority);
    if ((bytes = write(STDOUT_FILENO, buffer, numRead)) == -1)
        error_exit("write");
    bytes = write(STDOUT_FILENO, "\n", 1);

    free(buffer);

    /* closing client receive queue */
    mq_close(mqd);
}

/**
* service_deregister() - deregister client queues
* @client_q_name:  The base name of the client
*
* Calls mq_unlink on the send and receive queues associated with the base name
*
*/
void service_deregister(const char client_q_name[])
{
    char client_q_send_name[256];
    char client_q_receive_name[256];
    strncpy(client_q_receive_name, "/mq_received_by_", sizeof("/mq_received_by_"));
    strcat(client_q_receive_name, client_q_name);
    strncpy(client_q_send_name, "/mq_sent_from_", sizeof("/mq_sent_from_"));
    strcat(client_q_send_name, client_q_name);

    fprintf(stderr, RED"**Service API (service_deregister):"RESET" unlinking %s and %s\n", client_q_send_name, client_q_receive_name);

    // Unlink Client Queue
    if(mq_unlink(client_q_receive_name) == -1)
        error_exit("mq_unlink (client_q_receive_name) in service_deregister");
    if(mq_unlink(client_q_send_name) == -1)
        error_exit("mq_unlink (client_q_send_name) in service_deregister");
}
