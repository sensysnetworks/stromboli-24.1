/*
 * Copyright 1994-2001 The MathWorks, Inc.
 *
 * File: ext_svr.c     $Revision: 1.1.1.1 $
 *
 * Abstract:
 *  External mode server interface (TCPIP example).  Provides functions
 *  that get called by main routine (modelname.c):
 *    o ExtParseArgsAndInitUD:  parse args and create UserData
 *    o ExtWaitForStartMsg:     return true if waiting for host to start
 *    o rt_ExtModeInit:         external mode initialization
 *    o grt_Sleep:              pause the process (GRT only)
 *    o rt_MsgServerWork:       server for setting/getting messages from host
 *    o rt_MsgServer:           server dispatcher - for multi-tasking targets
 *    o rt_UploadServerWork:    server for setting upload data on host
 *    o rt_UploadServer:        server dispatcher - for multi-tasking targets
 *    o rt_ExtModeShutdown:     external mode termination
 *
 *  Paremter downloading and data uploading supported for single and
 *  multi-tasking targets.
 *
 * 2002 - Fix by Gopal Santhanam <gopal@nerur.com>
 */

/*****************
 * Include files *
 *****************/

/*ANSI C headers*/
#include <stdio.h>
#include <stdlib.h>

#if defined(VXWORKS)
 /*VxWorks headers*/
# include <selectLib.h>
# include <sockLib.h>
# include <inetLib.h>
# include <ioLib.h>
# include <taskLib.h>
#endif


/*Real Time Workshop headers*/
#include "tmwtypes.h"
#include "simstruc.h"
#include "ext_share.h"
#include "ext_svr_transport.h"
#include "updown.h"
#include "updown_util.h"
#include "dt_info.h"


/*Uncomment to test 4 byte reals*/
/*#define real_T float*/

#ifndef __LCC__
#define UNUSED_PARAM(p) (void)((p))
#else
#define UNUSED_PARAM(p) /* nothing */
#endif


/**********************
 * External Variables *
 **********************/
#ifndef VXWORKS
extern int_T  volatile          startModel;
#else
int_T  volatile startModel = FALSE; /* ignored */
extern SEM_ID uploadSem;
#endif

extern TargetSimStatus volatile modelStatus;

/********************
 * Global Variables *
 ********************/

/*
 * Flags.
 */
PRIVATE boolean_T   connected       = FALSE;
PRIVATE boolean_T   commInitialized = FALSE;

/*
 * Pointer to opaque user data (defined by ext_svr_transport.c).
 */
PRIVATE ExtUserData *extUD          = NULL;

/*
 * Buffer used to receive messages.
 */
PRIVATE int_T msgBufSize = 0;
PRIVATE char  *msgBuf    = NULL;

  
/*******************
 * Local Functions *
 *******************/


/* Function: GrowRecvBufIfNeeded ===============================================
 * Abstract:
 *  Allocate or increase the size of buffer for receiving messages from target.
 */
PRIVATE boolean_T GrowRecvBufIfNeeded(const int msgSize)
{
    if (msgSize > msgBufSize) {
        if (msgBuf != NULL) free(msgBuf);

        msgBufSize = msgSize;
        msgBuf     = (char *)malloc(msgBufSize);

        if (msgBuf == NULL) return(EXT_ERROR);
    }
    return(EXT_NO_ERROR);
} /* end GrowRecvBufIfNeeded */


/* Function: GetMsgHdr =========================================================
 * Abstract:
 *  Attempts to retrieve a message header from the host.  If a header is in 
 *  fact retrieved, the reference arg, 'hdrAvail' will be returned as true.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR is returned on failure.
 *
 * NOTES:
 *  o It is not necessarily an error for 'hdrAvail' to be returned as false.
 *    It typically means that we were polling for messages and none were
 *    available.
 */
PRIVATE boolean_T GetMsgHdr(MsgHeader *msgHdr, boolean_T *hdrAvail)
{
    int_T     nGot  = 0; /* assume */
    boolean_T error = EXT_NO_ERROR;
    
    /*
     * Read the message header.  Assumed that the entire message
     * header can be read in one shot.
     */
    error = ExtGetHostMsg(extUD,sizeof(MsgHeader),&nGot,(char_T *)msgHdr);
    if (error) goto EXIT_POINT;
    assert((nGot == 0) || (nGot == sizeof(MsgHeader)));

EXIT_POINT:
    *hdrAvail = (boolean_T)(nGot > 0);
    return(error);
} /* end GetMsgHdr */


/* Function: GetMsg ============================================================
 * Abstract:
 *  Receive nBytes from the host.  Return a buffer containing the bytes or
 *  NULL if an error occurs.  Note that the pointer returned is that of the
 *  global msgBuf.  If the buf needs to be grown to accommodate the message,
 *  it is realloc'd.  This function will try to get the requested number
 *  of bytes indefinately - it is assumed that the data is either already there,
 *  or will show up in a "reasonable" amount of time.
 */
PRIVATE const char *GetMsg(const int msgSize)
{
    int_T     nGot;
    boolean_T error     = EXT_NO_ERROR;
    int_T     nGotTotal = 0;

    error = GrowRecvBufIfNeeded(msgSize);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;
    
    /* Get the data. */
    while(nGotTotal < msgSize) {
        error = ExtGetHostMsg(extUD,
            msgSize - nGotTotal, &nGot, (char_T *)(msgBuf + nGotTotal));
        if (error) {
	    fprintf(stderr,"ExtGetHostMsg() failed.\n");
            goto EXIT_POINT;
	}

	nGotTotal += nGot;
    }

EXIT_POINT:
    return((error == EXT_NO_ERROR) ? msgBuf : NULL);
} /* end GetMsg */


boolean_T rt_UploadServerWork(SimStruct *S); /* forward declaration */


/* Function: DisconnectFromHost ================================================
 * Abstract:
 *  Disconnect from the host.
 */
PRIVATE void DisconnectFromHost(SimStruct *S)
{
    UploadPrepareForFinalFlush();
#ifdef VXWORKS
    /*
     * Patch by Gopal Santhanam 5/24/2002 (for VXWORKS) We
     * were having problems in RTAI in that the semaphore
     * signaled in UploadPrepareForFinalFlush was taken up by
     * the upload server task.  This meant that the subsequent
     * call to rt_UploadServerWork in this function would
     * block indefinitely!
     */
    semGive(uploadSem);
#endif
    rt_UploadServerWork(S);
    
    UploadLogInfoTerm();

    connected       = FALSE;
    commInitialized = FALSE;

    ExtCloseConnection(extUD);
} /* end DisconnectFromHost */


/* Function: ProcessConnectMsg =================================================
 * Abstract:
 *  Process the EXT_CONNECT message and send response to host.
 */
PRIVATE boolean_T ProcessConnectMsg(SimStruct *S)
{
    int_T                   nSet;
    MsgHeader               msgHdr;
    int_T                   tmpBufSize;
    uint32_T                *tmpBuf = NULL;
    boolean_T               error   = EXT_NO_ERROR;
    
    const DataTypeTransInfo *dtInfo    = ssGetModelMappingInfo(S);
    uint_T                  *dtSizes   = dtGetDataTypeSizes(dtInfo);
    int_T                   nDataTypes = dtGetNumDataTypes(dtInfo);

    assert(connected);
    assert(!comminitialized);

    /*
     * Send the 1st of two EXT_CONNECT_RESPONSE messages to the host. 
     * The message consists purely of the msgHeader.  In this special
     * case the msgSize actually contains the number of bits per byte
     * (not always 8 - see TI compiler for C30 and C40).
     */
    msgHdr.type = (uint32_T)EXT_CONNECT_RESPONSE;
    msgHdr.size = (uint32_T)8; /* 8 bits per byte */

    error = ExtSetHostMsg(extUD,sizeof(msgHdr),(char_T *)&msgHdr,&nSet);
    if (error || (nSet != sizeof(msgHdr))) {
        fprintf(stderr,
            "ExtSetHostMsg() failed for 1st EXT_CONNECT_RESPONSE.\n");
        goto EXIT_POINT;
    }

    /* Send 2nd EXT_CONNECT_RESPONSE message containing the following 
     * fields:
     *
     * CS1 - checksum 1 (uint32_T)
     * CS2 - checksum 2 (uint32_T)
     * CS3 - checksum 3 (uint32_T)
     * CS4 - checksum 4 (uint32_T)
     * 
     * targetStatus  - the status of the target (uint32_T)
     *
     * nDataTypes    - # of data types        (uint32_T)
     * dataTypeSizes - 1 per nDataTypes       (uint32_T[])
     */

    {
        int nMsgEls    = 4 +                        /* checkSums       */
                         1 +                        /* targetStatus    */
                         1 +                        /* nDataTypes      */
                         dtGetNumDataTypes(dtInfo); /* data type sizes */

        tmpBufSize = nMsgEls * sizeof(uint32_T);
        tmpBuf     = (uint32_T *)malloc(tmpBufSize);
        if (tmpBuf == NULL) {
            error = EXT_ERROR; goto EXIT_POINT;
        }
    }
    
    /* Send message header. */
    msgHdr.type = EXT_CONNECT_RESPONSE;
    msgHdr.size = tmpBufSize;

    error = ExtSetHostMsg(extUD,sizeof(msgHdr),(char_T *)&msgHdr,&nSet);
    if (error || (nSet != sizeof(msgHdr))) {
        fprintf(stderr,
            "ExtSetHostMsg() failed for 2nd EXT_CONNECT_RESPONSE.\n");
        goto EXIT_POINT;
    }
   
    /* Checksums, target status & SL_DOUBLESize. */
    tmpBuf[0] = ssGetChecksum0(S);
    tmpBuf[1] = ssGetChecksum1(S);
    tmpBuf[2] = ssGetChecksum2(S);
    tmpBuf[3] = ssGetChecksum3(S);

    tmpBuf[4] = (uint32_T)modelStatus;

    /* nDataTypes and dataTypeSizes */
    tmpBuf[5] = (uint32_T)nDataTypes;
    (void)memcpy(&tmpBuf[6], dtSizes, sizeof(uint32_T)*nDataTypes);


    /* Send the message. */
    error = ExtSetHostMsg(extUD,tmpBufSize,(char_T *)tmpBuf,&nSet);
    if (error || (nSet != tmpBufSize)) {
        fprintf(stderr,
            "ExtSetHostMsg() failed.\n");
        goto EXIT_POINT;
    }

    commInitialized = TRUE;

EXIT_POINT:
    free(tmpBuf);
    return(error);
} /* end ProcessConnectMsg */


/* Function: SendMsgHdrToHost ==================================================
 * Abstract:
 *  Send a message header to the host.
 */
PRIVATE boolean_T SendMsgHdrToHost(
    const ExtModeAction action,
    const int           size)  /* # of bytes to follow msg header */
{
    int_T     nSet;
    MsgHeader msgHdr;
    boolean_T error = EXT_NO_ERROR;

    msgHdr.type = (uint32_T)action;
    msgHdr.size = size;

    error = ExtSetHostMsg(extUD,sizeof(msgHdr),(char_T *)&msgHdr,&nSet);
    if (error || (nSet != sizeof(msgHdr))) {
        error = EXT_ERROR;
        fprintf(stderr,"ExtSetHostMsg() failed.\n");
        goto EXIT_POINT;
    }

EXIT_POINT:
    return(error);
} /* end SendMsgHdrToHost */


/* Function: SendMsgDataToHost =================================================
 * Abstract:
 *  Send message data to host. You are responsible for sending a header
 *  prior to sending the header.
 */
PRIVATE boolean_T SendMsgDataToHost(const char *data, const int size)
{
    int_T     nSet;
    boolean_T error = EXT_NO_ERROR;

    error = ExtSetHostMsg(extUD,size,data,&nSet);
    if (error || (nSet != size)) {
        error = EXT_ERROR;
        fprintf(stderr,"ExtSetHostMsg() failed.\n");
        goto EXIT_POINT;
    }

EXIT_POINT:
    return(error);
} /* end SendMsgDataToHost */


/* Function: SendMsgToHost =====================================================
 * Abstract:
 *  Send a message to the host on the message socket.  Messages can be of
 *  two forms:
 *      o message header only
 *          the type is used as a flag to notify Simulink of an event
 *          that has taken place on the target (event == action == type)
 *      o msg header, followed by data
 */
PRIVATE boolean_T SendMsgToHost(
    const ExtModeAction action,
    const int           size,  /* # of bytes to follow msg header */
    const char          *data)
{
    boolean_T error = EXT_NO_ERROR;

    error = SendMsgHdrToHost(action,size);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

    if (data != NULL) {
        error = SendMsgDataToHost(data, size);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
    } else {
        assert(size == 0);
    }

EXIT_POINT:
    return(error);
} /* end SendMsgToHost */


/* Function: ProcessSetParamMsg ================================================
 * Receive and process the EXT_SETPARAM message.
 */
PRIVATE boolean_T ProcessSetParamMsg(SimStruct *S, const int msgSize)
{
    const char  *msg;
    boolean_T   error = EXT_NO_ERROR;

    /*
     * Receive message and set parameters.
     */
    msg = GetMsg(msgSize);
    if (msg == NULL) {
        error = EXT_ERROR; goto EXIT_POINT;
    }
    SetParam(S, msg);

    /*
     * Send response to host. 
     */
    error = SendMsgToHost(EXT_SETPARAM_RESPONSE, 0, NULL);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

EXIT_POINT:
    return(error);
} /* end ProcessSetParamMsg */


/* Function: ProcessGetParamsMsg ===============================================
 *  Respond to the hosts request for the parameters by gathering up all the
 *  params and sending them to the host.
 */
PRIVATE boolean_T ProcessGetParamsMsg(SimStruct *S)
{
    int_T                         i;
    int_T                         nBytesTotal;
    boolean_T                     error    = EXT_NO_ERROR;
    const DataTypeTransInfo       *dtInfo  = ssGetModelMappingInfo(S);
    const DataTypeTransitionTable *dtTable = dtGetParamDataTypeTrans(dtInfo);

    if (dtTable != NULL) {
        /*
         * We've got some params in the model.  Send their values to the
         * host.
         */
        int_T        nTrans   = dtGetNumTransitions(dtTable);
        const uint_T *dtSizes = dtGetDataTypeSizes(dtInfo);

 #ifdef VERBOSE
        printf("\nUploading initial parameters....\n");
 #endif

        /*
         * Take pass 1 through the transitions to figure out how many
         * bytes we're going to send.
         */
        nBytesTotal = 0;
        for (i=0; i<nTrans; i++) {
            boolean_T tranIsComplex = dtTransGetComplexFlag(dtTable, i);
            int_T     dt            = dtTransGetDataType(dtTable, i);
            int_T     dtSize        = dtSizes[dt];
            int_T     elSize        = dtSize * (tranIsComplex ? 2 : 1);
            int_T     nEls          = dtTransNEls(dtTable, i);
            int_T     nBytes        = elSize * nEls;

            nBytesTotal += nBytes;
        }

        /*
         * Send the message header.
         */
        error = SendMsgHdrToHost(EXT_GETPARAMS_RESPONSE,nBytesTotal);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        /*
         * Take pass 2 through the transitions and send the parameters.
         */
        for (i=0; i<nTrans; i++) {
            char_T    *tranAddress  = dtTransGetAddress(dtTable, i);
            boolean_T tranIsComplex = dtTransGetComplexFlag(dtTable, i);
            int_T     dt            = dtTransGetDataType(dtTable, i);
            int_T     dtSize        = dtSizes[dt];
            int_T     elSize        = dtSize * (tranIsComplex ? 2 : 1);
            int_T     nEls          = dtTransNEls(dtTable, i);
            int_T     nBytes        = elSize * nEls;

            error = SendMsgDataToHost(tranAddress, nBytes);
            if (error != EXT_NO_ERROR) goto EXIT_POINT;
        }
    } else {
        /*
         * We've got no params in the model.
         */
        error = SendMsgHdrToHost(EXT_GETPARAMS_RESPONSE,0);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
    }

EXIT_POINT:
    return(error);
} /* end ProcessGetParamsMsg */


/*********************
 * Visible Functions *
 *********************/


/* Function: ExtParseArgsAndInitUD =============================================
 * Abstract:
 *  Pass remaining arguments (main program should have NULL'ed out any args
 *  that it processed) to external mode.
 *  
 *  The actual, transport-specific parsing routine (implemented in
 *  ext_svr_transport.c) MUST NULL out all entries of argv that it processes.
 *  The main program depends on this in order to determine if any unhandled
 *  command line options were specified (i.e., if the main program detects
 *  any non-null fields after the parse, it throws an error).
 *
 *  Returns an error string on failure, NULL on success.
 *
 * NOTES:
 *  The external mode UserData is created here so that the specified command-
 *  line options can be stored.
 */
PUBLIC const char_T *ExtParseArgsAndInitUD(const int_T  argc,
                                           const char_T *argv[])
{
    const char_T *error = NULL;
    
    /*
     * Create the user data.
     */
    extUD = ExtUserDataCreate();
    if (extUD == NULL) {
        error = "Could not create external mode user data.  Out of memory.\n";
        goto EXIT_POINT;
    }

    /*
     * Parse the transport-specific args.
     */
    error = ExtProcessArgs(extUD,argc,argv);
    if (error != NULL) goto EXIT_POINT;
        
EXIT_POINT:
    if (error != NULL) {
        ExtUserDataDestroy(extUD);
        extUD = NULL;
    }
    return(error);
} /* end ExtParseArgsAndInitUD */


/* Function: ExtWaitForStartMsg ================================================
 * Abstract:
 *  Return true if waiting for host to tell us when to start.
 */
PUBLIC boolean_T ExtWaitForStartMsg(void)
{
    return(ExtWaitForStartMsgFromHost(extUD));
} /* end ExtWaitForStartMsg */


/* Function: rt_ExtModeInit ====================================================
 * Abstract:
 *  Called once at program startup to do any initialization related to external
 *  mode. 
 */
PUBLIC boolean_T rt_ExtModeInit(void)
{
    boolean_T error = EXT_NO_ERROR;

    error = ExtInit(extUD);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

    UploadLogInfoReset();

    /* For internal Mathworks testing only */
#ifdef TMW_GRT_TESTING
    (void)system("echo grt testing marker: prog started > batmarker");
#endif

EXIT_POINT:
    return(error);
} /* end rt_ExtModeInit */


/* Function: grt_Sleep =========================================================
 * Abstract:
 *  Called by grt_main to "pause".  It attempts to do this in a way that does
 *  not hog the processor.  GRT only.
 */
#ifndef VXWORKS
PUBLIC void grt_Sleep(
    long sec,  /* number of seconds to wait       */
    long usec) /* number of micro seconds to wait */
{
    ExtGRTSleep(extUD,sec,usec);
} /* end grt_Sleep */
#endif


/* Function: rt_MsgServerWork ==================================================
 * Abstract:
 *  If not connected, establish communication of the message line and the
 *  data upload line.  If connected, send/receive messages and parameters
 *  on the message line.
 */
PUBLIC boolean_T rt_MsgServerWork(SimStruct *S)
{
    MsgHeader  msgHdr;
    boolean_T  hdrAvail;
    boolean_T  error             = EXT_NO_ERROR;
    boolean_T  disconnectOnError = FALSE;
    
    /*
     * If not connected, attempt to make connection to host.
     */
    if (!connected) {
        error = ExtOpenConnection(extUD,&connected);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
    }

    /*
     * If ExtOpenConnection is not blocking and there are no pending
     * requests to open a connection, we'll still be unconnected.
     */
    if (!connected) goto EXIT_POINT; /* nothing do do */
    
    /*
     * Process messages.
     */

    /* Wait for a message. */
    error = GetMsgHdr(&msgHdr, &hdrAvail);
    if (error != EXT_NO_ERROR) {
        printf("\nError occured getting message header.\n");
        disconnectOnError = TRUE;
        goto EXIT_POINT;
    }
    
    if (!hdrAvail) goto EXIT_POINT; /* nothing to do */

    /*
     * This is the first message.  Should contain the string:
     * 'ext-mode'.  Its contents are not important to us.
     * It is used as a flag to start the handshaking process.
     */
    if (!commInitialized) {
        msgHdr.type = EXT_CONNECT;
    }

    /* 
     * At this point we know that we have a message: process it.
     */
    switch(msgHdr.type) {

    case EXT_GET_TIME:
    {
        time_T t = ssGetT(S);

        /* Skip verbosity print out - we get too many of these */
        /*PRINT_VERBOSE(("got EXT_GET_TIME message.\n"));*/

        error = SendMsgToHost(
            EXT_GET_TIME_RESPONSE,sizeof(time_T),(char_T *)&t);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
        break;
    }

    case EXT_ARM_TRIGGER:
    {
        PRINT_VERBOSE(("got EXT_ARM_TRIGGER message.\n"));
        UploadArmTrigger();
        break;
    }

    case EXT_SELECT_SIGNALS:
    {
        const char *msg;

        PRINT_VERBOSE(("got EXT_SELECT_SIGNALS message.\n"));

        msg = GetMsg(msgHdr.size);
        if (msg == NULL) {
            error = EXT_ERROR;
            goto EXIT_POINT;
        }

        error = UploadLogInfoInit(S, msg);
        if (error != NO_ERR) {
            printf(
                "\nError in UploadLogInfoInit(). Most likely a memory\n"
                "allocation error or an attempt to re-initialize the\n"
                "signal selection during the data logging process\n"
                "(i.e., multiple EXT_SELECT_SIGNAL messages were received\n"
                "before the logging session terminated or an\n"
                "EXT_CANCEL_LOGGING message was received)");

            goto EXIT_POINT;
        }
        break;
    }

    case EXT_SELECT_TRIGGER: 
    {
        const char *msg;

        PRINT_VERBOSE(("got EXT_SELECT_TRIGGER message.\n"));

        msg = GetMsg(msgHdr.size);
        if (msg == NULL) {
            error = EXT_ERROR;
            goto EXIT_POINT;
        }

        error = UploadInitTrigger(S, msg);
        if (error != EXT_NO_ERROR) {
            printf("\nError in UploadInitTrigger\n");
            goto EXIT_POINT;
        }
        break;
    }

    case EXT_CONNECT:
    {
        PRINT_VERBOSE(("got EXT_CONNECT message.\n"));
        error = ProcessConnectMsg(S);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
        break;
    }

    case EXT_SETPARAM:
    {
        PRINT_VERBOSE(("got EXT_SETPARAM message.\n"));
        error = ProcessSetParamMsg(S, msgHdr.size);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
        break;
    }

    case EXT_GETPARAMS:
    {
        PRINT_VERBOSE(("got EXT_GETPARAMS message.\n"));
        error = ProcessGetParamsMsg(S);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
        break;
    }

    case EXT_DISCONNECT_REQUEST:
    {
        PRINT_VERBOSE(("got EXT_DISCONNECT_REQUEST message.\n"));
        
        /*
         * Note that from the target's point of view this is
         * more a "notify" than a "request".  The host needs to
         * have this acknowledged before it can begin closing
         * the connection.
         */
        error = SendMsgToHost(EXT_DISCONNECT_REQUEST_RESPONSE, 0, NULL);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        DisconnectFromHost(S);

        break;
    }

    case EXT_MODEL_START:
        PRINT_VERBOSE(("got EXT_MODEL_START message.\n"));
#ifdef VXWORKS
        {
            extern SEM_ID startStopSem;
            semGive(startStopSem);
        }
#endif
        startModel = TRUE;
        error = SendMsgToHost(EXT_MODEL_START_RESPONSE, 0, NULL);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        break;

    case EXT_MODEL_STOP:
        PRINT_VERBOSE(("got EXT_MODEL_STOP message.\n"));
        ssSetStopRequested(S, TRUE);
        break;

    case EXT_MODEL_PAUSE:
        PRINT_VERBOSE(("got EXT_MODEL_PAUSE message.\n"));
        modelStatus = TARGET_STATUS_PAUSED;
        startModel  = FALSE;

        error = SendMsgToHost(EXT_MODEL_PAUSE_RESPONSE, 0, NULL);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        break;

    case EXT_MODEL_STEP:
        PRINT_VERBOSE(("got EXT_MODEL_STEP message.\n"));
        if ((modelStatus == TARGET_STATUS_PAUSED) && !startModel) {
            startModel = TRUE;
        }
        
        error = SendMsgToHost(EXT_MODEL_STEP_RESPONSE, 0, NULL);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        break;

    case EXT_MODEL_CONTINUE:
        PRINT_VERBOSE(("got EXT_MODEL_CONTINUE message.\n"));
        if (modelStatus == TARGET_STATUS_PAUSED) {
            modelStatus = TARGET_STATUS_RUNNING;
            startModel  = FALSE;
        }
        
        error = SendMsgToHost(EXT_MODEL_CONTINUE_RESPONSE, 0, NULL);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        break;

    case EXT_CANCEL_LOGGING:
        PRINT_VERBOSE(("got EXT_CANCEL_LOGGING message.\n"));
        UploadCancelLogging();
        break;

    default:
        fprintf(stderr,"received invalid message.\n");
        break;
    } /* end switch */

EXIT_POINT:
    if (error != EXT_NO_ERROR) {
        if (disconnectOnError) {
            fprintf(stderr,
                "Error occured in rt_MsgServerWork.\n"
                "Disconnecting from host!\n");
            DisconnectFromHost(S);
            
            /*
             * Patch by Gopal Santhanam 5/25/2002 (for VXWORKS)
             * If there there was a problem and we have already disconnected
             * from the host, there is no point in returning that error
             * back to rt_MsgServer.  That would cause the task servicing
             * external messages to quit.  Once disconnected, we could
             * just as easily resume by waiting for a new connection.
             */
#ifdef VXWORKS
            error = EXT_NO_ERROR;
#endif            
        }
    }

    return(error);
} /* end rt_MsgServerWork */


/* Function: rt_MsgServer ======================================================
 * Abstract:
 *  Call rt_MsgServerWork forever.   Used only for RTOS (e.g., Tornado/VxWorks
 *  when running as a low priority task.
 */
#ifdef VXWORKS
PUBLIC boolean_T rt_MsgServer(SimStruct *S)
{
    for(;;) {
        if (rt_MsgServerWork(S) == EXT_ERROR) return(EXT_ERROR);
    }
}
#endif


/* Function: rt_UploadServerWork ===============================================
 * Abstract:
 *  Upload model signals to host.
 */
PUBLIC boolean_T rt_UploadServerWork(SimStruct *S)
{
    int_T         i;
    ExtBufMemList upList;
    boolean_T     error = EXT_NO_ERROR;

    UNUSED_PARAM(S);
    
#ifdef VXWORKS
    /*
     * Don't spin the CPU unless we've got data to upload.
     * The upload.c/UploadBufAddTimePoint function gives the sem
     * each time that data is added.
     */
    semTake(uploadSem, WAIT_FOREVER);
#endif

    if (!connected) goto EXIT_POINT;
    
    UploadBufGetData(&upList);
    while(upList.nActiveBufs > 0) {
        for (i=0; i<upList.nActiveBufs; i++) {
            int_T        nSet;
            const BufMem *bufMem = &upList.bufs[i];

            error = ExtSetHostData(
                extUD,bufMem->nBytes1,bufMem->section1,&nSet);
            if (error || (nSet != bufMem->nBytes1)) {
                fprintf(stderr,"ExtSetHostData() failed on data upload.\n");
                goto EXIT_POINT;
            }

            if (bufMem->section2 != NULL) {
                /* circular buffer was wrapped - send 2nd piece */
                error = ExtSetHostData(
                    extUD,bufMem->nBytes2,bufMem->section2,&nSet);
                if (error || (nSet != bufMem->nBytes2)) {
                    fprintf(stderr,"ExtSetHostData() failed on data upload.\n");
                    goto EXIT_POINT;
                }
            }

            /* comfirm that the data was sent */
            UploadBufDataSent(upList.tids[i]);
        }
        UploadBufGetData(&upList);
    }

EXIT_POINT:
    return(error);
} /* end rt_UploadServerWork */


/* Function: rt_UploadServer ===================================================
 * Abstract:
 *  Call rt_UploadServerWork forever.   Used only for RTOS (e.g.,
 *  Tornado/VxWorks when running as a low priority task.
 */
#ifdef VXWORKS
PUBLIC boolean_T rt_UploadServer(SimStruct *S)
{
    for(;;) {
        if (rt_UploadServerWork(S) == EXT_ERROR) return(EXT_ERROR);
    }
} /* end rt_UploadServer */
#endif


/* Function: rt_ExtModeShutdown ================================================
 * Abstract:
 *  Called when target program terminates to enable cleanup of external 
 *  mode.
 */
PUBLIC boolean_T rt_ExtModeShutdown(SimStruct *S)
{
    boolean_T error = EXT_NO_ERROR;

    /*
     * Make sure buffers are flushed so that the final points get to
     * host (this is important for the case of the target reaching tfinal
     * while data is uploading is in progress).
     */
    UploadPrepareForFinalFlush();
    rt_UploadServerWork(S);
    
    UploadLogInfoTerm();
    if (msgBuf != NULL) free(msgBuf);
    
    if (connected) {
        error = SendMsgToHost(EXT_MODEL_SHUTDOWN, 0, NULL);
        if (error != EXT_NO_ERROR) {
            fprintf(stderr,
                "\nError sending 'EXT_MODEL_SHUTDOWN' message to host.\n");
        }
        connected = FALSE;
        commInitialized = FALSE;
        modelStatus = TARGET_STATUS_WAITING_TO_START;        
    }

    ExtShutDown(extUD);
    ExtUserDataDestroy(extUD);

    /* For internal Mathworks testing only */
#ifdef TMW_GRT_TESTING
# ifdef WIN32
    (void)system("del /f batmarker");
# else
    (void)system("rm -f batmarker");
# endif
#endif
        
    return(error);
} /* end rt_ExtModeShutdown */


/* [EOF] ext_svr.c */
