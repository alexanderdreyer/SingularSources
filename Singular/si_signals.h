/*****************************************
*  Computer Algebra System SINGULAR      *
*****************************************/
/*
* ABSTRACT: wrappijng signal-interuptable system calls
* AUTHOR: Alexander Dreyer, The PolyBoRi Team
*/

#include <signal.h>
#include <errno.h>

#ifndef SINGULAR_SI_SIGNALS_H
#define SINGULAR_SI_SIGNALS_H

#define SI_EINTR_SAVE_FUNC(func, decl, args)	\
inline int si_##func decl                \
{                                        \
  int res = -1;	                         \
  do                                     \
  {                                      \
    res = func args;		         \
  } while((res < 0) && (errno == EINTR));\
  return res;                            \
}


SI_EINTR_SAVE_FUNC(select,
		   (int nfds, fd_set *readfds, fd_set *writefds,
		    fd_set *exceptfds, struct timeval *timeout),
		   (nfds,readfds, writefds, exceptfds, timeout)
		   )

SI_EINTR_SAVE_FUNC(pselect,
		   (int nfds, fd_set *readfds, fd_set *writefds,
		    fd_set *exceptfds, const struct timespec *timeout,
		    const sigset_t *sigmask),
		   (nfds, readfds, writefds, exceptfds, timeout,sigmask)
		   )

SI_EINTR_SAVE_FUNC(read,(int fd, void *buf, size_t count),
		   (fd, buf, count))


/// TODO: wrap and replace the following system calls: from man 7 signal
/// read(2), readv(2), write(2), writev(2), and ioctl(2) (slow devices)
/// open(2), if it can block, see fifo(7)
/// wait(2), wait3(2), wait4(2), waitid(2), and waitpid(2).
/// Socket interfaces: accept(2), connect(2), recv(2), recvfrom(2),
/// recvmsg(2), send(2), sendto(2), and sendmsg(2), unless a timeout has
/// been set on the socket 
/// flock(2) and fcntl(2)
/// mq_receive(3),  mq_timedreceive(3),  mq_send(3),  and
///             mq_timedsend(3).
///futex(2
///sem_wait(3) and sem_timedwait(3)
/// 
/// regardless of the use of SA_RESTART: 
///
/// accept(2), recv(2), recvfrom(2), and recvmsg(2), connect(2),
/// send(2), sendto(2), and sendmsg(2), when a timeout is set
/// pause(2), sigsuspend(2), sigtimedwait(2), and sigwaitinfo(2).
/// epoll_wait(2), epoll_pwait(2), poll(2), ppoll(2)
///  msgrcv(2), msgsnd(2), semop(2), and semtimedop(2).
/// clock_nanosleep(2), nanosleep(2), and usleep(3).
/// read(2) from an inotify(7) file descriptor.
/// io_getevents
/// sleep(3)

 // Interruption of System Calls and Library Functions by Stop Signals
 //       On Linux, even in the absence of signal handlers, certain blocking interfaces can fail with the
 //       error  EINTR  after the process is stopped by one of the stop signals and then resumed via SIG‐
 //       CONT.  This behavior is not sanctioned by POSIX.1, and doesn't occur on other systems.

 //           * Socket interfaces, when a timeout  has  been  set  on  the  socket  using  setsockopt(2):
 //             accept(2),  recv(2),  recvfrom(2), and recvmsg(2), if a receive timeout (SO_RCVTIMEO) has
 //             been set; connect(2), send(2), sendto(2), and sendmsg(2), if a send timeout (SO_SNDTIMEO)
 //             has been set.

 //           * epoll_wait(2), epoll_pwait(2).

 //           * semop(2), semtimedop(2).

 //           * sigtimedwait(2), sigwaitinfo(2).

 //           * read(2) from an inotify(7) file descriptor.

 //           * Linux 2.6.21 and earlier: futex(2) FUTEX_WAIT, sem_timedwait(3), sem_wait(3).

 //           * Linux 2.6.8 and earlier: msgrcv(2), msgsnd(2).

 //           * Linux 2.4 and earlier: nanosleep(2).






#undef SI_EINTR_SAVE_FUNC_BODY


#endif /* SINGULAR_SI_SIGNALS_H */
