#define ENABLE_DEBUG
#include "log.h"
#include <stdlib.h>
#include <unistd.h>
int main()
{
    log_t *log = log_create();
    log_init(log);
    log_set_file(log, "test.log", "debug.log");
    log_set_socket(log, "127.0.0.1", LOG_SOCKET_PORT_DEFAULT, TCP);
    log_dispatch(log,DISPATCH_UNBLOCK);

    LOG_FATAL(log, "%s", "test-fatal");
    LOG_DEBUG(log, "%s", "test-debug");
    LOG_INFO(log, "%s", "test-debug");
    LOG_DEBUG_TO_SOCKET(log, "%s","test-debug");
    sleep(10);
    log_stop(log);
    log_destroy(log);
    return 0;
}
