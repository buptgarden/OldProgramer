#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>

#define MAX_CONNECTIONS 10000
#define THREAD_POOL_SIZE 10
#define TASK_QUEUE_SIZE 1000
#define BUFFER_SIZE 4096
#define MEMORY_POOL_SIZE 1000
#define SERVER_PORT 8080
#define MAX_EVENTS 1000

typedef enum
{
    LOG_INFO,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

typedef struct
{
    int client_fd;
    int epoll_fd;
    void (*handler)(int client_fd, int epoll_fd);
} task_t;

typedef struct memory_node
{
    void *data;
    struct memory_node *next;
    int in_use;
} memory_node_t;

typedef struct
{
    memory_node_t *nodes;
    memory_node_t *free_list;
    pthread_mutex_t mutex;
    size_t node_size;
    size_t pool_size;
    size_t used_count;
} memory_pool_t;

typedef struct
{
    pthread_t *threads;
    task_t *task_queue;
    int thread_count;
    int queue_size;
    int queue_front;
    int queue_rear;
    int task_count;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    int shutdown;
} thread_pool_t;

typedef struct
{
    int listen_fd;
    int epoll_fd;
    thread_pool_t *pool;
    memory_pool_t *memory_pool;
    int running;
    int connection_count;
    pthread_mutex_t status_mutex;
} server_t;

server_t *g_server = NULL;

void log_message(log_level_t level, const char *format, ...)
{
    time_t now;
    struct tm *tm_info;
    char timestamp[64];
    const char *level_str;

    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    switch (level)
    {
    case LOG_INFO:
        level_str = "INFO";
        break;
    case LOG_ERROR:
        level_str = "ERROR";
        break;
    case LOG_DEBUG:
        level_str = "DEBUG";
        break;
    default:
        level_str = "UNKNOWN";
        break;
    }

    printf("[%s] [%s] ", timestamp, level_str);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);
}

memory_pool_t *memory_pool_create(size_t node_size, size_t pool_size)
{
    memory_pool_t *pool = malloc(sizeof(memory_pool_t));

    if (!pool)
    {
        log_message(LOG_ERROR, "Failed to create memory pool");
        return NULL;
    }

    pool->node_size = node_size;
    pool->pool_size = pool_size;
    pool->used_count = 0;
    pool->free_list = NULL;

    pool->nodes = malloc(sizeof(memory_node_t) * pool_size);
    if (!pool->nodes)
    {
        log_message(LOG_ERROR, "Failed to create memory nodes");
        free(pool);
        return NULL;
    }

    for (size_t i = 0; i < pool_size; i++)
    {
        pool->nodes[i].data = malloc(node_size);
        if (!pool->nodes[i].data)
        {
            log_message(LOG_ERROR, "Failed to create memory node");
            for (size_t j = 0; j < i; j++)
            {
                free(pool->nodes[j].data);
            }
            free(pool->nodes);
            free(pool);
            return NULL;
        }
        pool->nodes[i].in_use = 0;
        pool->nodes[i].next = (i == pool_size - 1) ? NULL : &pool->nodes[i + 1];
    }

    pool->free_list = pool->nodes;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0)
    {
        log_message(LOG_ERROR, "Failed to initialize memory pool mutex");
        for (size_t j = 0; j < pool_size; j++)
        {
            free(pool->nodes[j].data);
        }
        free(pool->nodes);
        free(pool);
        return NULL;
    }

    log_message(LOG_INFO, "Memory Pool created: %zu nodes, %zu bytes each", pool_size, node_size);

    return pool;
}

void *memory_poll_alloc(memory_pool_t *pool)
{
    if (!pool)
    {
        return NULL;
    }

    pthread_mutex_lock(&pool->mutex);

    memory_node_t *node = pool->free_list;
    if (node)
    {
        pool->free_list = node->next;
        node->in_use = 1;
        pool->used_count++;
    }

    pthread_mutex_unlock(&pool->mutex);

    if (!pool->free_list)
    {
        pthread_mutex_unlock(&pool->mutex);
        log_message(LOG_ERROR, "Memory Pool exhausted");
        return NULL;
    }

    memory_node_t *node = pool->free_list;
    pool->free_list = node->next;
    node->in_use = 1;
    pool->used_count++;

    return node->data;
}

void memory_pool_free(memory_pool_t *pool, void *ptr)
{
    if (!pool || !ptr)
    {
        return;
    }

    pthread_mutex_lock(&pool->mutex);

    memory_node_t *node = NULL;
    for (size_t i = 0; i < pool->pool_size; i++)
    {
        if (pool->nodes[i].data == ptr)
        {
            node = &pool->nodes[i];
            break;
        }
    }

    if (node && node->in_use)
    {
        node->in_use = 0;
        node->next = pool->free_list;
        pool->free_list = node;
        pool->used_count--;
    }

    pthread_mutex_unlock(&pool->mutex);
}

void memory_pool_destroy(memory_pool_t *pool)
{
    if (!pool)
    {
        return;
    }

    pthread_mutex_lock(&pool->mutex);

    for (size_t i = 0; i < pool->pool_size; i++)
    {
        free(pool->nodes[i].data);
    }
    free(pool->nodes);

    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);

    free(pool);
    log_message(LOG_INFO, "Memory Pool destroyed");
}

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        log_message(LOG_ERROR, "Failed to get file flags %s", strerror(errno));
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        log_message(LOG_ERROR, "Failed to set file flags %s", strerror(errno));
        return -1;
    }
    return 0;
}

int add_to_epoll(int epoll_fd, int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        log_message(LOG_ERROR, "Failed to add fd %d to epoll %d", fd, epoll_fd);
        return -1;
    }

    return 0;
}

int remove_from_epoll(int epoll_fd, int fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        log_message(LOG_ERROR, "Failed to remove fd %d from epoll %d", fd, epoll_fd);
        return -1;
    }

    return 0;
}

void cleanup_connection(int fd)
{
    if (g_server)
    {
        remove_from_epoll(g_server->epoll_fd, fd);

        pthread_mutex_lock(&g_server->status_mutex);
        g_server->connection_count;
        pthread_mutex_unlock(&g_server->status_mutex);
        log_message(LOG_DEBUG, "Connection closed: fd=%d, active_connections=%d", fd, g_server->connection_count);
    }

    close(fd);
}

thread_pool_t *thread_pool_create(int thread_count, int queue_size)
{
    if (thread_count <= 0 || queue_size <= 0)
    {
        log_message(LOG_ERROR, "Invalid thread count or queue size");
        return NULL;
    }

    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (!pool)
    {
        log_message(LOG_ERROR, "Failed to allocate memory for thread pool");
        return NULL;
    }

    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->task_count = 0;
    pool->shutdown = 0;

    pool->threads = malloc(sizeof(pthread_t) * thread_count);
    if (!pool->threads)
    {
        log_message(LOG_ERROR, "Failed to allocate memory for threads");
        free(pool);
        return NULL;
    }

    pool->task_queue = malloc(sizeof(task_t) * queue_size);
    if (!pool->task_queue)
    {
        log_message(LOG_ERROR, "Failed to allocate memory for queue");
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0)
    {
        log_message(LOG_ERROR, "Failed to initialize mutex");
        free(pool->task_queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->queue_cond, NULL) != 0)
    {
        log_message(LOG_ERROR, "Failed to initialize mutex");
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool->task_queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0)
        {
            log_message(LOG_ERROR, "Failed to create worker thread %d", i);

            pool->shutdown = 1;
            pthread_cond_broadcast(&pool->queue_cond);

            for (int j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
            }

            pthread_cond_destroy(&pool->queue_cond);
            pthread_mutex_destroy(&pool->queue_mutex);
            free(pool->task_queue);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }

    log_message(LOG_INFO, "Thread pool created: %d threads, queue size %d", thread_count, queue_size);

    return pool;
}

int tread_pool_add_task(thread_pool_t *pool, task_t *task)
{
    if (!pool || !task)
    {
        log_message(LOG_ERROR, "Invalid thread pool or task");
        return -1;
    }

    pthread_mutex_lock(&pool->queue_mutex);

    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->queue_mutex);
        log_message(LOG_DEBUG, "Thread pool is shutting down, task rejected");
        return -1;
    }

    if (pool->task_count >= pool->queue_size)
    {
        pthread_mutex_unlock(&pool->queue_mutex);
        log_message(LOG_DEBUG, "Thread pool queue is full, task rejected");
        return -1;
    }

    pool->task_queue[pool->queue_rear] = *task;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_size;
    pool->task_count++;

    pthread_cond_signal(&pool->queue_cond);

    pthread_mutex_unlock(&pool->queue_mutex);
    log_message(LOG_DEBUG, "Task added to queue: fd=%d, queue_size=%d", task->client_fd, pool->task_count);
    return 0;
}

static int thread_pool_get_task(thread_pool_t *pool, task_t *task)
{
    if (!pool || !task)
    {
        log_message(LOG_ERROR, "Invalid thread pool or task");
        return -1;
    }

    pthread_mutex_lock(&pool->queue_mutex);

    while (pool->task_count == 0 && !pool->shutdown)
    {
        pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
    }

    if (pool->shutdown && pool->task_count == 0)
    {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1;
    }

    *task = pool->task_queue[pool->queue_front];
    pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
    pool->task_count--;

    pthread_mutex_unlock(&pool->queue_mutex);
    log_message(LOG_DEBUG, "Task retrieved from queue: fd=%d, queue_size=%d", task->client_fd, pool->task_count);
    return 0;
}

void *worker_thread(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    task_t task;

    while (1)
    {
        if (thread_pool_get_task(pool, &task) != 0)
        {
            break;
        }

        if (task.handler)
        {
            log_message(LOG_DEBUG, "Worker thread processing task: fd=%d", task.client_fd);
            task.handler(task.client_fd, task.epoll_fd);
        }
        else
        {
            log_message(LOG_ERROR, "Task handler is NULL");
        }
    }
    log_message(LOG_INFO, "Worker thread exiting: %lu", pthread_self());

    return NULL;
}

void thread_pool_destroy(thread_pool_t *pool)
{
    if (!pool)
    {
        log_message(LOG_ERROR, "Invalid thread pool");
        return;
    }

    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->queue_mutex);

    pthread_cond_broadcast(&pool->queue_cond);

    for (int i = 0; i < pool->thread_count; i++)
    {
        if (pthread_join(pool->threads[i], NULL) != 0)
        {
            log_message(LOG_ERROR, "Failed to join worker thread %d", i);
        }
    }

    pthread_cond_destroy(&pool->queue_cond);
    pthread_mutex_destroy(&pool->queue_mutex);
    free(pool->task_queue);
    free(pool->threads);
    free(pool);

    log_message(LOG_INFO, "Thread pool destroyed");
}

int main(int argc, char *argv[])
{

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    listen(sockfd, 10);

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);

    struct epoll_event events[10];
    while (1)
    {
        int nfds = epoll_wait(epollfd, events, 10, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(1);
        }

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == sockfd)
            {
                int connfd = accept(sockfd, NULL, NULL);
                if (connfd == -1)
                {
                    perror("accept");
                    exit(1);
                }
                printf("accept %d\n", connfd);

                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else if (events[i].events & EPOLLIN)
            {
                char buf[1024];
                int n = read(events[i].data.fd, buf, 1024);
                if (n == -1)
                {
                    perror("read");
                    exit(1);
                }
                printf("read %s\n", buf);
            }
        }
    }
}