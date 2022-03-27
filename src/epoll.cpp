#include "epoll.hpp"

#ifdef __APPLE__

#else

#include <cassert>


#define EPOLL_SIZE 1024

using namespace AsyncRuntime;

enum { R = 1, W = 2 };


Epoll::Epoll() : events(EPOLL_SIZE, {0}), events_count(0)
{
    int size = (int)events.size();
    fd = epoll_create(size);
    assert(fd >= 0);
}


Epoll::~Epoll()
{
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}


int Epoll::Wait(int timeout)
{
    if (events.empty())
        return 0;

    int _count = epoll_wait(fd, &events[0], (int)events.size(), timeout);
    if (_count <= 0)
        return 0;
    int i = 0;
    while (i < _count) {
        struct epoll_event *ev = &events[i];
        FD *_fd = (FD*)ev->data.ptr;
        if (_fd->on_read) {
            if (ev->events & EPOLLIN)
                _fd->on_read(_fd);
        }
        if (_fd->on_write) {
            if (ev->events & EPOLLOUT || ev->events & EPOLLERR ||
                ev->events & EPOLLHUP) {
                _fd->on_write(_fd);
            }
        }
        i++;
    }
    return _count;
}


int Epoll::Add(FD *fd_, int mask)
{
    if ((events_count + 1) > events.size()) {
        events.push_back({0});
    }
    epoll_event ev{0};
    ev.events = 0;
    fd_->mask = mask;
    if (fd_->mask & R)
        ev.events |= EPOLLIN;
    if (fd_->mask & W)
        ev.events |= EPOLLOUT;
    ev.data.ptr = fd_;
    int rc = epoll_ctl(fd, EPOLL_CTL_ADD, fd_->fd, &ev);
    if (rc == -1)
        return -1;
    events_count++;
    return 0;
}


int Epoll::Modify(FD *fd_, int mask)
{
    epoll_event ev{0};
    ev.events = 0;
    if (mask & R)
        ev.events |= EPOLLIN;
    if (mask & W)
        ev.events |= EPOLLOUT;
    ev.data.ptr = fd_;
    int rc = epoll_ctl(fd, EPOLL_CTL_MOD, fd_->fd, &ev);
    if (rc == -1)
        return -1;
    fd_->mask = mask;
    return 0;
}


int Epoll::Read(FD *fd_, FDCallback on_read, void *arg, int enable)
{
    int mask = fd_->mask;
    if (enable)
        mask |= R;
    else
        mask &= ~R;
    fd_->on_read = on_read;
    fd_->on_read_arg = arg;
    if (mask == fd_->mask)
        return 0;
    return Modify(fd_, mask);
}


int Epoll::Write(FD *fd_, FDCallback on_write, void *arg, int enable)
{
    int mask = fd_->mask;
    if (enable)
        mask |= W;
    else
        mask &= ~W;
    fd_->on_write = on_write;
    fd_->on_write_arg = arg;
    if (mask == fd_->mask)
        return 0;
    return Modify(fd_, mask);
}


int Epoll::Delete(FD *fd_)
{
    epoll_event ev{0};
    ev.events = 0;
    if (fd_->mask & R)
        ev.events |= EPOLLIN;
    if (fd_->mask & W)
        ev.events |= EPOLLOUT;
    ev.data.ptr = fd_;
    fd_->mask = 0;
    fd_->on_write = nullptr;
    fd_->on_write_arg = nullptr;
    fd_->on_read = nullptr;
    fd_->on_read_arg = nullptr;
    events_count--;
    assert(events_count >= 0);
    return epoll_ctl(fd, EPOLL_CTL_DEL, fd_->fd, &ev);
}

#endif