#include "zfpCompresserModule.h"
using namespace yarp::os;
using namespace yarp::sig;


ZFPCompresserModule::ZFPCompresserModule(int _numIter):numIter(_numIter), interrupted(false){

}
ZFPCompresserModule::~ZFPCompresserModule(){

}

bool ZFPCompresserModule::configure(yarp::os::ResourceFinder &rf){
    //Open ports
    bool ret=portIn.open("/ZFPCompresser/depthImage:i");
    ret &= portOut.open("/ZFPCompresser/depthImage:o");

    //Connect ports
    ret &= NetworkBase::connect("/depthCamera/depthImage:o", portIn.getName());

    if(!ret) {
        yError()<<"Could not connect to some of the ports";
        return false;
    }
    thread = new ThreadCompresser(33,buffer,portOut); //30 Hz;
    thread->start();

    return true;
}
bool ZFPCompresserModule::updateModule(){

    static int count = numIter;
    if(count<=0) {
        yDebug()<<"finishing acquisition...";
        return false;
    }

    // read port


    yInfo() <<"vgSLAMModule:acquiring images...";
    ImageOf<PixelFloat> *image = portIn.read();

    if (!image) {
        yError()<<"Acquisition failed";
        return false;
    }

    yInfo() <<"ZFPCompresserModule:acquiring images [done]";

    Stamp sL,sR;
    //count -= 1; //comment for an infinite loop

    yInfo() <<"ZFPCompresserModule:writing image to buffers...";
    buffer.write(*image);
    return true;
}
double ZFPCompresserModule::getPeriod(){
    return 0.0;
}
bool ZFPCompresserModule::interruptModule(){
    //interrupt ports
    interrupted=true;
    portIn.interrupt();
    portOut.interrupt();
    return true;
}
bool ZFPCompresserModule::close(){

    yInfo()<<"Waiting for worker thread...";
    while(thread->getNumProcessed() < numIter && !interrupted)
        yarp::os::Time::delay(0.1);

    //stop thread
    thread->close();
    //close ports
    portIn.close();
    portOut.close();
    //deallocate memory
    delete thread;
    return true;
}
