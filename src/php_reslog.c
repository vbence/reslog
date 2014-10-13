/*
 * This file is part of Reslog, a PHP extension to log resource usage
 * of PHP scripts.
 *
 * Author: Varga Bence <vbence@czentral.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_reslog.h"
#include "SAPI.h"
#include "ext/standard/url.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <syslog.h>
#include <unistd.h>

static zend_function_entry reslog_functions[] = {
    PHP_FE(restest, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry reslog_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_RESLOG_EXTNAME,
    reslog_functions,
    PHP_MINIT(reslog),
    PHP_MSHUTDOWN(reslog),
    PHP_RINIT(reslog),
    PHP_RSHUTDOWN(reslog),    
    PHP_MINFO(reslog),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_RESLOG_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_RESLOG
ZEND_GET_MODULE(reslog)
#endif

PHP_INI_BEGIN()
    PHP_INI_ENTRY("reslog.file", "/tmp/php-reslog", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("reslog.hideuri", "0", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("reslog.syslog", "0", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("reslog.showhost", "0", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
    PHP_INI_ENTRY("reslog.usecanonical", "1", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
PHP_INI_END()

PHP_MINFO_FUNCTION(reslog)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "ResourceLogger Extension", "enabled");
    php_info_print_table_row(2, "Version", PHP_RESLOG_VERSION);
    php_info_print_table_end();
    
    DISPLAY_INI_ENTRIES();
} 

PHP_MINIT_FUNCTION(reslog)
{
    REGISTER_INI_ENTRIES();

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(reslog)
{
    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}

PHP_RINIT_FUNCTION(reslog)
{
    // initial usage (resources used by previous runs)
    getrusage(RUSAGE_SELF, &startusage);
    
    // request time
    gettimeofday(&tp, NULL);
    
    return SUCCESS;
}

#define LOCK_RETRY_TIME 2000
#define LOCKED_FILE_TIMEOUT 50000

PHP_RSHUTDOWN_FUNCTION(reslog)
{
    // environment variables
    const char * REQUEST_URI = "REQUEST_URI";
    const char * REMOTE_ADDR = "REMOTE_ADDR";
    const char * SERVER_NAME = "SERVER_NAME";
    const char * HTTP_HOST = "HTTP_HOST";

    const char * STRING_NA = "n/a";

    // fake uri (if reporting is disabled)
    const char * FAKE_URI = "/";
    
    // check presence of REQUEST_URI
    char * request_uri_env = sapi_getenv((char *)REQUEST_URI, strlen(REQUEST_URI));
    if (request_uri_env == NULL) {
        //return SUCCESS;
        request_uri_env = STRING_NA;
    }
    char * request_uri = INI_BOOL("reslog.hideuri") ? FAKE_URI : request_uri_env;

    // get current rusage
    struct rusage endusage;
    getrusage(RUSAGE_SELF, &endusage);
    
    struct timeval etp;
    gettimeofday(&etp, NULL);
    
    // resources uses by the current script (differences between the initial and current values)
    unsigned long u_elapsed = (endusage.ru_utime.tv_sec * 1000000 + endusage.ru_utime.tv_usec) - (startusage.ru_utime.tv_sec * 1000000 + startusage.ru_utime.tv_usec);
    unsigned long s_elapsed = (endusage.ru_stime.tv_sec * 1000000 + endusage.ru_stime.tv_usec) - (startusage.ru_stime.tv_sec * 1000000 + startusage.ru_stime.tv_usec);
    unsigned long t_elapsed = (etp.tv_sec * 1000000 + etp.tv_usec) - (tp.tv_sec * 1000000 + tp.tv_usec);
    unsigned long hardfault =  endusage.ru_majflt - startusage.ru_majflt;
    unsigned long blk_in_count =  endusage.ru_inblock - startusage.ru_inblock;
    unsigned long blk_out_count =  endusage.ru_oublock - startusage.ru_oublock;

    // peak memory usage
    unsigned long mem_peak = zend_memory_usage(0 TSRMLS_CC);
    unsigned long mem_peak_real = zend_memory_usage(1 TSRMLS_CC);
    
    // request time - something similar to Apache's style [15/Feb/2006:10:43:34 +0100]
    struct tm t;
    char stime[40];
    localtime_r(&tp.tv_sec, &t);
    strftime(stime, sizeof(stime) - 1, "%d/%b/%Y:%H:%M:%S %Z", &t);

    // buffer to build log line
    char line_buffer[1024] = "";

    // show server name (if needed)
    int showhost = INI_BOOL("reslog.showhost");
    if (showhost) {
    
        // SERVER_NAME (canonical) or HTTP_HOST
        char * server_name = INI_BOOL("reslog.usecanonical") ? SERVER_NAME : HTTP_HOST;
        if (server_name == NULL) {
            server_name = STRING_NA;
        }
        snprintf(line_buffer + strlen(line_buffer), sizeof(line_buffer) - strlen(line_buffer) - 1
            , "%s "
            , sapi_getenv(server_name, strlen(server_name)));
    }

    char * remote_addr = sapi_getenv((char *)REMOTE_ADDR, strlen(REMOTE_ADDR));
    if (remote_addr == NULL) {
        remote_addr = STRING_NA;
    }

    // fixed data
    snprintf(line_buffer + strlen(line_buffer), sizeof(line_buffer) - strlen(line_buffer) - 1
        , "%s [%s] \"%s\" %u - %u %u %u - %u %u - %u %u %u\n"
        , remote_addr, stime, request_uri, getpid()
        , u_elapsed, s_elapsed, t_elapsed
        , blk_in_count, blk_out_count
        , mem_peak, mem_peak_real, hardfault);

    int use_syslog = INI_BOOL("reslog.syslog");
    if (use_syslog) {

        syslog(LOG_INFO | LOG_USER, line_buffer);

    } else {

        // write the log
        FILE* f = fopen (INI_STR("reslog.file"), "a");

        if (f == NULL) {
            return SUCCESS;
        }

        int timeout = 0;
        int lock_error;
        do {
            lock_error = lockf(fileno(f), F_TLOCK, (off_t)0);

            if (!lock_error)
                break;

            //sleep for a bit
            usleep(LOCK_RETRY_TIME);

            //Incremement timeout
            timeout += LOCK_RETRY_TIME;

            //Check for time out
            if (timeout > LOCKED_FILE_TIMEOUT) {
                return;
            }

        } while (lock_error == EACCES || lock_error == EAGAIN);

        fwrite(line_buffer, strlen(line_buffer), 1, f);

        fflush(f);
        lockf(fileno(f), F_ULOCK, 0);

        fclose(f);
    }

    return SUCCESS;
}

PHP_FUNCTION(restest)
{
    RETURN_LONG(0);
}
