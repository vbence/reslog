ResLog
======

*ResLog* is a PHP extension to log resource usage of PHP scripts.

The data comes from the `getrusage` system call (LAMP stack required). Log messages can go to *syslog* or to a file specified.

It is possible to use *separate files* for different virtual hosts.

## Log Format

Log messages have the following format (similarly to Apache's access log):

    [ http_host SPACE ] remote_addr SPACE "[" request_time "]" SPACE <"> request_uri <"> SPACE pid SPACE u_elapsed SPACE s_elapsed SPACE t_elapsed

http_host
: Name of the *host* from the `SERVER_NAME` environment, if `reslog.usecanonical` is enabled (default). Otherwise the host name in use by the client from the `HTTP_HOST` environment.
This can be useful in case of multiple `ServerAlias` directives.

remote_addr
: IP address of the remote host (from the `REMOTE_ADDR` environment variable).

request_time
: Time of the request.

request_uri
: Full URI of this request (from the `REQUEST_URI` environment variable).

u_elapsed
: CPU time (in microseconds), *this thread* spend in *user mode*. 

s_elapsed
: CPU time (in microseconds), *this thread* spend in *system mode*. 

t_elapsed
: Elapsed time (microseconds) from starting the script to finishing.
This is not indicative of the current script's performance as other threads will have an impact on this amount.

## Compiling

Get the binary with the following commands:

    phpize
    ./configure
    make

Install it (requires root privileges):

    sudo make install

Activate the extension by adding the follwing to your *php.ini*:

    extension=reslog.so

You will have to **restart Apache** for the changes to take effect.

## Configuration

The extension has the following parameters (also settable in `.htaccess` but not with `ini_set`).

> **Note:** It is strongly recommended to use `php_admin_flag` for all the settings if you are operating a shared hosting environment.

reslog.file
: *String*, name of the file to use for log messages. (File operations will be done with the same privileges with which the script is executed).

reslog.showhost
: *Boolean*, determines whether to prefix log lines with the server's name.

reslog.syslog
: *Boolean*, if enabled all logging will be done through the *syslog* facility. (Ignoring *reslog.file* setting).

reslog.usecanonical
: *Boolean*, if enabled (default) the server's name will be read from `SERVER_NAME`, otherwise `HTTP_HOST` will be used. (Last one is relevant for virtual hosts with aliases. Please consult Apache's manual for the difference). 

## Testing Successful Installation

ResLog should show up in the output of `phpinfo` displaying configuration parameters.
