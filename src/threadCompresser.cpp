#include"threadCompresser.h"
#include "yarp/sig/all.h"
#include "zfp.h"
#include <yarp/sig/IplImage.h>

using namespace yarp::os;
using namespace yarp::sig;

ThreadCompresser::ThreadCompresser(int period,SafeBuffer<ImageOf<PixelFloat>> &buffer, yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelFloat>> &portOut):RateThread(period){
    ThreadCompresser::portOut =&portOut;
    ThreadCompresser::buffer = &buffer;
    interrupted = false;
    numProcessed=0;
    field = NULL;

}

ThreadCompresser::~ThreadCompresser(){
    if(field)
       zfp_field_free(field);
}

int ThreadCompresser::compressAndDecompress(float* array, int nx, int ny, float tolerance)
{
    int status = 0;    /* return value: 0 = success */
    zfp_type type;     /* array scalar type */
    zfp_stream* zfp;   /* compressed stream */
    zfp_stream* zfp2;
    void* bufferComp;/* storage for compressed stream */
    void* bufferDecomp;
    size_t bufsize;    /* byte size of compressed bufferComp */
    bitstream* stream; /* bit stream to write to or read from */
    bitstream* stream2;
    size_t zfpsize;    /* byte size of compressed stream */
//    std::vector<float> errorvec(nx*ny);
    int rate=16;
    uint precision=5;

    bufferDecomp=malloc(nx*ny*4);

    /* allocate meta data for the 3D array a[nz][ny][nx] */
    type = zfp_type_float;
    //      field = zfp_field_3d(array, type, nx, ny, nz);
    field = zfp_field_2d(array,type,nx,ny);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of three functions */
    //zfp_stream_set_rate(zfp, rate, type, 2, 0);
//    zfp_stream_set_precision(zfp, precision, type);
    zfp_stream_set_accuracy(zfp, tolerance, type);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    bufferComp = malloc(bufsize);

    /* associate bit stream with allocated buffer */
    stream = stream_open(bufferComp, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    /* compress and decompress entire array */
    /* compress array and output compressed stream */
    zfpsize = zfp_compress(zfp, field);
    if (!zfpsize) {
        fprintf(stderr, "compression failed\n");
        status = 1;
        return status;
    }
    else
        yInfo()<<"compression successful, ratio of compression:"<<(nx*ny*4.0)/(zfpsize)<<":1"<<"orgSize="
              <<nx*ny*4.0<<"compressedSize="<<zfpsize;//4 -> float
  //        else
  //          fwrite(buffer, 1, zfpsize, stdout);

    /* read compressed stream and decompress array */
    memcpy(bufferComp,stream_data(zfp->stream),zfpsize);

    /* allocate meta data for a compressed stream */
    zfp2 = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of three functions */
//    zfp_stream_set_rate(zfp, rate, type, 2, 0);
    zfp_stream_set_accuracy(zfp2, tolerance, type);
//    zfp_stream_set_precision(zfp, precision, type);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp2, field);
//    bufferComp = malloc(bufsize);

    /* associate bit stream with allocated buffer */
    stream2 = stream_open(bufferDecomp, bufsize);
    zfp_stream_set_bit_stream(zfp2, stream2);
    zfp_stream_rewind(zfp2);
    if (!zfp_decompress(zfp2, field)) {
      fprintf(stderr, "decompression failed\n");
      status = 1;
      return status;
    }
    else
      yInfo()<<"decompression successful";

//    for(int j=0;j<ny;j++){
//        for(int i=0;i<nx;i++){
//            errorvec[i + nx * (j)]=fabs(array_org[i + nx * (j)]-((float*) field->data)[i + nx * (j)]);
//        }
//    }
//    yInfo()<<"Max error:"<<*std::max_element(errorvec.begin(),errorvec.end())<<"Average error"
//          <<accumulate( errorvec.begin(), errorvec.end(), 0.0)/errorvec.size();;

    /* clean up */


    //ImageOf<PixelFloat> image((float*)field->data);

    ImageOf<PixelFloat> &image = portOut->prepare();
//    image.resize(nx, ny);
    //image.setPixelCode(VOCAB_PIXEL_MONO_FLOAT);
    image.setExternal(/*field->data*/array, nx, ny);
    portOut->writeStrict();
    portOut->write();

//    Bottle& output = portOut.prepare();
//    output.clear();
//    output.
//    cout << "writing " << output.toString().c_str() << endl;
//    portOut.write();

    //zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    free(bufferComp);
    free(bufferDecomp);

    return status;
}

int ThreadCompresser::getNumProcessed(){
    return numProcessed;
}

void ThreadCompresser::run(){
    yInfo()<<"ZFPCompresser is running...";
    ImageOf<PixelFloat> image;
    yInfo()<<"ThreadCompresser reading from the buffer...";
    if(!interrupted){
        if(buffer->read(image)){
            //compression & de-compression
            double inizio=Time::now();
            compressAndDecompress((float*) image.getRawImage(),320,240,1e-3);
            double fine=Time::now();
            yInfo()<<fine-inizio;
            numProcessed++;
            yInfo()<<"NumProcessed"<<numProcessed;
            //send to the port
        }
        else
            yInfo()<<"ThreadCompresser has been interrupted on read";
    }




}

void ThreadCompresser::interrupt(){
        interrupted = true;
        buffer->interrupt();
    }

void ThreadCompresser::close() {
    interrupt();
    RateThread::stop();
    numProcessed = 0;
    interrupted = false;
}


