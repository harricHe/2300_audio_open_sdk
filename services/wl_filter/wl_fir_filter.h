/**************************************
-----------------filter.h---------------------
product name：fir filter
module name：hello
date：2020.12.04
auther：dylan 
file describe: fir_filter process head file 
***************************************/
#ifndef USER_FIR_H_  // NOLINT
#define USER_FIR_H_

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

//macro define
#define FILTER_LEN  64
#if 0
static double fir_coeffs[ FILTER_LEN ] ={
	 5.626776642e-01f, -3.218191161e-02f, -6.388021798e-02f,  3.956126516e-01f, -2.771835140e-01f,
	 1.204222772e-01f, -7.925080702e-02f, -4.399817091e-02f,  1.504590958e-01f, -1.260558178e-01f,
	 5.405172231e-02f,  8.882945492e-02f, -8.169507369e-02f,  4.807208281e-02f, -2.286305296e-02f,
	-2.217021693e-02f,  6.369311970e-02f, -2.686459465e-02f,  8.025052847e-03f,  4.450153793e-03f,
	 6.753048952e-03f,  3.921066144e-02f, -3.732490219e-02f,  7.075266251e-03f,  1.980631850e-02f,
	-1.215158012e-03f,  3.080498622e-02f, -3.238056490e-02f,  1.551775729e-02f,  4.215226548e-02f,
	-4.349127689e-02f,  1.233905663e-02f,  2.699123717e-02f, -1.149181096e-02f,  9.739744376e-03f,
	 2.111064246e-04f,  7.032851359e-03f,  9.388501439e-03f, -2.488517609e-03f,  7.661069023e-03f,
	 4.740738850e-03f,  1.534500022e-04f,  1.504705563e-03f,  1.013345916e-02f,  4.315081310e-03f,
	 7.515195461e-03f,  8.909145826e-03f, -2.158803010e-02f,  1.717111724e-02f,  1.497643096e-02f,
	// -2.419685558e-02f,  2.467641991e-02f,  8.443890621e-03f, -1.063484879e-02f,  1.497439109e-02f,
	// -1.142982410e-02f,  2.163654848e-02f,  1.266775447e-02f, -1.925359990e-02f,  2.389630340e-02f,
	// -2.801695909e-03f, -4.584951438e-03f,  4.600666481e-03f, -1.509269604e-02f,  2.028562874e-02f,
	// -4.707052527e-04f, -4.924167071e-03f,  2.094628676e-02f, -5.777102161e-03f,  1.223685028e-02f,
	//  5.396625817e-04f, -1.295388382e-02f,  1.715953352e-02f, -8.267654250e-03f,  2.793545012e-03f,
	//  1.233839461e-02f, -9.748750221e-03f,  1.020313117e-02f, -2.682249531e-03f, -5.104955352e-03f,
	//  1.309108430e-02f, -3.115788996e-03f,  6.339842903e-03f,  4.548679255e-03f, -5.746129352e-03f,
	//  1.003148599e-02f, -1.175071400e-03f, -3.030387376e-03f,  1.315116917e-02f,  2.782598507e-03f,
	//  1.451862857e-03f,  2.225570708e-03f, -3.527941597e-03f,  9.654624107e-03f,  3.144677412e-03f,
	// -8.269894843e-03f,  9.185257790e-03f,  4.180049122e-03f, -2.115683568e-03f,  2.017552305e-03f,
	// -3.566266700e-03f,  7.592957820e-03f,  1.602387529e-03f, -9.112940860e-03f,  7.931043055e-03f,
	//  3.828509280e-03f, -7.193995930e-04f,  2.800473587e-03f, -1.347155889e-03f,  7.292389898e-03f,
	//  1.332548659e-03f, -5.205128514e-03f,  6.493143773e-03f,  2.949376970e-03f,  1.813673987e-03f,
	//  3.485766436e-03f, -1.983992517e-03f,  5.713492287e-03f,  4.199488109e-03f, -3.090616674e-03f,
	//  2.735051656e-03f, -6.517540750e-04f,  4.482376207e-04f,  5.370245375e-03f, -2.689554772e-03f,
	// -1.104031955e-04f,  4.676893962e-03f, -2.136297416e-03f,  2.018573614e-03f,  5.603579136e-04f,
	//  2.482332709e-04f,  5.546622742e-03f, -3.796245861e-03f, -2.134618071e-04f,  6.152328686e-03f,
	// -1.421168228e-04f,  1.454570895e-03f, -6.507569883e-04f,  1.379918937e-03f,  4.673532541e-03f,
	// -6.045997525e-04f,  7.904534577e-06f,  1.224749697e-03f,  3.705453086e-03f,  1.238840213e-03f,
	// -1.702858005e-03f,  4.583559791e-03f,  2.207526557e-03f, -9.396117471e-04f, -6.291887860e-04f,
	// -1.246028302e-03f,  6.449918615e-03f,  1.373578748e-05f, -2.936265026e-03f,  3.985445694e-03f,
	// -1.434229978e-03f,  3.469426269e-03f,  1.443405319e-03f, -2.356678371e-03f,  3.804703344e-03f,
	//  2.415833563e-03f,  8.629805963e-04f, -2.644665453e-03f,  3.402234916e-04f,  9.063670336e-03f,
	// -4.591840401e-03f, -3.642199937e-03f,  5.811989863e-03f, -7.571899872e-04f,  6.955770479e-04f,
	//  1.649099866e-03f,  2.701565386e-03f,  4.343403417e-04f, -5.364396858e-03f,  4.714803122e-03f,
	//  8.263011372e-03f, -4.933326761e-03f, -5.014234913e-03f,  6.014365489e-03f,  4.320349629e-03f,
	// -3.463118803e-03f, -1.340267236e-03f,  3.124787209e-03f,  2.855839417e-03f,  1.525011594e-03f,
	// -9.520338444e-04f, -1.915316256e-04f,  4.407516021e-03f,  9.585502373e-04f, -4.509907477e-03f,
	//  4.694454785e-03f,  2.392832789e-03f, -3.591897078e-03f,  1.264832757e-03f,  1.396978590e-03f,
	//  4.088588747e-03f, -4.482112183e-04f, -3.063163731e-03f,  3.508714965e-03f,  6.414128363e-04f,
	//  4.489130891e-03f, -2.010933751e-03f, -6.180283646e-03f,  1.022122297e-02f, -2.394443373e-03f,
	// -5.073273391e-03f,  6.425911621e-03f, -3.548131557e-03f,  6.868116964e-03f, -8.253091658e-05f,
	// -9.375245863e-03f,  1.020369499e-02f, -1.758824387e-04f, -4.114332089e-03f,  3.515321036e-03f,
	// -1.871893631e-03f,  7.064083963e-03f, -2.360491137e-03f, -5.385025263e-03f,  7.636719573e-03f,
	// -3.888372685e-03f,  2.821668789e-03f, -1.511915645e-03f, -7.519272909e-03f,  1.276460247e-02f,
	// -4.919037401e-03f, -3.499787732e-03f,  6.654260626e-03f, -6.536182865e-03f,  6.932151569e-03f,
	// -4.728494376e-03f, -2.196459825e-03f,  1.098400079e-02f, -9.758874832e-03f,  5.471252970e-03f,
	// -5.083265956e-04f, -1.288733573e-03f,  3.306928781e-03f, -1.041971076e-02f,  1.571993892e-02f,
	// -9.549524558e-03f, -8.588508192e-04f,  1.349133257e-02f, -2.051625284e-02f,  2.269256750e-02f,
	// -1.797519410e-02f, -2.319198473e-03f,  2.642256785e-02f, -3.569306626e-02f,  3.300340179e-02f,
	// -2.570050688e-02f,  1.478515625e-02f,  4.214571168e-03f, -1.885658796e-02f,  2.657960429e-02f,
	// -2.551117680e-02f, 
};
#else
static double fir_coeffs[FILTER_LEN] = {
//#include "coef_4ka.txt"
#include "coef_4k_64.txt"
};
#endif

// 
void firFloatInit( void );
void firFloat( double *coeffs, double *input, double *output,int length, int filterLength );
void intToFloat( int16_t *input, double *output, int length );
void floatToInt( double *input, int16_t *output, int length );


#ifdef __cplusplus
}
#endif

#endif  // 

