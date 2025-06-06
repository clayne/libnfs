include(CheckIncludeFile)
include(CheckIncludeFiles)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("dispatch/dispatch.h" HAVE_DISPATCH_DISPATCH_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("fuse.h" HAVE_FUSE_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("netinet/tcp.h" HAVE_NETINET_TCP_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
# On MacOS 10.8 and earlier, sys/socket.h must be included before net/if.h
if(HAVE_SYS_SOCKET_H)
  check_include_files("sys/socket.h;net/if.h" HAVE_NET_IF_H)
else()
  check_include_file("net/if.h" HAVE_NET_IF_H)
endif()
check_include_file("poll.h" HAVE_POLL_H)
check_include_file("pwd.h" HAVE_PWD_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("stdatomic.h" HAVE_STDATOMIC_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/filio.h" HAVE_SYS_FILIO_H)
check_include_file("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_file("sys/statvfs.h" HAVE_SYS_STATVFS_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/sysmacros.h" HAVE_SYS_SYSMACROS_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/uio.h" HAVE_SYS_UIO_H)
check_include_file("sys/vfs.h" HAVE_SYS_VFS_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("utime.h" HAVE_UTIME_H)
check_include_file("signal.h" HAVE_SIGNAL_H)
check_include_file("sys/utsname.h" HAVE_SYS_UTSNAME_H)

include(CheckStructHasMember)
check_struct_has_member("struct sockaddr" sa_len sys/socket.h HAVE_SOCKADDR_LEN)
check_struct_has_member("struct sockaddr_storage" ss_family sys/socket.h HAVE_SOCKADDR_STORAGE)
check_struct_has_member("struct stat" st_mtim.tv_nsec sys/stat.h HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)

include(CheckCSourceCompiles)
check_c_source_compiles("#include <net/if.h>
                         int main(void)
                         {
                           int i = SO_BINDTODEVICE;
                         }"
                        HAVE_SO_BINDTODEVICE)

check_c_source_compiles("#include <talloc.h>
                         #include <tevent.h>
                         int main(void)
                         {
                           struct tevent_context *ctx = tevent_context_init(NULL);
                           int major = talloc_version_major();
                         }"
                        HAVE_TALLOC_TEVENT)

check_c_source_compiles("#include <time.h>
                         int main(void)
                         {
                           int i = clock_gettime(CLOCK_MONOTONIC_COARSE, NULL);
                         }"
                        HAVE_CLOCK_GETTIME)

check_c_source_compiles("#include <sys/types.h>
                         #define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
                         int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721 && LARGE_OFF_T % 2147483647 == 1) ? 1 : -1];
                         int main()
                         {
                           return 0;
                         }"
                        NO_LFS_REQUIRED)

#
# Find out if gnutls exports the function gnutls_transport_is_ktls_enabled().
# This tells whether the gnutls library has kTLS support.
#
set(CMAKE_REQUIRED_LIBRARIES gnutls)
check_c_source_compiles("#include <gnutls/socket.h>
                         int main()
                         {
                                gnutls_session_t session;
                                gnutls_transport_is_ktls_enabled(session);
                         }"
                        HAVE_GNUTLS_TRANSPORT_IS_KTLS_ENABLED)

#
# Test for pthread availability.
#
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_c_source_compiles("#include <pthread.h>
                         int main()
                         {
                                pthread_t thread;
                                if (pthread_create(&thread, NULL, NULL, NULL) != 0) {
                                        return -1;
                                }
                                return 0;
                         }"
                        HAVE_PTHREAD)

if(NOT NO_LFS_REQUIRED)
  check_c_source_compiles("#include <sys/types.h>
                           #define _FILE_OFFSET_BITS 64
                           #define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
                           int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721 && LARGE_OFF_T % 2147483647 == 1) ? 1 : -1];
                           int main()
                           {
                             return 0;
                           }"
                          _FILE_OFFSET_BITS)

  check_c_source_compiles("#include <sys/types.h>
                           #define _LARGE_FILES 1
                           #define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
                           int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721 && LARGE_OFF_T % 2147483647 == 1) ? 1 : -1];
                           int main()
                           {
                             return 0;
                           }"
                          _LARGE_FILES)
endif()


include(CheckSymbolExists)
check_symbol_exists("makedev" "sys/mkdev.h" MAJOR_IN_MKDEV)
check_symbol_exists("makedev" "sys/sysmacros.h" MAJOR_IN_SYSMACROS)
check_symbol_exists("pthread_threadid_np" "pthread.h" HAVE_PTHREAD_THREADID_NP)

include(CheckCCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCC)
  check_c_compiler_flag(-Wall C_ACCEPTS_WALL)

  if(C_ACCEPTS_WALL)
    add_definitions(-Wall)
  endif()
endif()

configure_file(cmake/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)
