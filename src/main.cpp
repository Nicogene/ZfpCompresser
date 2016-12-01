#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <zfp.h>
#include <yarp/os/all.h>
#include "zfpCompresserModule.h"



int main (int argc, char** argv) {
    yarp::os::Network yarp;
    ZFPCompresserModule mod(10);
    yarp::os::ResourceFinder rf;
    rf.configure(argc,argv);
    //it calls .configure and then .updateModule
    mod.runModule(rf);

      return 0;
}
