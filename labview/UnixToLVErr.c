/* 
Got it from a National Instruments support engineer without comment.  Using
this is only meaningful together with NI LabVIEW, so I hope its ok to spread
it together with the LabVIEW/LXRT binding. If unsure, ask NI or recode...
*/

MgErr UnixToLVErr(void)
    {
    switch(errno) {                     
        case 0:             return noErr;
        case ESPIPE:        return fEOF;
        case EINVAL:
        case EBADF:         return mgArgErr;
        case ETXTBSY:       return fIsOpen;
        case ENOENT:        return fNotFound;
#ifdef EAGAIN
        case EAGAIN:    /* SVR4, file is locked */
#endif
#ifdef EDEADLK
        case EDEADLK:   /* deadlock would occur */
#endif
#ifdef ENOLCK
        case ENOLCK:    /* NFS, lock not avail */
#endif
        case EPERM:
        case EACCES:        return fNoPerm;
        case ENOSPC:        return fDiskFull;
        case EEXIST:        return fDupPath;
        case ENFILE:
        case EMFILE:        return fTMFOpen;
        case ENOMEM:        return mFullErr;
        case EIO:           return fIOErr;
        }
    return fIOErr;   /* fIOErr generally signifies some unknown file error */
    }