/*
 * This file is part of Reslog, a PHP extension to log resource usage
 * by PHP scripts.
 * 
 * Author: Varga Bence <vbence@czentral.org>
 */
 
#ifndef PHP_RESLOG_H
#define PHP_RESLOG_H 1

#define PHP_RESLOG_VERSION "1.0.3"
#define PHP_RESLOG_EXTNAME "ResLog"

struct rusage startusage;
struct timeval tp;

PHP_MINIT_FUNCTION(reslog);
PHP_MSHUTDOWN_FUNCTION(reslog);
PHP_RINIT_FUNCTION(reslog);
PHP_RSHUTDOWN_FUNCTION(reslog);
PHP_FUNCTION(restest);
PHP_MINFO_FUNCTION(reslog);

extern zend_module_entry reslog_module_entry;
#define phpext_reslog_ptr &reslog_module_entry

#endif
