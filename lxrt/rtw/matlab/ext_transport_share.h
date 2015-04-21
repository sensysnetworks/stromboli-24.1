/* File: ext_custom_share.h
 * Absract:
 *	External mode shared data structures used by the external communication,
 *      mex link, and the generated code.  This file is for definitions 
 *      related to custom external mode implementations (e.g., sockets).
 *      See ext_share.h for definitions common to all implementations
 *      of external mode (not that ext_share.h should not be modified).
 *     
 *      MODIFY THIS FILE OF CUSTOM EXTERNAL MODE IMPLEMENTATIONS.
 *
 * Copyright 1994-2001 The MathWorks, Inc.
 * $Revision: 1.1.1.1 $
 *
 *
 * 2002 - Fix by Gopal Santhanam <gopal@nerur.com>
 */

#ifndef __EXTCUSTSHARE__
#define __EXTCUSTSHARE__


#ifdef WIN32
  /* WINDOWS */
# define close closesocket
# define SOCK_ERR SOCKET_ERROR
#else
  /* UNIX, VXWORKS */
# define INVALID_SOCKET -1
# define SOCK_ERR (-1)

  typedef int SOCKET;
#endif

#define SERVER_PORT_NUM  (17725)   /* sqrt(pi)*10000 */


/***************** PRIVATE FUNCTIONS ******************************************
 *                                                                            *
 *  THE FOLLOWING FUNCTIONS ARE SPECIFIC TO THE TCPIP EXAMPLE OF HOST-        *
 *  TARGET COMMUNICATION.  THEY ARE IDENTICAL FOR THE TARGET AND THE HOST.    *
 *  AS SUCH, THE ARE DEFINED IN THIS COMMON FILE.  SEE                        *
 *      <MATLABROOT>\RTW\EXT_MODE\EXT_TRANSPORT.C AND                         *
 *      <MATLABROOT>\RTW\C\SRC\EXT_SVR_TRANSPORT.C                            *
 *                                                                            *
 *  FOR MORE INFO.                                                            *
 *                                                                            *
 ******************************************************************************/


/* Function: SocketDataPending =================================================
 * Abstract:
 *  Returns true, via the 'pending' arg, if data is pending on the msg line.
 *  Returns false otherwise.  If the timeout is 0, do simple polling (i.e.,
 *  return immediately).  Otherwise, wait the specified amount of time.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure (reaching
 *  a nonzero timeout is considered a failure).
 */
PRIVATE boolean_T SocketDataPending(
    const SOCKET sock,
    boolean_T    *outPending,
    long int     timeOutSecs,
    long int     timeOutUSecs)
{
    fd_set          ReadFds;
    int             pending;
    struct timeval  tval;
    boolean_T       error          = EXT_NO_ERROR;
    const int       timeOutOccured = 0;
    const boolean_T useTimeOut     = (boolean_T)((timeOutSecs != 0) || 
                                        (timeOutUSecs != 0));
    
    FD_ZERO(&ReadFds);
    FD_SET(sock, &ReadFds);

    tval.tv_sec  = timeOutSecs;
    tval.tv_usec = timeOutUSecs;

    pending = select(sock + 1, &ReadFds, NULL, NULL, &tval);
    if ((pending == SOCK_ERR) || (useTimeOut && (pending == timeOutOccured))) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }

EXIT_POINT:
    *outPending = ((boolean_T)(pending == 1));
    return(error);    
} /* end SocketDataPending */ 


/* Function: SocketDataGet =====================================================
 * Abstract:
 *  Attempts to gets the specified number of bytes from the specified socket. 
 *  The number of bytes read is returned via the 'nBytesGot' parameter.
 *  EXT_NO_ERROR is returned on success, EXT_ERROR is returned on failure.
 *
 * NOTES:
 *  o it is not an error for 'nBytesGot' to be returned as 0
 *  o this function blocks if no data is available
 */
PRIVATE boolean_T SocketDataGet(
    const SOCKET sock,
    const int_T  nBytesToGet,
    int_T        *nBytesGot, /* out */
    char_T       *dst)       /* out */
{
    int_T     nRead;
    boolean_T error = EXT_NO_ERROR;
   
    /* 
     * Patch by Gopal Santhan (5/25/2002)
     * Make sure the socket didn't simply get disconnected!
     * Added a check to SocketDataPending and the second
     * clause on the "if (nRead ...)" line.
     */
    boolean_T pending;
    error = SocketDataPending(sock, &pending, 0, 0);
    if (error) goto EXIT_POINT;
    
    nRead = recv(sock, dst, nBytesToGet, 0);
    if (nRead == SOCK_ERR || (nRead == 0 && nBytesToGet != 0 && pending)) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }

EXIT_POINT:
    if (error) {
        nRead = 0;
    }
    *nBytesGot = nRead;
    return(error);
} /* end SocketDataGet */ 


/* Function: SocketDataSet =====================================================
 * Abstract:
 *  Sets (sends) the specified number of bytes on the specified socket.  As long
 *  as an error does not occur, this function is guaranteed to set the requested
 *  number of bytes.  The number of bytes set is returned via the 'nBytesSet'
 *  parameter.  EXT_NO_ERROR is returned on success, EXT_ERROR is returned on
 *  failure.
 *
 * NOTES:
 *  o this function blocks if tcpip's send buffer doesn't have room for all
 *    of the data to be sent
 */
PRIVATE boolean_T SocketDataSet(
    const SOCKET sock,
    const int_T  nBytesToSet,
    const char_T *src,
    int_T        *nBytesSet) /* out */
{
    int_T     nSent;    
    boolean_T error = EXT_NO_ERROR;
    
#ifndef VXWORKS
    nSent = send(sock, src, nBytesToSet, 0);
#else
    /*
     * VXWORKS send prototype does not have src as const.  This suppresses
     * the compiler warning.
     */
    nSent = send(sock, (char_T *)src, nBytesToSet, 0);
#endif
    if (nSent == SOCK_ERR) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }

EXIT_POINT:
    *nBytesSet = nSent;
    return(error);
} /* end SocketDataSet */


#endif /* __EXTCUSTSHARE__ */
