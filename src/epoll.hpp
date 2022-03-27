#ifndef AR_EPOLL_H
#define AR_EPOLL_H


#include "ar/helper.hpp"

#ifdef __APPLE__

#else
#include <sys/epoll.h>

namespace AsyncRuntime {
    struct FD;
    typedef struct epoll_event EpollEvent;

    typedef void (*FDCallback)(FD *);


    /**
     * @struct FD
     * @brief file descriptor
     */
    struct FD {
        int         fd;
        int         mask;
        FDCallback  on_read;
        void *      on_read_arg;
        FDCallback  on_write;
        void *      on_write_arg;
    };


    /**
     * @class Epoll class
     * @brief
     */
    class Epoll {
    public:
        explicit Epoll();
        ~Epoll();

        Epoll(const Epoll&) = delete;
        Epoll& operator =(const Epoll&) = delete;
        Epoll(Epoll&&) = delete;
        Epoll& operator =(Epoll&&) = delete;


        /**
         * @brief
         * @param timeout_ms
         */
        int Wait(int timeout_ms = -1);


        /**
         * @brief
         * @param fd
         * @param mask
         * @return
         */
        int Add(FD *fd, int mask = EPOLLIN | EPOLLPRI);


        /**
         * @brief
         * @param fd
         * @param mask
         * @return
         */
        int Modify(FD *fd, int mask = 0);


        /**
         * @brief
         * @param fd
         * @param on_read
         * @param arg
         * @param enable
         * @return
         */
        int Read(FD *fd, FDCallback on_read, void *arg, int enable = 1);



        /**
         * @brief
         * @param fd
         * @param on_read
         * @param arg
         * @param enable
         * @return
         */
        int Write(FD *fd, FDCallback on_write, void *arg, int enable = 1);


        /**
         * @brief
         * @param fd
         * @return
         */
        int Delete(FD *fd);

        std::vector<EpollEvent>     events;
    private:
        int                         fd;
        int                         events_count;
    };
}
#endif

#endif //AR_EPOLL_H
