
#ifndef SAFEBUFFER_H
#define SAFEBUFFER_H

#include <stdio.h>
#include <iostream>
#include <queue>
#include <yarp/os/all.h>
#include <functional>


template <class T>
class SafeBuffer
{
protected:
    yarp::os::Mutex bufMutex;//mutex to lock the array, for avoiding race condition
    yarp::os::Semaphore semArray;
    std::queue<T> array;
    bool interrupted;

public:
    SafeBuffer() : interrupted(false), semArray(0) {}

    virtual bool read(T& data) {

        //wait for new data
        /*
         * Decrement the counter, even if we must wait to do that.  If the counter
         * would decrement below zero, the calling thread must stop and
         * wait for another thread to call Semaphore::post on this semaphore.
         */
        semArray.wait();
        //yDebug()<<"read() : unwait";

        if(interrupted) {
            return false;
        }

        // read
        bufMutex.lock();
        data=array.front();
        array.pop();
        bufMutex.unlock();
        return true;
    }

    virtual bool write(T& data) {
        //write
        bufMutex.lock();
        array.push(data);
        //yDebug()<<"write() : signal";
        bufMutex.unlock();
        //signal that a new data is ready
        /*
         * Increment the counter.  If another thread is waiting to decrement the
         * counter, it is woken up.
         */
        semArray.post();
        return true;
    }

    void interrupt() {
        bufMutex.lock();
        interrupted = true;
        bufMutex.unlock();
        semArray.post();
    }

};

#endif // VGSLAMBUFFER_H
