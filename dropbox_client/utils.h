

#ifndef DROPBOX_CLIENT_UTILS_H
#define DROPBOX_CLIENT_UTILS_H

#define DEBUG
#ifdef DEBUG
#define debug_log(...) { fprintf(stderr, "[process %d] %s @ line %d : ", getpid(), __PRETTY_FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fputs("\n", stderr); }
#else
#define debug_log(...)
#endif

#endif //DROPBOX_CLIENT_UTILS_H
