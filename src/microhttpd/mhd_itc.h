/*
  This file is part of libmicrohttpd
  Copyright (C) 2016 Karlson2k (Evgeny Grin)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

/**
 * @file microhttpd/mhd_itc.h
 * @brief  Header for platform-independent inter-thread communication
 * @author Karlson2k (Evgeny Grin)
 * @author Christian Grothoff
 *
 * Provides basic abstraction for inter-thread communication.
 * Any functions can be implemented as macro on some platforms
 * unless explicitly marked otherwise.
 * Any function argument can be skipped in macro, so avoid
 * variable modification in function parameters.
 */
#ifndef MHD_ITC_H
#define MHD_ITC_H 1
#include "mhd_itc_types.h"

#include <fcntl.h>

#if defined(_MHD_ITC_EVENTFD)

/* **************** Optimized GNU/Linux ITC implementation by eventfd ********** */
#include <sys/eventfd.h>
#include <stdint.h>      /* for uint64_t */
#ifdef HAVE_UNISTD_H
#include <unistd.h>      /* for read(), write(), errno */
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h> /* for strerror() */
#endif


/**
 * Initialise ITC by generating eventFD
 * @param itc the itc to initialise
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_init_(itc) (-1 != ((itc) = eventfd (0, EFD_CLOEXEC | EFD_NONBLOCK)))

/**
 * Get description string of last errno for itc operations.
 */
#define MHD_itc_last_strerror_() strerror(errno)

/**
 * Internal static const helper for MHD_itc_activate_()
 */
static const uint64_t _MHD_itc_wr_data = 1;

/**
 * Activate signal on @a itc
 * @param itc the itc to use
 * @param str ignored
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_activate_(itc, str) \
  ((write((itc), (const void*)&_MHD_itc_wr_data, 8) > 0) || (EAGAIN == errno))

/**
 * Return read FD of @a itc which can be used for poll(), select() etc.
 * @param itc the itc to get FD
 * @return FD of read side
 */
#define MHD_itc_r_fd_(itc) ((int)(itc))

/**
 * Return write FD of @a itc
 * @param itc the itc to get FD
 * @return FD of write side
 */
#define MHD_itc_w_fd_(itc) ((int)(itc))

/**
 * drain data from real pipe
 */
#define MHD_pipe_drain_(pip) do { \
   uint64_t tmp; \
   read (pip, &tmp, sizeof (tmp)); \
 } while (0)

/**
 * Close any FDs of the pipe (non-W32)
 */
#define MHD_pipe_close_(pip) do { \
    if ( (0 != close (pip)) && \
         (EBADF == errno) )             \
      MHD_PANIC (_("close failed"));    \
  } while (0)

/**
 * Check if we have an uninitialized pipe
 */
#define MHD_INVALID_PIPE_(pip)  (-1 == pip)

/**
 * Setup uninitialized @a pip data structure.
 */
#define MHD_make_invalid_pipe_(pip) do { \
    pip = -1;        \
  } while (0)


/**
 * Change itc FD options to be non-blocking.  As we already did this
 * on eventfd creation, this always succeeds.
 *
 * @param fd the FD to manipulate
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_nonblocking_(pip) (!0)


#elif defined(_MHD_ITC_PIPE)

/* **************** Standard UNIX ITC implementation by pipe ********** */

#ifdef HAVE_UNISTD_H
#include <unistd.h>      /* for read(), write(), errno */
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h> /* for strerror() */
#endif


/**
 * Initialise ITC by generating pipe
 * @param itc the itc to initialise
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_init_(itc) (!pipe((itc).fd))

/**
 * Get description string of last errno for itc operations.
 */
#define MHD_itc_last_strerror_() strerror(errno)

/**
 * Activate signal on @a itc
 * @param itc the itc to use
 * @param str one-symbol string, useful only for strace debug
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_activate_(itc, str) \
  ((write((itc).fd[1], (const void*)(str), 1) > 0) || (EAGAIN == errno))


/**
 * Return read FD of @a itc which can be used for poll(), select() etc.
 * @param itc the itc to get FD
 * @return FD of read side
 */
#define MHD_itc_r_fd_(itc) ((itc).fd[0])

/**
 * Return write FD of @a itc
 * @param itc the itc to get FD
 * @return FD of write side
 */
#define MHD_itc_w_fd_(itc) ((itc).fd[1])

/**
 * drain data from real pipe
 */
#define MHD_pipe_drain_(pip) do { \
   long tmp; \
   while (0 < read((pip).fd[0], (void*)&tmp, sizeof (tmp))) ; \
 } while (0)

/**
 * Close any FDs of the pipe (non-W32)
 */
#define MHD_pipe_close_(pip) do { \
    if ( (0 != close ((pip).fd[0])) && \
         (EBADF == errno) )             \
      MHD_PANIC (_("close failed"));    \
    if ( (0 != close ((pip).fd[1])) && \
         (EBADF == errno) )             \
      MHD_PANIC (_("close failed"));    \
  } while (0)

/**
 * Check if we have an uninitialized pipe
 */
#define MHD_INVALID_PIPE_(pip)  (-1 == (pip).fd[0])

/**
 * Setup uninitialized @a pip data structure.
 */
#define MHD_make_invalid_pipe_(pip) do { \
    (pip).fd[0] = (pip).fd[1] = -1; \
  } while (0)

/**
 * Change itc FD options to be non-blocking.
 *
 * @param fd the FD to manipulate
 * @return non-zero if succeeded, zero otherwise
 */
int
MHD_itc_nonblocking_ (MHD_itc_ itc);


#elif defined(_MHD_ITC_SOCKETPAIR)

/* **************** ITC implementation by socket pair ********** */

#include "mhd_sockets.h"


/**
 * Initialise ITC by generating socketpair
 * @param itc the itc to initialise
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_init_(itc) MHD_socket_pair_((itc).sk)

/**
 * Get description string of last error for itc operations.
 */
#define MHD_itc_last_strerror_() MHD_socket_last_strerr_()

/**
 * Activate signal on @a itc
 * @param itc the itc to use
 * @param str one-symbol string, useful only for strace debug
 * @return non-zero if succeeded, zero otherwise
 */
#define MHD_itc_activate_(itc, str) \
  ((send((itc).sk[1], (const char*)(str), 1, 0) > 0) || \
   (MHD_SCKT_ERR_IS_EAGAIN_(MHD_socket_get_error_())))

/**
 * Return read FD of @a itc which can be used for poll(), select() etc.
 * @param itc the itc to get FD
 * @return FD of read side
 */
#define MHD_itc_r_fd_(itc) ((itc).sk[0])

/**
 * Return write FD of @a itc
 * @param itc the itc to get FD
 * @return FD of write side
 */
#define MHD_itc_w_fd_(itc) ((itc).sk[1])

/**
 * Drain data from emulated pipe
 */
#define MHD_pipe_drain_(pip) do { long tmp; while (0 < recv((pip).sk[0], (void*)&tmp, sizeof (tmp), 0)) ; } while (0)


/**
 * Close emulated pipe FDs
 */
#define MHD_pipe_close_(pip) do { \
   MHD_socket_close_chk_ ((pip).sk[0]); \
   MHD_socket_close_chk_ ((pip).sk[1]); \
} while (0)

/**
 * Check for uninitialized pipe @a pip
 */
#define MHD_INVALID_PIPE_(pip)  (MHD_INVALID_SOCKET == (pip).sk[0])

/**
 * Setup uninitialized @a pip data structure.
 */
#define MHD_make_invalid_pipe_(pip) do { \
    pip.sk[0] = pip.sk[1] = MHD_INVALID_SOCKET; \
  } while (0)


#define MHD_itc_nonblocking_(pip) (MHD_socket_nonblocking_((pip).sk[0]) && MHD_socket_nonblocking_((pip).sk[1]))

#endif /* _MHD_ITC_SOCKETPAIR */

#endif /* MHD_ITC_H */
