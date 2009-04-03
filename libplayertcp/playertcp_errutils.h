#if defined (WIN32)
  #define ErrNo WSAGetLastError()
#else
  #define ErrNo errno
#endif

// Joy of joys, even strerror isn't the same for Windows sockets
#if defined (WIN32)
  const int ERRNO_EAGAIN = WSAEWOULDBLOCK;
  const int ERRNO_EWOULDBLOCK = WSAEWOULDBLOCK;
  #define STRERROR(errMacro,text) {LPVOID errBuffer = NULL; \
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, \
    ErrNo, 0, (LPTSTR) &errBuffer, 0, NULL); \
    errMacro (text, (LPTSTR) errBuffer); \
    LocalFree (errBuffer);}
#else
  #define STRERROR(errMacro,text) errMacro (text, strerror (ErrNo));
  const int ERRNO_EAGAIN = EAGAIN;
  const int ERRNO_EWOULDBLOCK = EWOULDBLOCK;
#endif
