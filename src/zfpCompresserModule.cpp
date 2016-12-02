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
    //ret &= NetworkBase::connect("/depthCamera/depthImage:o", portIn.getName());

    if(!ret) {
        yError()<<"Could not connect to some of the ports";
        return false;
    }
//    thread = new ThreadCompresser(33,buffer,portOut); //30 Hz;
//    thread->start();

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

    //imageOut.setExternal(decompressed,image->width(),image->height());
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
//      fwrite(buffer, 1, zfpsize, stdout);


    /* clean up */
    compressed = (float*) malloc(zfpsize);
    //memcpy(compressed,(float*)buffer,bufsize);
    memcpy(compressed,(float*) stream_data(zfp->stream),zfpsize);
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
//    stream=stream_open(array,nx*ny*10);
    decompressed = (float*) malloc(nx*ny*sizeof(float));
    field = zfp_field_2d(decompressed, type, nx, ny);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of three functions */
    /*  zfp_stream_set_rate(zfp, rate, type, 3, 0); */
    /*  zfp_stream_set_precision(zfp, precision, type); */
    zfp_stream_set_accuracy(zfp, tolerance, type);
//    yDebug()<<"1";
//    buffer = malloc(zfpsize);
//    yDebug()<<"2";
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

//int ZFPCompresserModule::compressAndDecompress(float* array, float* decompressed, int nx, int ny, float tolerance)
//{
//    int status = 0;    /* return value: 0 = success */
//    zfp_field* field;  /* array meta data */
//    zfp_type type;     /* array scalar type */
//    zfp_stream* zfp;   /* compressed stream */
//    zfp_stream* zfp2;
//    void* bufferComp;/* storage for compressed stream */
//    void* bufferDecomp;
//    size_t bufsize;    /* byte size of compressed bufferComp */
//    bitstream* stream; /* bit stream to write to or read from */
//    bitstream* stream2;
//    size_t zfpsize;    /* byte size of compressed stream */
////    std::vector<float> errorvec(nx*ny);
//    int rate=16;
//    uint precision=5;

//    bufferDecomp=malloc(nx*ny*4);

//    /* allocate meta data for the 3D array a[nz][ny][nx] */
//    type = zfp_type_float;
//    //      field = zfp_field_3d(array, type, nx, ny, nz);
//    field = zfp_field_2d(array,type,nx,ny);

//    /* allocate meta data for a compressed stream */
//    zfp = zfp_stream_open(NULL);

//    /* set compression mode and parameters via one of three functions */
//    //zfp_stream_set_rate(zfp, rate, type, 2, 0);
////    zfp_stream_set_precision(zfp, precision, type);
//    zfp_stream_set_accuracy(zfp, tolerance, type);

//    /* allocate buffer for compressed data */
//    bufsize = zfp_stream_maximum_size(zfp, field);
//    bufferComp = malloc(bufsize);

//    /* associate bit stream with allocated buffer */
//    stream = stream_open(bufferComp, bufsize);
//    zfp_stream_set_bit_stream(zfp, stream);
//    zfp_stream_rewind(zfp);

//    /* compress and decompress entire array */
//    /* compress array and output compressed stream */
//    zfpsize = zfp_compress(zfp, field);
//    if (!zfpsize) {
//        fprintf(stderr, "compression failed\n");
//        status = 1;
//        return status;
//    }
//    else
//        yInfo()<<"compression successful, ratio of compression:"<<(nx*ny*4.0)/(zfpsize)<<":1"<<"orgSize="
//              <<nx*ny*4.0<<"compressedSize="<<zfpsize;//4 -> float
//  //        else
//  //          fwrite(buffer, 1, zfpsize, stdout);

//    /* read compressed stream and decompress array */
//    memcpy(bufferComp,stream_data(zfp->stream),zfpsize);

//    /* allocate meta data for a compressed stream */
//    zfp2 = zfp_stream_open(NULL);

//    /* set compression mode and parameters via one of three functions */
////    zfp_stream_set_rate(zfp, rate, type, 2, 0);
//    zfp_stream_set_accuracy(zfp2, tolerance, type);
////    zfp_stream_set_precision(zfp, precision, type);

//    /* allocate buffer for compressed data */
//    bufsize = zfp_stream_maximum_size(zfp2, field);
////    bufferComp = malloc(bufsize);

//    /* associate bit stream with allocated buffer */
//    stream2 = stream_open(bufferDecomp, bufsize);
//    zfp_stream_set_bit_stream(zfp2, stream2);
//    zfp_stream_rewind(zfp2);
//    if (!zfp_decompress(zfp2, field)) {
//      fprintf(stderr, "decompression failed\n");
//      status = 1;
//      return status;
//    }
//    else
//      yInfo()<<"decompression successful";

////    for(int j=0;j<ny;j++){
////        for(int i=0;i<nx;i++){
////            errorvec[i + nx * (j)]=fabs(array_org[i + nx * (j)]-((float*) field->data)[i + nx * (j)]);
////        }
////    }
////    yInfo()<<"Max error:"<<*std::max_element(errorvec.begin(),errorvec.end())<<"Average error"
////          <<accumulate( errorvec.begin(), errorvec.end(), 0.0)/errorvec.size();;

//    /* clean up */


//    //ImageOf<PixelFloat> image((float*)field->data);

////    ImageOf<PixelFloat> &image = portOut.prepare();
////    //image.setPixelCode(VOCAB_PIXEL_MONO_FLOAT);
////    image.setExternal(/*field->data*/array, nx, ny);
////    portOut.write();



////    Bottle& output = portOut.prepare();
////    output.clear();
////    output.
////    cout << "writing " << output.toString().c_str() << endl;
////    portOut.write();

//    //zfp_field_free(field);
//    zfp_stream_close(zfp);
//    stream_close(stream);
//    free(bufferComp);
//    free(bufferDecomp);

//    return status;
//}


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
//    while(thread->getNumProcessed() < numIter && !interrupted)
//        yarp::os::Time::delay(0.1);

    //stop thread
//    thread->close();
    //close ports
    portIn.close();
    portOut.close();
    //deallocate memory
    delete thread;
    return true;
}
