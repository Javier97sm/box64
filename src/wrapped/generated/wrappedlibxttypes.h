/*******************************************************************
 * File automatically generated by rebuild_wrappers.py (v2.1.0.16) *
 *******************************************************************/
#ifndef __wrappedlibxtTYPES_H_
#define __wrappedlibxtTYPES_H_

#ifndef LIBNAME
#error You should only #include this file inside a wrapped*.c file
#endif
#ifndef ADDED_FUNCTIONS
#define ADDED_FUNCTIONS() 
#endif

typedef intptr_t (*lFppp_t)(void*, void*, void*);
typedef void (*vFpuipp_t)(void*, uint64_t, int64_t, void*, void*);
typedef intptr_t (*lFpippp_t)(void*, int64_t, void*, void*, void*);

#define SUPER() ADDED_FUNCTIONS() \
	GO(XtAppAddWorkProc, lFppp_t) \
	GO(XtAddEventHandler, vFpuipp_t) \
	GO(XtAppAddInput, lFpippp_t)

#endif // __wrappedlibxtTYPES_H_
