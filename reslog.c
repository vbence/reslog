/*
 * Varga Bence
 * lanny - at - freemail.hu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_reslog.h"
#include "SAPI.h"
#include "ext/standard/url.h"
#include "ext/standard/reg.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

static function_entry reslog_functions[] = {
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

PHP_RSHUTDOWN_FUNCTION(reslog)
{
    // environment variables - constants for strlen()
    const char * REQUEST_URI = "REQUEST_URI";
    const char * REMOTE_ADDR = "REMOTE_ADDR";
	const char * SERVER_NAME = "SERVER_NAME";
    const char * HTTP_HOST = "HTTP_HOST";
    
    // check presence of REQUEST_URI    
    char * request_uri = sapi_getenv(REQUEST_URI, strlen(REQUEST_URI));
    if (request_uri == NULL)
        return SUCCESS;

    // logile-safe URI's - may be not the fastest method
    char * uri = php_reg_replace(" ", "%20", request_uri, 0, 0);

    // get current rusage
    struct rusage endusage;
    getrusage(RUSAGE_SELF, &endusage);
    
    struct timeval etp;
    gettimeofday(&etp, NULL);
    
    // resources uses by the current script (differences between the initial and current values)
    unsigned long u_elapsed = (endusage.ru_utime.tv_sec * 1000000 + endusage.ru_utime.tv_usec) - (startusage.ru_utime.tv_sec * 1000000 + startusage.ru_utime.tv_usec);
    unsigned long s_elapsed = (endusage.ru_stime.tv_sec * 1000000 + endusage.ru_stime.tv_usec) - (startusage.ru_stime.tv_sec * 1000000 + startusage.ru_stime.tv_usec);
    unsigned long t_elapsed = (etp.tv_sec * 1000000 + etp.tv_usec) - (tp.tv_sec * 1000000 + tp.tv_usec);
    
    // request time - something similar to Apache's style [15/Feb/2006:10:43:34 +0100]
    struct tm t;
    char stime[40];
    localtime_r(&tp.tv_sec, &t);
    strftime(stime, sizeof(stime) - 1, "%d/%b/%Y:%H:%M:%S %Z", &t);
    
    // log hostname? (single logfile with vrtual hosting)
    int showhost = INI_BOOL("reslog.showhost");
    
    // write the log
    FILE* f = fopen (INI_STR("reslog.file"), "a");
	
	// show server name (if needed)
    if (showhost) {
	
    	// SERVER_NAME (canonical) or HTTP_HOST
		char * server_name = INI_BOOL("reslog.usecanonical") ? SERVER_NAME : HTTP_HOST;
		fprintf(f, "%s ", sapi_getenv(server_name, strlen(server_name)));
	}
	
	// fixed data
	fprintf(f, "%s [%s] \"%s\" %u %u %u %u\n", sapi_getenv(REMOTE_ADDR, strlen(REMOTE_ADDR)), stime, uri, getpid(), u_elapsed, s_elapsed, t_elapsed);
	fclose(f);
    
    // free some resources PHP uses its own memory management (at least in php_reg_replace), so this would be freed at the end of the request anyway)
    efree(uri);

    return SUCCESS;
}

PHP_FUNCTION(restest)
{
    RETURN_LONG(0);
}
