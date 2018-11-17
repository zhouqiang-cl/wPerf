#ifndef __DEFS_H_
#define __DEFS_H_

#include <uv.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#define MAX_PATH_LEN    100

#define container_of(ptr, type, field)                                        \
  ((type *) ((char *) (ptr) - ((char *) &((type *) 0)->field)))

struct config {
    /*
     * event
     */
    char **instances_in;
    char **instances_out;
    int instances_num;

    /*
     * io worker
     */
    char ***argv;
    /*
     * filter
     */
    const char *pid_list;
    const char *disk_list;
    const char *nic_list;

    uint64_t timeout;
    const char *output_dir;
};

struct recorder;

struct event_ctx {
    uv_loop_t *loop;
    struct recorder *recorder;
    union {
        uv_fs_t open;
        uv_fs_t read;
        uv_fs_t write;
        uv_fs_t close;
    } req;
    uv_file fd[2];
    uv_buf_t iov;
};

struct ioworker {
    uv_loop_t *loop;
    uv_process_t req;
    uv_stdio_container_t child_stdio[3];
    uv_process_options_t options;
    union {
        uv_fs_t open;
        uv_fs_t close;
    } fs_req;
    uv_file fd;
};

struct recorder {
    uv_loop_t *loop;
    struct config cf;
    uv_timer_t expire_handler;
    bool expired;
    struct event_ctx *events;
    struct ioworker *workers;
    int worker_count;
};

extern const int instances_num;

void setup_event_instances(struct config *cf, const char *base, const char **p);
void setup_ioworkers(struct config *cf, struct recorder *recorder);
int recorder_run(struct config *cf, uv_loop_t *loop);
int record_events(struct recorder *recorder);
int record_ioworkers(struct recorder *recorder);

static inline void create_instance_dir(char *dir, uv_fs_t *req,
                                       const char *base, const char *name)
{
    int r;

    snprintf(dir, MAX_PATH_LEN, "%s/%s", base, name);
    r = uv_fs_mkdir(NULL, req, dir, 0755, NULL);
    assert(r == 0 || r == UV_EEXIST);
}

static inline char *get_instance_input(char *dir, const char *base,
                                       const char *name)
{
    snprintf(dir, MAX_PATH_LEN, "%s/%s/trace_pipe", base, name);
    return strdup(dir);
}

static inline void create_instance_output(char *dir, uv_fs_t *req,
                                          const char *base, const char *name)
{
    int r;

    snprintf(dir, MAX_PATH_LEN, "/%s/%s", base, name);
    r = uv_fs_mkdir(NULL, req, dir, 0755, NULL);
    assert(r == 0 || r == UV_EEXIST);
}

static inline char *get_instance_output(char *dir, const char *base, const char *name)
{
    snprintf(dir, MAX_PATH_LEN, "%s/%s/output", base, name);
    return strdup(dir);
}

#endif // __DEFS_H_
