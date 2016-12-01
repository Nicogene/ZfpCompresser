#ifndef ZFPCOMPRESSERMODULE_H
#define ZFPCOMPRESSERMODULE_H

#include "yarp/os/all.h"
#include "yarp/sig/all.h"
#include "threadCompresser.h"
#include "safebuffer.h"


class ZFPCompresserModule : public yarp::os::RFModule
{
    int numIter;
    bool interrupted;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelFloat> > portIn, portOut;
    ThreadCompresser* thread;
    SafeBuffer<yarp::sig::ImageOf<yarp::sig::PixelFloat>> buffer;


public:
    ZFPCompresserModule(int _numIter);
    ~ZFPCompresserModule();
    bool configure(yarp::os::ResourceFinder &rf);
    bool updateModule();
    double getPeriod();
    bool interruptModule();
    bool close();
};

#endif
