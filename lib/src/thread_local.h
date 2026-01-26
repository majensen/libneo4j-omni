#ifndef THREAD_LOCAL
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#    define THREAD_LOCAL _Thread_local
#  else
#    define THREAD_LOCAL
#  endif
#endif
