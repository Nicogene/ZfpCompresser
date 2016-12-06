#ifndef ZFPCOMPRESSERMODULE_H
#define ZFPCOMPRESSERMODULE_H

#include "yarp/os/all.h"
#include "yarp/sig/all.h"
#include "zfp.h"



class ZFPCompresserModule : public yarp::os::RFModule
{
    int numIter;
    bool interrupted;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelFloat> > portIn, portOut;


public:
    ZFPCompresserModule(int _numIter);
    ~ZFPCompresserModule();
    bool configure(yarp::os::ResourceFinder &rf);
    bool updateModule();
    double getPeriod();
    bool interruptModule();
    bool close();
protected:
//    int compressAndDecompress(float* array, int nx, int ny, float tolerance);
    int compress(float* array, float* &compressed, int &zfpsize, int nx, int ny, float tolerance);
    int decompress(float* array, float* &decompressed, int zfpsize, int nx, int ny, float tolerance);
};

#endif
