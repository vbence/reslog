PHP_ARG_ENABLE(reslog, whether to enable Resource Logger support,
[ --enable-reslog   Enable Resource Logger support])

if test "$PHP_RESLOG" = "yes"; then
  AC_DEFINE(HAVE_RESLOG, 1, [Whether you have Resource Logger])
  PHP_NEW_EXTENSION(reslog, php_reslog.c, $ext_shared)
fi