#include "defs.h"

#define PATH_MAX_LEN    100

void on_read(uv_fs_t *req);

uv_fs_t open_req;
uv_fs_t read_req;
uv_fs_t write_req;
uv_timer_t timer_req;

static char buffer[1024];

static uv_buf_t iov;
static int timeout = 0;

void on_write(uv_fs_t *req)
{
    if (req->result < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror((int)req->result));
    }
    else {
        if (!timeout)
            uv_fs_read(uv_default_loop(), &read_req, open_req.result, &iov, 1, -1, on_read);
    }
}

void on_read(uv_fs_t *req)
{
    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
    } else if (req->result == 0) {
        uv_fs_t close_req;
        // synchronous
        uv_fs_close(uv_default_loop(), &close_req, open_req.result, NULL);
    } else if (req->result > 0) {
        iov.len = req->result;
        uv_fs_write(uv_default_loop(), &write_req, 1, &iov, 1, -1, on_write);
    }
}

void on_open(uv_fs_t *req)
{
    // The request passed to the callback is the same as the one the call setup
    // function was passed.
    assert(req == &open_req);
    if (req->result >= 0) {
        iov = uv_buf_init(buffer, sizeof(buffer));
        uv_fs_read(uv_default_loop(), &read_req, req->result,
                   &iov, 1, -1, on_read);
    } else {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
    }
}

void setup_instances(struct config *cf, const char *base, const char **p)
{
    uv_fs_t req;
    char dir[PATH_MAX_LEN];
    int r;

    while (*p) {
        snprintf(dir, PATH_MAX_LEN, "%s/%s", base, *p);
        r = uv_fs_mkdir(NULL, &req, dir, 0755, NULL);
        assert(r == 0 || r == UV_EEXIST);
        uv_fs_req_cleanup(&req);
        p++;
    }
}

static void timer_expire(uv_timer_t *handle) {
    timeout = 1;
}

void record(struct config *cf, uv_loop_t *loop)
{
    uv_fs_open(loop, &open_req, "/sys/kernel/debug/tracing/instances/switch/trace_pipe", O_RDONLY, 0, on_open);
    uv_timer_init(loop, &timer_req);
    uv_timer_start(&timer_req, timer_expire, 10000, 0);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    uv_fs_req_cleanup(&open_req);
    uv_fs_req_cleanup(&read_req);
    uv_fs_req_cleanup(&write_req);
}
