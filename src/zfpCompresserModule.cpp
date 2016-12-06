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

    if(!ret) {
        yError()<<"Could not connect to some of the ports";
        return false;
    }


    return true;
}
bool ZFPCompresserModule::updateModule(){

    static int count = numIter;
    if(count<=0) {
        yDebug()<<"finishing acquisition...";
        return false;
    }

    // read port


    yInfo() <<"ZFPCompresserModule:acquiring images...";
    ImageOf<PixelFloat> *image = portIn.read();

    if (!image) {
        yError()<<"Acquisition failed";
        return false;
    }

    yInfo() <<"ZFPCompresserModule:acquiring images [done]";

    //Stamp s;
    float *compressed = NULL;
    float *decompressed=NULL;
    int sizeCompressed;

    //count -= 1; //comment for an infinite loop
    compress((float*)image->getRawImage(), compressed, sizeCompressed, image->width(),image->height(),1e-3);
    decompress(compressed, decompressed, sizeCompressed, image->width(),image->height(),1e-3);

    ImageOf<PixelFloat> &imageOut=portOut.prepare();
    //yDebug()<<decompressed[100];OK
    if(!decompressed){
        yError()<<"Failed to decompress, exiting...";
        return false;
    }

    //imageOut.setExternal(decompressed,image->width(),image->height());//segmentation fault!!
    imageOut.resize(image->width(),image->height());
    memcpy(imageOut.getRawImage(),decompressed,image->width()*image->height()*4);
    portOut.write();
    if(compressed)
        free(compressed);
    if(decompressed)
        free(decompressed);
    return true;
}

int ZFPCompresserModule::compress(float* array, float* &compressed, int &zfpsize, int nx, int ny, float tolerance){
    int status = 0;    /* return value: 0 = success */
    zfp_type type;     /* array scalar type */
    zfp_field* field;  /* array meta data */
    zfp_stream* zfp;   /* compressed stream */
    void* buffer;      /* storage for compressed stream */
    size_t bufsize;    /* byte size of compressed buffer */
    bitstream* stream; /* bit stream to write to or read from */

    type = zfp_type_float;
    field = zfp_field_2d(array, type, nx, ny);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of three functions */
    /*  zfp_stream_set_rate(zfp, rate, type, 3, 0); */
    /*  zfp_stream_set_precision(zfp, precision, type); */
    zfp_stream_set_accuracy(zfp, tolerance, type);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    buffer = malloc(bufsize);

    /* associate bit stream with allocated buffer */
    stream = stream_open(buffer, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    /* compress entire array */
    /* compress array and output compressed stream */
    zfpsize = zfp_compress(zfp, field);
    if (!zfpsize) {
      fprintf(stderr, "compression failed\n");
      status = 1;
    }
    else
        yInfo()<<"compression successful, ratio of compression:"<<(nx*ny*4.0)/(zfpsize)<<":1"<<"orgSize="
              <<nx*ny*4.0<<"compressedSize="<<zfpsize;//4 -> float


    compressed = (float*) malloc(zfpsize);
    memcpy(compressed,(float*) stream_data(zfp->stream),zfpsize);

    /* clean up */
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    free(buffer);

    return status;
}

int ZFPCompresserModule::decompress(float* array, float* &decompressed, int zfpsize, int nx, int ny, float tolerance){
    int status = 0;    /* return value: 0 = success */
    zfp_type type;     /* array scalar type */
    zfp_field* field;  /* array meta data */
    zfp_stream* zfp;   /* compressed stream */
    void* buffer;      /* storage for compressed stream */
    size_t bufsize;    /* byte size of compressed buffer */
    bitstream* stream; /* bit stream to write to or read from */

    type = zfp_type_float;
    decompressed = (float*) malloc(nx*ny*sizeof(float));
    field = zfp_field_2d(decompressed, type, nx, ny);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of three functions */
    /*  zfp_stream_set_rate(zfp, rate, type, 3, 0); */
    /*  zfp_stream_set_precision(zfp, precision, type); */
    zfp_stream_set_accuracy(zfp, tolerance, type);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    buffer = malloc(bufsize);
    memcpy(buffer,array,zfpsize);


//    /* associate bit stream with allocated buffer */
    stream = stream_open(buffer, zfpsize);
    yDebug()<<"3";
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    /* compress or decompress entire array */
    /* read compressed stream and decompress array */
    //zfpsize = fread(buffer, 1, bufsize, stdin);
    //devo metterci il mio puntatore al dato compresso.
    if (!zfp_decompress(zfp, field)) {
      fprintf(stderr, "decompression failed\n");
      status = 1;
    }
    else
        yInfo()<<"Decompression successful";
    yDebug()<<"bufsize"<<bufsize<<"zfpsize"<<zfpsize;
    //std::cout<<"test decompressed data "<<((float*) field->data)[76799]<<std::endl; //OK
//    ((float*) field->data)[i + nx * (j)]
    //nx*ny*sizeof(float)
    //memcpy(decompressed,(float*) field->data,nx*ny*sizeof(float));


    /* clean up */
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    free(buffer);

    return status;

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
    //close ports
    portIn.close();
    portOut.close();
    return true;
}
