#ifndef _COMPRESSION_CONFIG_H
#define _COMPRESSION_CONFIG_H 1

#define INBUFENTRIES 1024 /* max. elements in input buffer */
#define DEFAULT_BITDEPTH 17 /* should be optimal for 100 kevents/Sec */
#define DEFAULT_STATEMASK 0xf /* take last four bits of detector_value */
#define TYPE2_BUFFERSIZE (1<<20)  /* should be sufficient for 700kcps events */
#define TYPE3_BUFFERSIZE (1<<18)  /* plenty for 700 kcps */
#define DEFAULT_FIRSTEPOCHDELAY 10 /* first epoch delay */


#define FNAMELENGTH 200  /* length of file name buffers */
#define FNAMEFORMAT "%200s"   /* for sscanf of filenames */
#define FILE_PERMISSIONS 0644  /* for all output files */

// struct timeval timeout = {0,500000}; /* timeout for input */
// #define RETRYREADWAIT 500000 /* sleep time in usec after an empty read */

struct timeval timeout = {0,5}; /* timeout for input */
#define RETRYREADWAIT 5 /* sleep time in usec after an empty read */

#endif