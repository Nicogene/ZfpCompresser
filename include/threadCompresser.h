#ifndef THREADCOMPRESSER_H
#define THREADCOMPRESSER_H
#include "yarp/os/all.h"
#include "yarp/sig/all.h"
#include "safebuffer.h"
#include <zfp.h>

class ThreadCompresser : public yarp::os::RateThread
{
    int numProcessed;
    bool interrupted;
    SafeBuffer<yarp::sig::ImageOf<yarp::sig::PixelFloat>>* buffer;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelFloat>> *portOut;
    zfp_field* field;  /* array meta data */

public:
    ThreadCompresser(int period, SafeBuffer<yarp::sig::ImageOf<yarp::sig::PixelFloat>> &buffer, yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelFloat>> &portOut );
    ~ThreadCompresser();
    int getNumProcessed();
    void run();
    void interrupt();
    void close();

protected:
    int compressAndDecompress(float* array, int nx, int ny, float tolerance);



};

#endif
