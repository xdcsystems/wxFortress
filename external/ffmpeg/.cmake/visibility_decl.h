#ifndef VISIBILITY_DECL_H
#define VISIBILITY_DECL_H

#ifndef DEFAULT_EXPORT
#    define DEFAULT_EXPORT __attribute__((visibility("default")))
#endif

#ifndef DEFAULT_NO_EXPORT
#  define DEFAULT_NO_EXPORT __attribute__((visibility("hidden")))
#endif

#endif

