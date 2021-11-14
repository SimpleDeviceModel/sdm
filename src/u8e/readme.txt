U8E is a library facilitating UTF-8 use in cross-platfrom
applications. Assuming that an application uses UTF-8 to represent
all text, U8E handles interaction with the environment, ensuring
that standard streams, file names, environment variables and
command line arguments are encoded properly. U8E implements text
codec classes which can be also used directly if needed.

On Windows U8E works by using the wide-character versions of
API functions. On other platforms it uses locale-dependent
multibyte encoding (as reported by nl_langinfo) to interact with
the environment.
