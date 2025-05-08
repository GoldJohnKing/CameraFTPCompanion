
#ifndef FINEFTP_EXPORT_H
#define FINEFTP_EXPORT_H

#ifdef FINEFTP_STATIC_DEFINE
#  define FINEFTP_EXPORT
#  define FINEFTP_NO_EXPORT
#else
#  ifndef FINEFTP_EXPORT
#    ifdef server_EXPORTS
        /* We are building this library */
#      define FINEFTP_EXPORT 
#    else
        /* We are using this library */
#      define FINEFTP_EXPORT 
#    endif
#  endif

#  ifndef FINEFTP_NO_EXPORT
#    define FINEFTP_NO_EXPORT 
#  endif
#endif

#ifndef FINEFTP_DEPRECATED
#  define FINEFTP_DEPRECATED __declspec(deprecated)
#endif

#ifndef FINEFTP_DEPRECATED_EXPORT
#  define FINEFTP_DEPRECATED_EXPORT FINEFTP_EXPORT FINEFTP_DEPRECATED
#endif

#ifndef FINEFTP_DEPRECATED_NO_EXPORT
#  define FINEFTP_DEPRECATED_NO_EXPORT FINEFTP_NO_EXPORT FINEFTP_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef FINEFTP_NO_DEPRECATED
#    define FINEFTP_NO_DEPRECATED
#  endif
#endif

#endif /* FINEFTP_EXPORT_H */
