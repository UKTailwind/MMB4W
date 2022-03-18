/***********************************************************************************************************************
MMBasic for Windows

Maths.cpp

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
	in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed
	on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed
	by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software
	without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

************************************************************************************************************************/


#include "olcPixelGameEngine.h"
#include "MainThread.h"
#include "MMBasic_Includes.h"
#include <complex>
#include <valarray>

#pragma warning(disable : 6011)
MMFLOAT PI;
using namespace std;
typedef std::complex<double> cplx;
void cmd_FFT(unsigned char* pp);
const double chitable[51][15] = {
		{0.995,0.99,0.975,0.95,0.9,0.5,0.2,0.1,0.05,0.025,0.02,0.01,0.005,0.002,0.001},
		{0.0000397,0.000157,0.000982,0.00393,0.0158,0.455,1.642,2.706,3.841,5.024,5.412,6.635,7.879,9.550,10.828},
		{0.0100,0.020,0.051,0.103,0.211,1.386,3.219,4.605,5.991,7.378,7.824,9.210,10.597,12.429,13.816},
		{0.072,0.115,0.216,0.352,0.584,2.366,4.642,6.251,7.815,9.348,9.837,11.345,12.838,14.796,16.266},
		{0.207,0.297,0.484,0.711,1.064,3.357,5.989,7.779,9.488,11.143,11.668,13.277,14.860,16.924,18.467},
		{0.412,0.554,0.831,1.145,1.610,4.351,7.289,9.236,11.070,12.833,13.388,15.086,16.750,18.907,20.515},
		{0.676,0.872,1.237,1.635,2.204,5.348,8.558,10.645,12.592,14.449,15.033,16.812,18.548,20.791,22.458},
		{0.989,1.239,1.690,2.167,2.833,6.346,9.803,12.017,14.067,16.013,16.622,18.475,20.278,22.601,24.322},
		{1.344,1.646,2.180,2.733,3.490,7.344,11.030,13.362,15.507,17.535,18.168,20.090,21.955,24.352,26.124},
		{1.735,2.088,2.700,3.325,4.168,8.343,12.242,14.684,16.919,19.023,19.679,21.666,23.589,26.056,27.877},
		{2.156,2.558,3.247,3.940,4.865,9.342,13.442,15.987,18.307,20.483,21.161,23.209,25.188,27.722,29.588},
		{2.603,3.053,3.816,4.575,5.578,10.341,14.631,17.275,19.675,21.920,22.618,24.725,26.757,29.354,31.264},
		{3.074,3.571,4.404,5.226,6.304,11.340,15.812,18.549,21.026,23.337,24.054,26.217,28.300,30.957,32.909},
		{3.565,4.107,5.009,5.892,7.042,12.340,16.985,19.812,22.362,24.736,25.472,27.688,29.819,32.535,34.528},
		{4.075,4.660,5.629,6.571,7.790,13.339,18.151,21.064,23.685,26.119,26.873,29.141,31.319,34.091,36.123},
		{4.601,5.229,6.262,7.261,8.547,14.339,19.311,22.307,24.996,27.488,28.259,30.578,32.801,35.628,37.697},
		{5.142,5.812,6.908,7.962,9.312,15.338,20.465,23.542,26.296,28.845,29.633,32.000,34.267,37.146,39.252},
		{5.697,6.408,7.564,8.672,10.085,16.338,21.615,24.769,27.587,30.191,30.995,33.409,35.718,38.648,40.790},
		{6.265,7.015,8.231,9.390,10.865,17.338,22.760,25.989,28.869,31.526,32.346,34.805,37.156,40.136,42.312},
		{6.844,7.633,8.907,10.117,11.651,18.338,23.900,27.204,30.144,32.852,33.687,36.191,38.582,41.610,43.820},
		{7.434,8.260,9.591,10.851,12.443,19.337,25.038,28.412,31.410,34.170,35.020,37.566,39.997,43.072,45.315},
		{8.034,8.897,10.283,11.591,13.240,20.337,26.171,29.615,32.671,35.479,36.343,38.932,41.401,44.522,46.797},
		{8.643,9.542,10.982,12.338,14.041,21.337,27.301,30.813,33.924,36.781,37.659,40.289,42.796,45.962,48.268},
		{9.260,10.196,11.689,13.091,14.848,22.337,28.429,32.007,35.172,38.076,38.968,41.638,44.181,47.391,49.728},
		{9.886,10.856,12.401,13.848,15.659,23.337,29.553,33.196,36.415,39.364,40.270,42.980,45.559,48.812,51.179},
		{10.520,11.524,13.120,14.611,16.473,24.337,30.675,34.382,37.652,40.646,41.566,44.314,46.928,50.223,52.620},
		{11.160,12.198,13.844,15.379,17.292,25.336,31.795,35.563,38.885,41.923,42.856,45.642,48.290,51.627,54.052},
		{11.808,12.879,14.573,16.151,18.114,26.336,32.912,36.741,40.113,43.195,44.140,46.963,49.645,53.023,55.476},
		{12.461,13.565,15.308,16.928,18.939,27.336,34.027,37.916,41.337,44.461,45.419,48.278,50.993,54.411,56.892},
		{13.121,14.256,16.047,17.708,19.768,28.336,35.139,39.087,42.557,45.722,46.693,49.588,52.336,55.792,58.301},
		{13.787,14.953,16.791,18.493,20.599,29.336,36.250,40.256,43.773,46.979,47.962,50.892,53.672,57.167,59.703},
		{14.458,15.655,17.539,19.281,21.434,30.336,37.359,41.422,44.985,48.232,49.226,52.191,55.003,58.536,61.098},
		{15.134,16.362,18.291,20.072,22.271,31.336,38.466,42.585,46.194,49.480,50.487,53.486,56.328,59.899,62.487},
		{15.815,17.074,19.047,20.867,23.110,32.336,39.572,43.745,47.400,50.725,51.743,54.776,57.648,61.256,63.870},
		{16.501,17.789,19.806,21.664,23.952,33.336,40.676,44.903,48.602,51.966,52.995,56.061,58.964,62.608,65.247},
		{17.192,18.509,20.569,22.465,24.797,34.336,41.778,46.059,49.802,53.203,54.244,57.342,60.275,63.955,66.619},
		{17.887,19.233,21.336,23.269,25.643,35.336,42.879,47.212,50.998,54.437,55.489,58.619,61.581,65.296,67.985},
		{18.586,19.960,22.106,24.075,26.492,36.336,43.978,48.363,52.192,55.668,56.730,59.892,62.883,66.633,69.346},
		{19.289,20.691,22.878,24.884,27.343,37.335,45.076,49.513,53.384,56.896,57.969,61.162,64.181,67.966,70.703},
		{19.996,21.426,23.654,25.695,28.196,38.335,46.173,50.660,54.572,58.120,59.204,62.428,65.476,69.294,72.055},
		{20.707,22.164,24.433,26.509,29.051,39.335,47.269,51.805,55.758,59.342,60.436,63.691,66.766,70.618,73.402},
		{21.421,22.906,25.215,27.326,29.907,40.335,48.363,52.949,56.942,60.561,61.665,64.950,68.053,71.938,74.745},
		{22.138,23.650,25.999,28.144,30.765,41.335,49.456,54.090,58.124,61.777,62.892,66.206,69.336,73.254,76.084},
		{22.859,24.398,26.785,28.965,31.625,42.335,50.548,55.230,59.304,62.990,64.116,67.459,70.616,74.566,77.419},
		{23.584,25.148,27.575,29.787,32.487,43.335,51.639,56.369,60.481,64.201,65.337,68.710,71.893,75.874,78.750},
		{24.311,25.901,28.366,30.612,33.350,44.335,52.729,57.505,61.656,65.410,66.555,69.957,73.166,77.179,80.077},
		{25.041,26.657,29.160,31.439,34.215,45.335,53.818,58.641,62.830,66.617,67.771,71.201,74.437,78.481,81.400},
		{25.775,27.416,29.956,32.268,35.081,46.335,54.906,59.774,64.001,67.821,68.985,72.443,75.704,79.780,82.720},
		{26.511,28.177,30.755,33.098,35.949,47.335,55.993,60.907,65.171,69.023,70.197,73.683,76.969,81.075,84.037},
		{27.249,28.941,31.555,33.930,36.818,48.335,57.079,62.038,66.339,70.222,71.406,74.919,78.231,82.367,85.351},
		{27.991,29.707,32.357,34.764,37.689,49.335,58.164,63.167,67.505,71.420,72.613,76.154,79.490,83.657,86.661}
};
MMFLOAT q[4] = { 1,0,0,0 };
MMFLOAT eInt[3] = { 0,0,0 };
MMFLOAT determinant(MMFLOAT** matrix, int size);
void transpose(MMFLOAT** matrix, MMFLOAT** matrix_cofactor, MMFLOAT** newmatrix, int size);
void cofactor(MMFLOAT** matrix, MMFLOAT** newmatrix, int size);
static void floatshellsort(MMFLOAT a[], int n) {
	long h, l, j;
	MMFLOAT k;
	for (h = n; h /= 2;) {
		for (l = h; l < n; l++) {
			k = a[l];
			for (j = l; j >= h && k < a[j - h]; j -= h) {
				a[j] = a[j - h];
			}
			a[j] = k;
		}
	}
}

static MMFLOAT* alloc1df(int n)
{
	//    int i;
	MMFLOAT* array;
	if ((array = (MMFLOAT*)GetMemory(n * sizeof(MMFLOAT))) == NULL) {
		error((char *)"Unable to allocate memory for 1D float array...\n");
		exit(0);
	}

	//    for (i = 0; i < n; i++) {
	//        array[i] = 0.0;
	//    }

	return array;
}

static MMFLOAT** alloc2df(int m, int n)
{
	int i;
	MMFLOAT** array;
	if ((array = (MMFLOAT**)GetMemory(m * sizeof(MMFLOAT*))) == NULL) {
		error((char *)"Unable to allocate memory for 2D float array...\n");
		exit(0);
	}

	for (i = 0; i < m; i++) {
		array[i] = alloc1df(n);
	}

	return array;
}

static void dealloc2df(MMFLOAT** array, int m, int n)
{
	int i;
	for (i = 0; i < m; i++) {
		FreeMemory((unsigned char*)array[i]);
		array[i] = NULL;
	}

	FreeMemory((unsigned char*)array);
	array = NULL;
}
extern "C" unsigned char*** alloc3df(int l, int m, int n)
{
	int i;
	uint8_t*** array;

	if ((array = (uint8_t***)GetMemory(l * sizeof(uint8_t**))) == NULL) {
		error((char *)"Unable to allocate memory for 3D float array...\n");
		exit(0);
	}

	for (i = 0; i < l; i++) {
		array[i] = (uint8_t **)alloc2df(m, n);
	}

	return array;
}
extern "C" void dealloc3df(uint8_t*** array, int l, int m, int n)
{
	int i;
	for (i = 0; i < l; i++) {
		dealloc2df((MMFLOAT **)array[i], m, n);
	}

	FreeMemorySafe((void**)&array);
}

void Q_Mult(MMFLOAT* q1, MMFLOAT* q2, MMFLOAT* n) {
	MMFLOAT a1 = q1[0], a2 = q2[0], b1 = q1[1], b2 = q2[1], c1 = q1[2], c2 = q2[2], d1 = q1[3], d2 = q2[3];
	n[0] = a1 * a2 - b1 * b2 - c1 * c2 - d1 * d2;
	n[1] = a1 * b2 + b1 * a2 + c1 * d2 - d1 * c2;
	n[2] = a1 * c2 - b1 * d2 + c1 * a2 + d1 * b2;
	n[3] = a1 * d2 + b1 * c2 - c1 * b2 + d1 * a2;
	n[4] = q1[4] * q2[4];
}

void Q_Invert(MMFLOAT* q, MMFLOAT* n) {
	n[0] = q[0];
	n[1] = -q[1];
	n[2] = -q[2];
	n[3] = -q[3];
	n[4] = q[4];
}
double sincpi(double x)
/*
 * This atrociously slow function call should eventually be replaced
 * by a tabular approach.
 */
{
	if (x == 0.0)
		return (1.0);
	return (sin(M_PI * x) / (M_PI * x));
}

int upsample(MMFLOAT* x, MMFLOAT* y, int xsamps, MMFLOAT up, MMFLOAT down)
/*
 * Convert low frequency data x, having xsamps 16-bit samples, to a
 * certain number ysamps of high-frequency y samples. The exact
 * number ysamps is the return value of this function, said value
 * determined by a formula in the declarations below. The high and
 * low frequencies are up, down respectively.
 */
{
	double down_ratio = down / up;
	double a, b;
	int ysamps = 1 + (int)((xsamps - 6) / down_ratio);
	int i, j;

	for (j = 0; j < ysamps; j++) {
		b = j * down_ratio;
		i = (int)b;
		a = b - i;
		y[j] = /*0.5 * */(
			x[i] * sincpi(2 + a) + x[i + 1] * sincpi(1 + a) +
			x[i + 2] * sincpi(a) + x[i + 3] * sincpi(1 - a) +
			x[i + 4] * sincpi(2 - a) + x[i + 5] * sincpi(3 - a));
	}
	return (ysamps);
}
void cmd_math(void) {
	unsigned char* tp;
	int t = T_NBR;
	MMFLOAT f;
	long long int i64;
	unsigned char* s;

	skipspace(cmdline);
	if (toupper(*cmdline) == 'S') {

		tp = checkstring(cmdline, (unsigned char *)"SET");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, card1 = 1;
			MMFLOAT* a1float = NULL;
			int64_t* a1int = NULL;
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			evaluate(argv[0], &f, &i64, &s, &t, false);
			if (t & T_STR) error((char *)"Syntax");
			ptr1 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be numerical");
			if (a1float != NULL) {
				for (i = 0; i < card1; i++)*a1float++ = ((t & T_INT) ? (MMFLOAT)i64 : f);
			}
			else {
				for (i = 0; i < card1; i++)*a1int++ = ((t & T_INT) ? i64 : FloatToInt64(f));
			}
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"SCALE");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int i, j, card1 = 1, card2 = 1;
			MMFLOAT* a1float = NULL, * a2float = NULL, scale;
			int64_t* a1int = NULL, * a2int = NULL;
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			evaluate(argv[2], &f, &i64, &s, &t, false);
			if (t & T_STR) error((char *)"Syntax");
			scale = getnumber(argv[2]);
			ptr2 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)a2float != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2int = (int64_t*)ptr2;
				if ((uint32_t)a2int != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be numerical");
			if (card1 != card2)error((char *)"Size mismatch");
			if (scale != 1.0) {
				if (a2float != NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)*a2float++ = ((t & T_INT) ? (MMFLOAT)i64 : f) * (*a1float++);
				}
				else if (a2float != NULL && a1float == NULL) {
					for (i = 0; i < card1; i++)(*a2float++) = ((t & T_INT) ? (MMFLOAT)i64 : f) * ((MMFLOAT)*a1int++);
				}
				else if (a2float == NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)(*a2int++) = FloatToInt64(((t & T_INT) ? i64 : FloatToInt64(f)) * (*a1float++));
				}
				else {
					for (i = 0; i < card1; i++)(*a2int++) = ((t & T_INT) ? i64 : FloatToInt64(f)) * (*a1int++);
				}
			}
			else {
				if (a2float != NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)*a2float++ = *a1float++;
				}
				else if (a2float != NULL && a1float == NULL) {
					for (i = 0; i < card1; i++)(*a2float++) = ((MMFLOAT)*a1int++);
				}
				else if (a2float == NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)(*a2int++) = FloatToInt64(*a1float++);
				}
				else {
					for (i = 0; i < card1; i++)*a2int++ = *a1int++;
				}
			}
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"SLICE");
		if (tp) {
			int i, j, start, increment, dim[MAXDIM], pos[MAXDIM], off[MAXDIM], dimcount = 0, target = -1, toarray = 0;
			int64_t* a1int = NULL, * a2int = NULL;
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			getargs(&tp, 13, (unsigned char *)",");
			if (argc < 7)error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & (T_NBR | T_INT)) {
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D or more numerical array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D or more numerical array");
				}
				for (i = 0; i < MAXDIM; i++) {
					if (vartbl[VarIndex].dims[i] - OptionBase > 0) {
						dimcount++;
						dim[i] = vartbl[VarIndex].dims[i] - OptionBase;
					}
					else dim[i] = 0;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 2D or more numerical array");
			if (((argc - 1) / 2 - 1) != dimcount)error((char *)"Argument count");
			for (i = 0; i < dimcount; i++) {
				if (*argv[i * 2 + 2]) pos[i] = (int)getint(argv[i * 2 + 2], OptionBase, dim[i] + OptionBase) - OptionBase;
				else {
					if (target != -1)error((char *)"Only one index can be omitted");
					target = i;
					pos[i] = 1;
				}
			}
			ptr2 = findvar(argv[i * 2 + 2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & (T_NBR | T_INT)) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Target must be 1D a numerical point array");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Target must be 1D a numerical point array");
				}
				toarray = vartbl[VarIndex].dims[0] - OptionBase;
				a2int = (int64_t*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Target must be 1D a numerical point array");
			if (dim[target] != toarray)error((char *)"Size mismatch between slice and target array");
			i = dimcount - 1;
			while (i >= 0) {
				off[i] = 1;
				for (j = 0; j < i; j++)off[i] *= (dim[j] + 1);
				i--;
			}
			start = 1;
			for (i = 0; i < dimcount; i++) {
				start += (pos[i] * off[i]);
			}
			start--;
			increment = off[target];
			start -= increment;
			for (i = 0; i <= dim[target]; i++)*a2int++ = a1int[start + i * increment];
			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"SENSORFUSION");
		if (tp) {
			cmd_SensorFusion((char *)tp);
			return;
		}
	}
	else if (toupper(*cmdline) == 'V') {
		tp = checkstring(cmdline, (unsigned char *)"V_MULT");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			void* ptr3 = NULL;
			int i, j, numcols = 0, numrows = 0;
			MMFLOAT* a1float = NULL, * a2float = NULL, * a2sfloat = NULL, * a3float = NULL;
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D floating point array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				numrows = vartbl[VarIndex].dims[1] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 2D floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				a2float = a2sfloat = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a floating point array");
			ptr3 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 3 must be a floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numrows)error((char *)"Array size mismatch");
				a3float = (MMFLOAT*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be a floating point array");
			if (ptr3 == ptr1 || ptr3 == ptr2)error((char *)"Destination array same as source");
			numcols++;
			numrows++;
			for (i = 0; i < numrows; i++) {
				a2float = a2sfloat;
				*a3float = 0.0;
				for (j = 0; j < numcols; j++) {
					*a3float = *a3float + ((*a1float++) * (*a2float++));
				}
				a3float++;
			}
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"V_NORMALISE");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int j, numrows = 0;
			MMFLOAT* a1float = NULL, * a1sfloat = NULL, * a2float = NULL, mag = 0.0;
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a floating point array");
				}
				numrows = vartbl[VarIndex].dims[0] - OptionBase;
				a1float = a1sfloat = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numrows)error((char *)"Array size mismatch");
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a floating point array");
			numrows++;
			for (j = 0; j < numrows; j++) {
				mag += (*a1sfloat) * (*a1sfloat);
				a1sfloat++;
			}
			mag = sqrt(mag);
			for (j = 0; j < numrows; j++) {
				*a2float++ = (*a1float++) / mag;
			}
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"V_CROSS");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			void* ptr3 = NULL;
			int j, numcols = 0;
			MMFLOAT* a1float = NULL, * a2float = NULL, * a3float = NULL;
			MMFLOAT a[3], b[3];
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 3 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 2)error((char *)"Argument 1 must be a 3 element floating point array");
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 3 element floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a floating point array");
			ptr3 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 3 must be a floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				a3float = (MMFLOAT*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be a floating point array");
			numcols++;
			for (j = 0; j < numcols; j++) {
				a[j] = *a1float++;
				b[j] = *a2float++;
			}
			*a3float++ = a[1] * b[2] - a[2] * b[1];
			*a3float++ = a[2] * b[0] - a[0] * b[2];
			*a3float = a[0] * b[1] - a[1] * b[0];
			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"V_PRINT");
		if (tp) {
			void* ptr1 = NULL;
			int j, numcols = 0;
			MMFLOAT* a1float = NULL;
			int64_t* a1int = NULL;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (ptr1 && vartbl[VarIndex].type & T_INT) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a numerical array");
			numcols++;
			if (a1float != NULL) {
				PFlt(*a1float++);
				for (j = 1; j < numcols; j++)PFltComma(*a1float++);
				PRet();
			}
			else {
				PInt(*a1int++);
				for (j = 1; j < numcols; j++)PIntComma(*a1int++);
				PRet();
			}
			return;
		}
	}
	else if (toupper(*cmdline) == 'M') {
		tp = checkstring(cmdline, (unsigned char *)"M_INVERSE");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int i, j, n, numcols = 0, numrows = 0;
			MMFLOAT* a1float = NULL, * a2float = NULL, det;
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				numrows = vartbl[VarIndex].dims[1] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else	error((char *)"Argument 1 must be a numerical 2D array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a numerical 2D array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a numerical 2D array");
				}
				if (numcols != vartbl[VarIndex].dims[0] - OptionBase)error((char *)"array size mismatch");
				if (numrows != vartbl[VarIndex].dims[1] - OptionBase)error((char *)"array size mismatch");
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else	error((char *)"Argument 2 must be a numerical 2D array");
			if (numcols != numrows)error((char *)"Array must be square");
			n = numrows + 1;
			MMFLOAT** matrix = alloc2df(n, n);
			for (i = 0; i < n; i++) { //load the matrix
				for (j = 0; j < n; j++) {
					matrix[j][i] = *a1float++;
				}
			}
			det = determinant(matrix, n);
			if (det == 0.0) {
				dealloc2df(matrix, numcols, numrows);
				error((char *)"Determinant of array is zero");
			}
			MMFLOAT** matrix1 = alloc2df(n, n);
			cofactor(matrix, matrix1, n);
			for (i = 0; i < n; i++) { //load the matrix
				for (j = 0; j < n; j++) {
					*a2float++ = matrix1[j][i];
				}
			}
			dealloc2df(matrix, numcols, numrows);
			dealloc2df(matrix1, numcols, numrows);

			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"M_TRANSPOSE");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int i, j, numcols1 = 0, numrows1 = 0, numcols2 = 0, numrows2 = 0;
			MMFLOAT* a1float = NULL, * a2float = NULL;
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D floating point array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D floating point array");
				}
				numcols1 = numrows2 = vartbl[VarIndex].dims[0] - OptionBase;
				numrows1 = numcols2 = vartbl[VarIndex].dims[1] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 2D floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 2 must be 2D floating point array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be 2D floating point array");
				}
				if (numcols2 != (vartbl[VarIndex].dims[0] - OptionBase))error((char *)"Array size mismatch");
				if (numrows2 != (vartbl[VarIndex].dims[1] - OptionBase))error((char *)"Array size mismatch");
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a 2D floating point array");
			numcols1++;
			numrows1++;
			numcols2++;
			numrows2++;
			MMFLOAT** matrix1 = alloc2df(numcols1, numrows1);
			MMFLOAT** matrix2 = alloc2df(numrows2, numcols2);
			for (i = 0; i < numrows1; i++) {
				for (j = 0; j < numcols1; j++) {
					matrix1[j][i] = *a1float++;
				}
			}
			for (i = 0; i < numrows1; i++) {
				for (j = 0; j < numcols1; j++) {
					matrix2[i][j] = matrix1[j][i];
				}
			}
			for (i = 0; i < numrows2; i++) {
				for (j = 0; j < numcols2; j++) {
					*a2float++ = matrix2[j][i];
				}
			}
			dealloc2df(matrix1, numcols1, numrows1);
			dealloc2df(matrix2, numcols2, numrows2);
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"M_MULT");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			void* ptr3 = NULL;
			int i, j, k, numcols1 = 0, numrows1 = 0, numcols2 = 0, numrows2 = 0, numcols3 = 0, numrows3 = 0;
			MMFLOAT* a1float = NULL, * a2float = NULL, * a3float = NULL;
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D floating point array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D floating point array");
				}
				numcols1 = numrows2 = vartbl[VarIndex].dims[0] - OptionBase + 1;
				numrows1 = vartbl[VarIndex].dims[1] - OptionBase + 1;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 2D floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a 2D floating point array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a 2D floating point array");
				}
				numcols2 = vartbl[VarIndex].dims[0] - OptionBase + 1;
				numrows2 = vartbl[VarIndex].dims[1] - OptionBase + 1;
				if (numrows2 != numcols1)error((char *)"Input array size mismatch");
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a 2D floating point array");
			ptr3 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 3 must be a 2D floating point array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 3 must be a 2D floating point array");
				}
				numcols3 = vartbl[VarIndex].dims[0] - OptionBase + 1;
				numrows3 = vartbl[VarIndex].dims[1] - OptionBase + 1;
				if (numcols3 != numcols2 || numrows3 != numrows1)error((char *)"Output array size mismatch");
				a3float = (MMFLOAT*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be a 2D floating point array");
			if (ptr3 == ptr1 || ptr3 == ptr2)error((char *)"Destination array same as source");
			MMFLOAT** matrix1 = alloc2df(numcols1, numrows1);
			MMFLOAT** matrix2 = alloc2df(numrows2, numcols2);
			MMFLOAT** matrix3 = alloc2df(numrows3, numcols3);
			for (i = 0; i < numrows1; i++) { //load the first matrix
				for (j = 0; j < numcols1; j++) {
					matrix1[j][i] = *a1float++;
				}
			}
			for (i = 0; i < numrows2; i++) { //load the second matrix
				for (j = 0; j < numcols2; j++) {
					matrix2[j][i] = *a2float++;
				}
			}
			// Now calculate the dot products
			for (i = 0; i < numrows3; i++) {
				for (j = 0; j < numcols3; j++) {
					matrix3[j][i] = 0.0;
					for (k = 0; k < numcols1; k++) {
						matrix3[j][i] += matrix1[k][i] * matrix2[j][k];
					}
				}
			}

			for (i = 0; i < numrows3; i++) { //store the answer
				for (j = 0; j < numcols3; j++) {
					*a3float++ = matrix3[j][i];
				}
			}
			dealloc2df(matrix1, numcols1, numrows1);
			dealloc2df(matrix2, numcols2, numrows2);
			dealloc2df(matrix3, numcols3, numrows3);
			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"M_PRINT");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, numcols = 0, numrows = 0;
			MMFLOAT* a1float = NULL;
			int64_t* a1int = NULL;
			// need three arrays with same cardinality, second array must be 2 dimensional
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				numrows = vartbl[VarIndex].dims[1] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (ptr1 && vartbl[VarIndex].type & T_INT) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				numrows = vartbl[VarIndex].dims[1] - OptionBase;
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a numerical 2D array");
			numcols++;
			numrows++;
			MMFLOAT** matrix = alloc2df(numcols, numrows);
			int64_t** imatrix = (int64_t**)matrix;
			if (a1float != NULL) {
				for (i = 0; i < numrows; i++) {
					for (j = 0; j < numcols; j++) {
						matrix[j][i] = *a1float++;
					}
				}
				for (i = 0; i < numrows; i++) {
					PFlt(matrix[0][i]);
					for (j = 1; j < numcols; j++) {
						PFltComma(matrix[j][i]);
					}
					PRet();
				}
			}
			else {
				for (i = 0; i < numrows; i++) {
					for (j = 0; j < numcols; j++) {
						imatrix[j][i] = *a1int++;
					}
				}
				for (i = 0; i < numrows; i++) {
					PInt(imatrix[0][i]);
					for (j = 1; j < numcols; j++) {
						PIntComma(imatrix[j][i]);
					}
					PRet();
				}
			}
			dealloc2df(matrix, numcols, numrows);
			return;
		}
	}
	else if (toupper(*cmdline) == 'Q') {

		tp = checkstring(cmdline, (unsigned char *)"Q_INVERT");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int numcols = 0;
			MMFLOAT* q = NULL, * n = NULL;
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 1 must be a 5 element floating point array");
				q = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 5 element floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a 5 element floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				n = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a 5 element floating point array");
			Q_Invert(q, n);
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"Q_VECTOR");
		if (tp) {
			void* ptr1 = NULL;
			int numcols = 0;
			MMFLOAT* q = NULL;
			MMFLOAT mag = 0.0;
			getargs(&tp, 7, (unsigned char *)",");
			if (!(argc == 7)) error((char *)"Argument count");
			MMFLOAT x = getnumber(argv[0]);
			MMFLOAT y = getnumber(argv[2]);
			MMFLOAT z = getnumber(argv[4]);
			ptr1 = findvar(argv[6], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 4 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 4 must be a 5 element floating point array");
				q = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 4 must be a 5 element floating point array");
			mag = sqrt(x * x + y * y + z * z);//calculate the magnitude
			q[0] = 0.0; //create a normalised vector
			q[1] = x / mag;
			q[2] = y / mag;
			q[3] = z / mag;
			q[4] = mag;
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"Q_EULER");
		if (tp) {
			void* ptr1 = NULL;
			int numcols = 0;
			MMFLOAT* q = NULL;
			getargs(&tp, 7, (unsigned char *)",");
			if (!(argc == 7)) error((char *)"Argument count");
			MMFLOAT yaw = -getnumber(argv[0]) / optionangle;
			MMFLOAT pitch = getnumber(argv[2]) / optionangle;
			MMFLOAT roll = getnumber(argv[4]) / optionangle;
			ptr1 = findvar(argv[6], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 4 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 4 must be a 5 element floating point array");
				q = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 4 must be a 5 element floating point array");
			MMFLOAT s1 = sin(pitch / 2);
			MMFLOAT c1 = cos(pitch / 2);
			MMFLOAT s2 = sin(yaw / 2);
			MMFLOAT c2 = cos(yaw / 2);
			MMFLOAT s3 = sin(roll / 2);
			MMFLOAT c3 = cos(roll / 2);
			q[1] = s1 * c2 * c3 - c1 * s2 * s3;
			q[2] = c1 * s2 * c3 + s1 * c2 * s3;
			q[3] = c1 * c2 * s3 - s1 * s2 * c3;
			q[0] = c1 * c2 * c3 + s1 * s2 * s3;
			q[4] = 1.0;
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"Q_CREATE");
		if (tp) {
			void* ptr1 = NULL;
			int numcols = 0;
			MMFLOAT* q = NULL;
			MMFLOAT mag = 0.0;
			getargs(&tp, 9, (unsigned char *)",");
			if (!(argc == 9)) error((char *)"Argument count");
			MMFLOAT theta = getnumber(argv[0]);
			MMFLOAT x = getnumber(argv[2]);
			MMFLOAT y = getnumber(argv[4]);
			MMFLOAT z = getnumber(argv[6]);
			ptr1 = findvar(argv[8], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 1 must be a 5 element floating point array");
				q = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 5 element floating point array");
			MMFLOAT sineterm = sin(theta / 2.0 / optionangle);
			q[0] = cos(theta / 2.0);
			q[1] = x * sineterm;
			q[2] = y * sineterm;
			q[3] = z * sineterm;
			mag = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);//calculate the magnitude
			q[0] = q[0] / mag; //create a normalised quaternion
			q[1] = q[1] / mag;
			q[2] = q[2] / mag;
			q[3] = q[3] / mag;
			q[4] = 1.0;
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"Q_MULT");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			void* ptr3 = NULL;
			int numcols = 0;
			MMFLOAT* q1 = NULL, * q2 = NULL, * n = NULL;
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 1 must be a 5 element floating point array");
				q1 = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 5 element floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a 5 element floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				q2 = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a 5 element floating point array");
			ptr3 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 3 must be a 5 element floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				n = (MMFLOAT*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be a 5 element floating point array");
			numcols++;
			Q_Mult(q1, q2, n);
			return;
		}

		tp = checkstring(cmdline, (unsigned char *)"Q_ROTATE");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			void* ptr3 = NULL;
			int numcols = 0;
			MMFLOAT* q1 = NULL, * v1 = NULL, * n = NULL;
			MMFLOAT temp[5], qtemp[5];
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 1 must be a 5 element floating point array");
				q1 = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 5 element floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 2 must be a 5 element floating point array");
				v1 = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a 5 element floating point array");
			ptr3 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 3 must be a 5 element floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				if (numcols != 4)error((char *)"Argument 3 must be a 5 element floating point array");
				n = (MMFLOAT*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be a 5 element floating point array");
			numcols++;
			Q_Mult(q1, v1, temp);
			Q_Invert(q1, qtemp);
			Q_Mult(temp, qtemp, n);
			return;
		}
	}
	else {
		tp = checkstring(cmdline, (unsigned char *)"ADD");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int i, j, card1 = 1, card2 = 1;
			MMFLOAT* a1float = NULL, * a2float = NULL, scale;
			int64_t* a1int = NULL, * a2int = NULL;
			getargs(&tp, 5, (unsigned char *)",");
			if (!(argc == 5)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			evaluate(argv[2], &f, &i64, &s, &t, false);
			if (t & T_STR) error((char *)"Syntax");
			scale = getnumber(argv[2]);
			ptr2 = findvar(argv[4], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2int = (int64_t*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 3 must be numerical");
			if (card1 != card2)error((char *)"Size mismatch");
			if (scale != 0.0) {
				if (a2float != NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)*a2float++ = ((t & T_INT) ? (MMFLOAT)i64 : f) + (*a1float++);
				}
				else if (a2float != NULL && a1float == NULL) {
					for (i = 0; i < card1; i++)(*a2float++) = ((t & T_INT) ? (MMFLOAT)i64 : f) + ((MMFLOAT)*a1int++);
				}
				else if (a2float == NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)(*a2int++) = FloatToInt64(((t & T_INT) ? i64 : FloatToInt64(f)) + (*a1float++));
				}
				else {
					for (i = 0; i < card1; i++)(*a2int++) = ((t & T_INT) ? i64 : FloatToInt64(f)) + (*a1int++);
				}
			}
			else {
				if (a2float != NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)*a2float++ = *a1float++;
				}
				else if (a2float != NULL && a1float == NULL) {
					for (i = 0; i < card1; i++)(*a2float++) = ((MMFLOAT)*a1int++);
				}
				else if (a2float == NULL && a1float != NULL) {
					for (i = 0; i < card1; i++)(*a2int++) = FloatToInt64(*a1float++);
				}
				else {
					for (i = 0; i < card1; i++)*a2int++ = *a1int++;
				}
			}
			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"INTERPOLATE");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			void* ptr3 = NULL;
			int i, j, card1 = 1, card2 = 1, card3 = 1;
			MMFLOAT* a1float = NULL, * a2float = NULL, * a3float = NULL, scale, tmp1, tmp2, tmp3;
			int64_t* a1int = NULL, * a2int = NULL, * a3int = NULL;
			getargs(&tp, 7, (unsigned char *)",");
			if (!(argc == 7)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			evaluate(argv[4], &f, &i64, &s, &t, false);
			if (t & T_STR) error((char *)"Syntax");
			scale = getnumber(argv[4]);
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2int = (int64_t*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be numerical");
			ptr3 = findvar(argv[6], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card3 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card3 *= j;
				}
				a3float = (MMFLOAT*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card3 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card3 *= j;
				}
				a3int = (int64_t*)ptr3;
				if ((uint32_t)ptr3 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be numerical");
			if ((card1 != card2) || (card1 != card3))error((char *)"Size mismatch");
			if ((ptr1 == ptr2) || (ptr1 == ptr3))error((char *)"Arrays must be different");
			if (a3int != NULL) {
				for (i = 0; i < card1; i++) {
					if (a1int != NULL)tmp1 = (MMFLOAT)*a1int++;
					else tmp1 = *a1float++;
					if (a2int != NULL)tmp2 = (MMFLOAT)*a2int++;
					else tmp2 = *a2float++;
					tmp3 = (tmp2 - tmp1) * scale + tmp1;
					*a3int++ = FloatToInt64(tmp3);
				}
			}
			else {
				for (i = 0; i < card1; i++) {
					if (a1int != NULL)tmp1 = (MMFLOAT)*a1int++;
					else tmp1 = *a1float++;
					if (a2int != NULL)tmp2 = (MMFLOAT)*a2int++;
					else tmp2 = *a2float++;
					tmp3 = (tmp2 - tmp1) * scale + tmp1;
					*a3float++ = tmp3;
				}
			}
			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"INSERT");
		if (tp) {
			int i, j, start, increment, dim[MAXDIM], pos[MAXDIM], off[MAXDIM], dimcount = 0, target = -1, toarray = 0;
			int64_t* a1int = NULL, * a2int = NULL;
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			getargs(&tp, 13, (unsigned char *)",");
			if (argc < 7)error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & (T_NBR | T_INT)) {
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D or more numerical array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a 2D or more numerical array");
				}
				for (i = 0; i < MAXDIM; i++) {
					if (vartbl[VarIndex].dims[i] - OptionBase > 0) {
						dimcount++;
						dim[i] = vartbl[VarIndex].dims[i] - OptionBase;
					}
					else dim[i] = 0;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a 2D or more numerical array");
			if (((argc - 1) / 2 - 1) != dimcount)error((char *)"Argument count");
			for (i = 0; i < dimcount; i++) {
				if (*argv[i * 2 + 2]) pos[i] = (int)getint(argv[i * 2 + 2], OptionBase, dim[i] + OptionBase) - OptionBase;
				else {
					if (target != -1)error((char *)"Only one index can be omitted");
					target = i;
					pos[i] = 1;
				}
			}
			ptr2 = findvar(argv[i * 2 + 2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & (T_NBR | T_INT)) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Source must be 1D a numerical point array");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Source must be 1D a numerical point array");
				}
				toarray = vartbl[VarIndex].dims[0] - OptionBase;
				a2int = (int64_t*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Source must be 1D a numerical point array");
			if (dim[target] != toarray)error((char *)"Size mismatch between insert and target array");
			i = dimcount - 1;
			while (i >= 0) {
				off[i] = 1;
				for (j = 0; j < i; j++)off[i] *= (dim[j] + 1);
				i--;
			}
			start = 1;
			for (i = 0; i < dimcount; i++) {
				start += (pos[i] * off[i]);
			}
			start--;
			increment = off[target];
			start -= increment;
			for (i = 0; i <= dim[target]; i++) a1int[start + i * increment] = *a2int++;
			return;
		}
		tp = checkstring(cmdline, (unsigned char *)"FFT");
		if (tp) {
			cmd_FFT(tp);
			return;
		}
	}

	error((char *)"Syntax");
}

void fun_math(void) {
	unsigned char* tp, * tp1;
	skipspace(ep);
	if (toupper(*ep) == 'A') {
		tp = checkstring(ep, (unsigned char *)"ATAN3");
		if (tp) {
			MMFLOAT y, x, z;
			getargs(&tp, 3, (unsigned char *)",");
			if (argc != 3)error((char *)"Syntax");
			y = getnumber(argv[0]);
			x = getnumber(argv[2]);
			z = atan2(y, x);
			if (z < 0.0) z = z + 2.0 * PI_VALUE;
			fret = z * optionangle;
			targ = T_NBR;
			return;
		}
	}
	else if (toupper(*ep) == 'C') {

		tp = checkstring(ep, (unsigned char *)"COSH");
		if (tp) {
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			fret = cosh(getnumber(argv[0]));
			targ = T_NBR;
			return;
		}
		tp = checkstring(ep, (unsigned char *)"CORREL");
		if (tp) {
			void* ptr1 = NULL;
			void* ptr2 = NULL;
			int i, j, card1 = 1, card2 = 1;
			MMFLOAT* a1float = NULL, * a2float = NULL, mean1 = 0, mean2 = 0;
			MMFLOAT* a3float = NULL, * a4float = NULL;
			MMFLOAT axb = 0, a2 = 0, b2 = 0;
			int64_t* a1int = NULL, * a2int = NULL;
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2int = (int64_t*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be numerical");
			if (card1 != card2)error((char *)"Array size mismatch");
			a3float = (MMFLOAT*)GetTempMemory(card1 * sizeof(MMFLOAT));
			a4float = (MMFLOAT*)GetTempMemory(card1 * sizeof(MMFLOAT));
			if (a1float != NULL) {
				for (i = 0; i < card1; i++)a3float[i] = (*a1float++);
			}
			else {
				for (i = 0; i < card1; i++)a3float[i] = (MMFLOAT)(*a1int++);
			}
			if (a2float != NULL) {
				for (i = 0; i < card1; i++)a4float[i] = (*a2float++);
			}
			else {
				for (i = 0; i < card1; i++)a4float[i] = (MMFLOAT)(*a2int++);
			}
			for (i = 0; i < card1; i++) {
				mean1 += a3float[i];
				mean2 += a4float[i];
			}
			mean1 /= card1;
			mean2 /= card1;
			for (i = 0; i < card1; i++) {
				a3float[i] -= mean1;
				a2 += (a3float[i] * a3float[i]);
				a4float[i] -= mean2;
				b2 += (a4float[i] * a4float[i]);
				axb += (a3float[i] * a4float[i]);
			}
			targ = T_NBR;
			fret = axb / sqrt(a2 * b2);
			return;
		}
		tp = (checkstring(ep, (unsigned char *)"CHI_P"));
		tp1 = (checkstring(ep, (unsigned char *)"CHI"));
		if (tp || tp1) {
			void* ptr1 = NULL;
			int chi_p = 1;
			if (tp1) {
				tp = tp1;
				chi_p = 0;
			}
			int i, j, df, numcols = 0, numrows = 0;
			MMFLOAT* a1float = NULL, * rows = NULL, * cols = NULL, chi = 0, prob, chi_prob;
			MMFLOAT total = 0.0;
			int64_t* a1int = NULL;
			{
				getargs(&tp, 1, (unsigned char *)",");
				if (!(argc == 1)) error((char *)"Argument count");
				ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
				if (vartbl[VarIndex].type & T_NBR) {
					if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
					if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
						error((char *)"Argument 1 must be a numerical 2D array");
					}
					if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
						error((char *)"Argument 1 must be a numerical 2D array");
					}
					numcols = vartbl[VarIndex].dims[0] - OptionBase;
					numrows = vartbl[VarIndex].dims[1] - OptionBase;
					a1float = (MMFLOAT*)ptr1;
					if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
				}
				else if (ptr1 && vartbl[VarIndex].type & T_INT) {
					if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
					if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
						error((char *)"Argument 1 must be a numerical 2D array");
					}
					if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
						error((char *)"Argument 1 must be a numerical 2D array");
					}
					numcols = vartbl[VarIndex].dims[0] - OptionBase;
					numrows = vartbl[VarIndex].dims[1] - OptionBase;
					a1int = (int64_t*)ptr1;
					if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
				}
				else error((char *)"Argument 1 must be a numerical 2D array");
				df = numcols * numrows;
				numcols++;
				numrows++;
				MMFLOAT** observed = alloc2df(numcols, numrows);
				MMFLOAT** expected = alloc2df(numcols, numrows);
				rows = alloc1df(numrows);
				cols = alloc1df(numcols);
				if (a1float != NULL) {
					for (i = 0; i < numrows; i++) {
						for (j = 0; j < numcols; j++) {
							observed[j][i] = *a1float++;
							total += observed[j][i];
							rows[i] += observed[j][i];
						}
					}
				}
				else {
					for (i = 0; i < numrows; i++) {
						for (j = 0; j < numcols; j++) {
							observed[j][i] = (MMFLOAT)(*a1int++);
							total += observed[j][i];
							rows[i] += observed[j][i];
						}
					}
				}
				for (j = 0; j < numcols; j++) {
					for (i = 0; i < numrows; i++) {
						cols[j] += observed[j][i];
					}
				}
				for (i = 0; i < numrows; i++) {
					for (j = 0; j < numcols; j++) {
						expected[j][i] = cols[j] * rows[i] / total;
						expected[j][i] = ((observed[j][i] - expected[j][i]) * (observed[j][i] - expected[j][i]) / expected[j][i]);
						chi += expected[j][i];
					}
				}
				prob = chitable[df][7];
				if (chi > prob) {
					i = 7;
					while (i < 15 && chi >= chitable[df][i])i++;
					chi_prob = chitable[0][i - 1];
				}
				else {
					i = 7;
					while (i >= 0 && chi <= chitable[df][i])i--;
					chi_prob = chitable[0][i + 1];
				}
				dealloc2df(observed, numcols, numrows);
				dealloc2df(expected, numcols, numrows);
				FreeMemory((unsigned char*)rows);
				FreeMemory((unsigned char*)cols);
				rows = NULL;
				cols = NULL;
				targ = T_NBR;
				fret = (chi_p ? chi_prob * 100 : chi);
				return;
			}
		}

	}
	else if (toupper(*ep) == 'D') {

		tp = checkstring(ep, (unsigned char *)"DOTPRODUCT");
		if (tp) {
			int i;
			void* ptr1 = NULL, * ptr2 = NULL;
			int numcols = 0;
			MMFLOAT* a1float = NULL, * a2float = NULL;
			// need two arrays with same cardinality
			getargs(&tp, 3, (unsigned char *)",");
			if (!(argc == 3)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a floating point array");
			ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 2 must be a floating point array");
				}
				if ((vartbl[VarIndex].dims[0] - OptionBase) != numcols)error((char *)"Array size mismatch");
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 2 must be a floating point array");
			numcols++;
			fret = 0;
			for (i = 0; i < numcols; i++) {
				fret = fret + ((*a1float++) * (*a2float++));
			}
			targ = T_NBR;
			return;
		}
	}
	else if (toupper(*ep) == 'L') {
		tp = checkstring(ep, (unsigned char *)"LOG10");
		if (tp) {
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			fret = log10(getnumber(argv[0]));
			targ = T_NBR;
			return;
		}

	}
	else if (toupper(*ep) == 'M') {
		tp = checkstring(ep, (unsigned char *)"M_DETERMINANT");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, n, numcols = 0, numrows = 0;
			MMFLOAT* a1float = NULL;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[1] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a numerical 2D array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				numrows = vartbl[VarIndex].dims[1] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else	error((char *)"Argument 1 must be a numerical 2D array");
			if (numcols != numrows)error((char *)"Array must be square");
			n = numrows + 1;
			MMFLOAT** matrix = alloc2df(n, n);
			for (i = 0; i < n; i++) { //load the matrix
				for (j = 0; j < n; j++) {
					matrix[j][i] = *a1float++;
				}
			}
			fret = determinant(matrix, n);
			dealloc2df(matrix, numcols, numrows);
			targ = T_NBR;

			return;
		}


		tp = checkstring(ep, (unsigned char *)"MAX");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, card1 = 1;
			MMFLOAT* a1float = NULL, max = -3.0e+38;
			int64_t* a1int = NULL;
			long long int* temp = NULL;
			getargs(&tp, 3, (unsigned char *)",");
			//			if(!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			if (argc == 3) {
				if (vartbl[VarIndex].dims[1] > 0) {		// Not an array
					error((char *)"Argument 1 must be a 1D numerical array");
				}
				temp = (long long int *) findvar(argv[2], V_FIND);
				if (!(vartbl[VarIndex].type & T_INT)) error((char *)"Invalid variable");
			}

			if (a1float != NULL) {
				for (i = 0; i < card1; i++) {
					if ((*a1float) > max) {
						max = (*a1float);
						if (temp != NULL) {
							*temp = i + OptionBase;
						}
					}
					a1float++;
				}
			}
			else {
				for (i = 0; i < card1; i++) {
					if (((MMFLOAT)(*a1int)) > max) {
						max = (MMFLOAT)(*a1int);
						if (temp != NULL) {
							*temp = i + OptionBase;
						}
					}
					a1int++;
				}
			}
			targ = T_NBR;
			fret = max;
			return;
		}
		tp = checkstring(ep, (unsigned char *)"MIN");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, card1 = 1;
			MMFLOAT* a1float = NULL, min = 3.0e+38;
			int64_t* a1int = NULL;
			long long int* temp = NULL;
			getargs(&tp, 3, (unsigned char *)",");
			//			if(!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			if (argc == 3) {
				if (vartbl[VarIndex].dims[1] > 0) {		// Not an array
					error((char *)"Argument 1 must be a 1D numerical array");
				}
				temp = (long long int*)findvar(argv[2], V_FIND);
				if (!(vartbl[VarIndex].type & T_INT)) error((char *)"Invalid variable");
			}
			if (a1float != NULL) {
				for (i = 0; i < card1; i++) {
					if ((*a1float) < min) {
						min = (*a1float);
						if (temp != NULL) {
							*temp = i + OptionBase;
						}
					}
					a1float++;
				}
			}
			else {
				for (i = 0; i < card1; i++) {
					if (((MMFLOAT)(*a1int)) < min) {
						min = (MMFLOAT)(*a1int);
						if (temp != NULL) {
							*temp = i + OptionBase;
						}
					}
					a1int++;
				}
			}
			targ = T_NBR;
			fret = min;
			return;
		}
		tp = checkstring(ep, (unsigned char *)"MAGNITUDE");
		if (tp) {
			int i;
			void* ptr1 = NULL;
			int numcols = 0;
			MMFLOAT* a1float = NULL;
			MMFLOAT mag = 0.0;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
				if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
					error((char *)"Argument 1 must be a floating point array");
				}
				numcols = vartbl[VarIndex].dims[0] - OptionBase;
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be a floating point array");
			numcols++;
			for (i = 0; i < numcols; i++) {
				mag = mag + ((*a1float) * (*a1float));
				a1float++;
			}
			fret = sqrt(mag);
			targ = T_NBR;
			return;
		}

		tp = checkstring(ep, (unsigned char *)"MEAN");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, card1 = 1;
			MMFLOAT* a1float = NULL, mean = 0;
			int64_t* a1int = NULL;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			if (a1float != NULL) {
				for (i = 0; i < card1; i++)mean += (*a1float++);
			}
			else {
				for (i = 0; i < card1; i++)mean += (MMFLOAT)(*a1int++);
			}
			targ = T_NBR;
			fret = mean / (MMFLOAT)card1;
			return;
		}

		tp = checkstring(ep, (unsigned char *)"MEDIAN");
		if (tp) {
			void* ptr2 = NULL;
			int i, j, card1, card2 = 1;
			MMFLOAT* a1float = NULL, * a2float = NULL;
			int64_t* a2int = NULL;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr2 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2float = (MMFLOAT*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card2 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card2 *= j;
				}
				a2int = (int64_t*)ptr2;
				if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			card1 = card2;
			card2 = (card2 - 1) / 2;
			a1float = (MMFLOAT*)GetTempMemory(card1 * sizeof(MMFLOAT));
			if (a2float != NULL) {
				for (i = 0; i < card1; i++)a1float[i] = (*a2float++);
			}
			else {
				for (i = 0; i < card1; i++)a1float[i] = (MMFLOAT)(*a2int++);
			}
			floatshellsort(a1float, card1);
			targ = T_NBR;
			if (card1 & 1)fret = a1float[card2];
			else fret = (a1float[card2] + a1float[card2 + 1]) / 2.0;
			return;
		}
	}
	else if (toupper(*ep) == 'S') {

		tp = checkstring(ep, (unsigned char *)"SINH");
		if (tp) {
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			fret = sinh(getnumber(argv[0]));
			targ = T_NBR;
			return;
		}

		tp = checkstring(ep, (unsigned char *)"SD");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, card1 = 1;
			MMFLOAT* a2float = NULL, * a1float = NULL, mean = 0, var = 0, deviation;
			int64_t* a2int = NULL, * a1int = NULL;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = a2float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = a2int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			if (a2float != NULL) {
				for (i = 0; i < card1; i++)mean += (*a2float++);
			}
			else {
				for (i = 0; i < card1; i++)mean += (MMFLOAT)(*a2int++);
			}
			mean = mean / (MMFLOAT)card1;
			if (a1float != NULL) {
				for (i = 0; i < card1; i++) {
					deviation = (*a1float++) - mean;
					var += deviation * deviation;
				}
			}
			else {
				for (i = 0; i < card1; i++) {
					deviation = (MMFLOAT)(*a1int++) - mean;
					var += deviation * deviation;
				}
			}
			targ = T_NBR;
			fret = sqrt(var / card1);
			return;
		}

		tp = checkstring(ep, (unsigned char *)"SUM");
		if (tp) {
			void* ptr1 = NULL;
			int i, j, card1 = 1;
			MMFLOAT* a1float = NULL, sum = 0;
			int64_t* a1int = NULL;
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
			if (vartbl[VarIndex].type & T_NBR) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1float = (MMFLOAT*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else if (vartbl[VarIndex].type & T_INT) {
				card1 = 1;
				for (i = 0; i < MAXDIM; i++) {
					j = (vartbl[VarIndex].dims[i] - OptionBase + 1);
					if (j)card1 *= j;
				}
				a1int = (int64_t*)ptr1;
				if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
			}
			else error((char *)"Argument 1 must be numerical");
			if (a1float != NULL) {
				for (i = 0; i < card1; i++)sum += (*a1float++);
			}
			else {
				for (i = 0; i < card1; i++)sum += (MMFLOAT)(*a1int++);
			}
			targ = T_NBR;
			fret = sum;
			return;
		}
	}
	else if (toupper(*ep) == 'T') {

		tp = checkstring(ep, (unsigned char *)"TANH");
		if (tp) {
			getargs(&tp, 1, (unsigned char *)",");
			if (!(argc == 1)) error((char *)"Argument count");
			fret = tanh(getnumber(argv[0]));
			targ = T_NBR;
			return;
		}
	}
	tp = checkstring(ep, (unsigned char*)"UPSAMPLE");
	if (tp) {
		void* ptr1 = NULL;
		void* ptr2 = NULL;
		int insize = 0, outsize = 0;
		MMFLOAT* q1 = NULL, * v1 = NULL, * n = NULL;
		getargs(&tp, 7, (unsigned char*)",");
		if (!(argc == 7)) error((char*)"Argument count");
		ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char*)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char*)"Argument 1 must be a floating point array");
			}
			insize = vartbl[VarIndex].dims[0] - OptionBase;
			q1 = (MMFLOAT*)ptr1;
			if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char*)"Argument 1 must be a floating point array");
		}

		else error((char*)"Argument 1 must be a floating point array");
		ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char*)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char*)"Argument 2 must be a floating point array");
			}
			outsize = vartbl[VarIndex].dims[0] - OptionBase;
			v1 = (MMFLOAT*)ptr2;
			if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char*)"Argument 2 must be a floating point array");
		}
		else error((char*)"Argument 2 must be a floating point array");
		MMFLOAT infreq = getnumber(argv[4]);
		if (infreq < 0)error((char*)"Invalid input frequency");
		MMFLOAT outfreq = getnumber(argv[6]);
		if (outfreq < infreq)error((char *)"Invalid output frequency");
		n = (MMFLOAT*)GetTempMemory((int)(outfreq / infreq * (MMFLOAT)insize + 10) * (int)sizeof(MMFLOAT));
		fret = upsample(q1, n, insize, outfreq, infreq);
		if (fret > outsize)error((char *)"Output array size");
		for (int i = 0; i < (int)fret; i++)*v1++ = *n++;
		return;
	}
	error((char *)"Syntax");
}
typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;
// Cooley-Tukey FFT (in-place, breadth-first, decimation-in-frequency)
// Better optimized but less intuitive
// !!! Warning : in some cases this code make result different from not optimased version above (need to fix bug)
// The bug is now fixed @2017/05/30 
void fft(CArray& x)
{
	// DFT
	unsigned int N = x.size(), k = N, n;
	double thetaT = 3.14159265358979323846264338328L / N;
	Complex phiT = Complex(cos(thetaT), -sin(thetaT)), T;
	while (k > 1)
	{
		n = k;
		k >>= 1;
		phiT = phiT * phiT;
		T = 1.0L;
		for (unsigned int l = 0; l < k; l++)
		{
			for (unsigned int a = l; a < N; a += n)
			{
				unsigned int b = a + k;
				Complex t = x[a] - x[b];
				x[a] += x[b];
				x[b] = t * T;
			}
			T *= phiT;
		}
	}
	// Decimate
	unsigned int m = (unsigned int)log2(N);
	for (unsigned int a = 0; a < N; a++)
	{
		unsigned int b = a;
		// Reverse bits
		b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
		b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
		b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
		b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
		b = ((b >> 16) | (b << 16)) >> (32 - m);
		if (b > a)
		{
			Complex t = x[a];
			x[a] = x[b];
			x[b] = t;
		}
	}
	//// Normalize (This section make it not working correctly)
	//Complex f = 1.0 / sqrt(N);
	//for (unsigned int i = 0; i < N; i++)
	//	x[i] *= f;
}

void ifft(CArray& x)
{
	// conjugate the complex numbers
	x = x.apply(std::conj);

	// forward fft
	fft(x);

	// conjugate the complex numbers again
	x = x.apply(std::conj);

	// scale the numbers
	x /= x.size();
}
void cmd_FFT(unsigned char* pp) {
	void* ptr1 = NULL;
	void* ptr2 = NULL;
	unsigned char* tp;
	PI = atan2(1, 1) * 4;
	cplx* a1cplx = NULL, * a2cplx = NULL;
	MMFLOAT* a3float = NULL, * a4float = NULL;
	int i, size, powerof2 = 0;
	tp = checkstring(pp, (unsigned char*)"MAGNITUDE");
	if (tp) {
		getargs(&tp, 3, (unsigned char *)",");
		ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char *)"Argument 1 must be a floating point array");
			}
			a3float = (MMFLOAT*)ptr1;
			if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
		}
		else error((char *)"Argument 1 must be a floating point array");
		size = (vartbl[VarIndex].dims[0] - OptionBase);
		ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char *)"Argument 2 must be a floating point array");
			}
			a4float = (MMFLOAT*)ptr2;
			if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
		}
		else error((char *)"Argument 2 must be a floating point array");
		if ((vartbl[VarIndex].dims[0] - OptionBase) != size)error((char *)"Array size mismatch");
		for (i = 1; i < 65536; i *= 2) {
			if (size == i - 1)powerof2 = 1;
		}
		if (!powerof2)error((char *)"array size must be a power of 2");
		a1cplx = (cplx*)GetTempMemory((size + 1) * 16);
		for (i = 0; i <= size; i++) { 
			a1cplx[i] = (cplx)(a3float[i]);
		}
		CArray data(a1cplx, size+1);
		fft(data);
		for (i = 0; i <= size; i++)a4float[i] = std::abs(data[i]);
		return;
	}
	tp = checkstring(pp, (unsigned char*)"PHASE");
	if (tp) {
		getargs(&tp, 3, (unsigned char *)",");
		ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char *)"Argument 1 must be a floating point array");
			}
			a3float = (MMFLOAT*)ptr1;
			if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
		}
		else error((char *)"Argument 1 must be a floating point array");
		size = (vartbl[VarIndex].dims[0] - OptionBase);
		ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char *)"Argument 2 must be a floating point array");
			}
			a4float = (MMFLOAT*)ptr2;
			if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
		}
		else error((char *)"Argument 2 must be a floating point array");
		if ((vartbl[VarIndex].dims[0] - OptionBase) != size)error((char *)"Array size mismatch");
		for (i = 1; i < 65536; i *= 2) {
			if (size == i - 1)powerof2 = 1;
		}
		if (!powerof2)error((char *)"array size must be a power of 2");
		a1cplx = (cplx*)GetTempMemory((size + 1) * 16);
		for (i = 0; i <= size; i++) {
			a1cplx[i] = (cplx)(a3float[i]);
		}
		CArray data(a1cplx, size + 1);
		fft(data);
		for (i = 0; i <= size; i++)a4float[i] = std::arg(data[i]);
		return;
	}
	tp = checkstring(pp, (unsigned char*)"INVERSE");
	if (tp) {
		getargs(&tp, 3, (unsigned char *)",");
		ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] <= 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[0] - OptionBase != 1) {		// Not an array
				error((char *)"Argument 1 must be a 2D floating point array");
			}
			a1cplx = (cplx*)ptr1;
			if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
		}
		else error((char *)"Argument 1 must be a 2D floating point array");
		size = (vartbl[VarIndex].dims[1] - OptionBase);
		ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
		if (vartbl[VarIndex].type & T_NBR) {
			if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
			if (vartbl[VarIndex].dims[0] <= 0) {		// Not an array
				error((char *)"Argument 2 must be a floating point array");
			}
			a4float = (MMFLOAT*)ptr2;
			if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
		}
		else error((char *)"Argument 2 must be a floating point array");
		if ((vartbl[VarIndex].dims[0] - OptionBase) != size)error((char *)"Array size mismatch");
		for (i = 1; i < 65536; i *= 2) {
			if (size == i - 1)powerof2 = 1;
		}
		if (!powerof2)error((char *)"array size must be a power of 2");
		CArray data(a1cplx, size + 1);
		ifft(data);
		for (i = 0; i <= size; i++)a4float[i] = std::real(data[i]);
		return;
	}
	getargs(&pp, 3, (unsigned char *)",");
	ptr1 = findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	if (vartbl[VarIndex].type & T_NBR) {
		if (vartbl[VarIndex].dims[1] != 0) error((char *)"Invalid variable");
		if (vartbl[VarIndex].dims[0] <= 1) {		// Not an array
			error((char *)"Argument 1 must be a floating point array");
		}
		a3float = (MMFLOAT*)ptr1;
		if ((uint32_t)ptr1 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
	}
	else error((char *)"Argument 1 must be a floating point array");
	size = (vartbl[VarIndex].dims[0] - OptionBase);
	ptr2 = findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	if (vartbl[VarIndex].type & T_NBR) {
		if (vartbl[VarIndex].dims[1] <= 0) error((char *)"Invalid variable");
		if (vartbl[VarIndex].dims[2] != 0) error((char *)"Invalid variable");
		if (vartbl[VarIndex].dims[0] - OptionBase != 1) {		// Not an array
			error((char *)"Argument 2 must be a 2D floating point array");
		}
		a4float = (MMFLOAT*)ptr2;
		if ((uint32_t)ptr2 != (uint32_t)vartbl[VarIndex].val.s)error((char *)"Syntax");
	}
	else error((char *)"Argument 2 must be a 2D floating point array");
	if ((vartbl[VarIndex].dims[1] - OptionBase) != size)error((char *)"Array size mismatch");
	for (i = 1; i < 65536; i *= 2) {
		if (size == i - 1)powerof2 = 1;
	}
	if (!powerof2)error((char *)"array size must be a power of 2");
	a1cplx = (cplx*)GetTempMemory((size + 1) * 16);
	for (i = 0; i <= size; i++) {
		a1cplx[i] = (cplx)(a3float[i]);
	}
	CArray data(a1cplx, size + 1);
	fft(data);
	for (i = 0; i <= size; i++) { a4float[i * 2] = real(data[i]); a4float[i * 2 + 1] = imag(data[i]); }
}
void cmd_SensorFusion(char* passcmdline) {
	unsigned char* p;
	if ((p = checkstring((unsigned char*)passcmdline, (unsigned char *)"MADGWICK")) != NULL) {
		getargs(&p, 25, (unsigned char *)",");
		if (argc < 23) error((char *)"Incorrect number of parameters");
		MMFLOAT t;
		MMFLOAT* pitch, * yaw, * roll;
		MMFLOAT ax; MMFLOAT ay; MMFLOAT az; MMFLOAT gx; MMFLOAT gy; MMFLOAT gz; MMFLOAT mx; MMFLOAT my; MMFLOAT mz; MMFLOAT beta;
		ax = getnumber(argv[0]);
		ay = getnumber(argv[2]);
		az = getnumber(argv[4]);
		gx = getnumber(argv[6]);
		gy = getnumber(argv[8]);
		gz = getnumber(argv[10]);
		mx = getnumber(argv[12]);
		my = getnumber(argv[14]);
		mz = getnumber(argv[16]);
		pitch = (MMFLOAT *)findvar(argv[18], V_FIND);
		if (!(vartbl[VarIndex].type & T_NBR)) error((char *)"Invalid variable");
		roll = (MMFLOAT *)findvar(argv[20], V_FIND);
		if (!(vartbl[VarIndex].type & T_NBR)) error((char *)"Invalid variable");
		yaw = (MMFLOAT *)findvar(argv[22], V_FIND);
		if (!(vartbl[VarIndex].type & T_NBR)) error((char *)"Invalid variable");
		beta = 0.5;
		if (argc == 25) beta = getnumber(argv[24]);
		t = (MMFLOAT)AHRSTimer / 1000.0;
		if (t > 1.0)t = 1.0;
		AHRSTimer = 0;
		MadgwickQuaternionUpdate(ax, ay, az, gx, gy, gz, mx, my, mz, beta, t, pitch, yaw, roll);
		return;
	}
	if ((p = checkstring((unsigned char*)passcmdline, (unsigned char *)"MAHONY")) != NULL) {
		getargs(&p, 27, (unsigned char *)",");
		if (argc < 23) error((char *)"Incorrect number of parameters");
		MMFLOAT t;
		MMFLOAT* pitch, * yaw, * roll;
		MMFLOAT Kp, Ki;
		MMFLOAT ax; MMFLOAT ay; MMFLOAT az; MMFLOAT gx; MMFLOAT gy; MMFLOAT gz; MMFLOAT mx; MMFLOAT my; MMFLOAT mz;
		ax = getnumber(argv[0]);
		ay = getnumber(argv[2]);
		az = getnumber(argv[4]);
		gx = getnumber(argv[6]);
		gy = getnumber(argv[8]);
		gz = getnumber(argv[10]);
		mx = getnumber(argv[12]);
		my = getnumber(argv[14]);
		mz = getnumber(argv[16]);
		pitch = (MMFLOAT *)findvar(argv[18], V_FIND);
		if (!(vartbl[VarIndex].type & T_NBR)) error((char *)"Invalid variable");
		roll = (MMFLOAT *)findvar(argv[20], V_FIND);
		if (!(vartbl[VarIndex].type & T_NBR)) error((char *)"Invalid variable");
		yaw = (MMFLOAT *)findvar(argv[22], V_FIND);
		if (!(vartbl[VarIndex].type & T_NBR)) error((char *)"Invalid variable");
		Kp = 10.0; Ki = 0.0;
		if (argc >= 25)Kp = getnumber(argv[24]);
		if (argc == 27)Ki = getnumber(argv[26]);
		t = (MMFLOAT)AHRSTimer / 1000.0;
		if (t > 1.0)t = 1.0;
		AHRSTimer = 0;
		MahonyQuaternionUpdate(ax, ay, az, gx, gy, gz, mx, my, mz, Ki, Kp, t, yaw, pitch, roll);
		return;
	}
	error((char *)"Invalid command");
}
void MadgwickQuaternionUpdate(MMFLOAT ax, MMFLOAT ay, MMFLOAT az, MMFLOAT gx, MMFLOAT gy, MMFLOAT gz, MMFLOAT mx, MMFLOAT my, MMFLOAT mz, MMFLOAT beta, MMFLOAT deltat, MMFLOAT* pitch, MMFLOAT* yaw, MMFLOAT* roll)
{
	MMFLOAT q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
	MMFLOAT norm;
	MMFLOAT hx, hy, _2bx, _2bz;
	MMFLOAT s1, s2, s3, s4;
	MMFLOAT qDot1, qDot2, qDot3, qDot4;

	// Auxiliary variables to avoid repeated arithmetic
	MMFLOAT _2q1mx;
	MMFLOAT _2q1my;
	MMFLOAT _2q1mz;
	MMFLOAT _2q2mx;
	MMFLOAT _4bx;
	MMFLOAT _4bz;
	MMFLOAT _2q1 = 2.0 * q1;
	MMFLOAT _2q2 = 2.0 * q2;
	MMFLOAT _2q3 = 2.0 * q3;
	MMFLOAT _2q4 = 2.0 * q4;
	MMFLOAT _2q1q3 = 2.0 * q1 * q3;
	MMFLOAT _2q3q4 = 2.0 * q3 * q4;
	MMFLOAT q1q1 = q1 * q1;
	MMFLOAT q1q2 = q1 * q2;
	MMFLOAT q1q3 = q1 * q3;
	MMFLOAT q1q4 = q1 * q4;
	MMFLOAT q2q2 = q2 * q2;
	MMFLOAT q2q3 = q2 * q3;
	MMFLOAT q2q4 = q2 * q4;
	MMFLOAT q3q3 = q3 * q3;
	MMFLOAT q3q4 = q3 * q4;
	MMFLOAT q4q4 = q4 * q4;

	// Normalise accelerometer measurement
	norm = sqrt(ax * ax + ay * ay + az * az);
	if (norm == 0.0) return; // handle NaN
	norm = 1.0 / norm;
	ax *= norm;
	ay *= norm;
	az *= norm;

	// Normalise magnetometer measurement
	norm = sqrt(mx * mx + my * my + mz * mz);
	if (norm == 0.0) return; // handle NaN
	norm = 1.0 / norm;
	mx *= norm;
	my *= norm;
	mz *= norm;

	// Reference direction of Earth's magnetic field
	_2q1mx = 2.0 * q1 * mx;
	_2q1my = 2.0 * q1 * my;
	_2q1mz = 2.0 * q1 * mz;
	_2q2mx = 2.0 * q2 * mx;
	hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
	hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
	_2bx = sqrt(hx * hx + hy * hy);
	_2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
	_4bx = 2.0 * _2bx;
	_4bz = 2.0 * _2bz;

	// Gradient decent algorithm corrective step
	s1 = -_2q3 * (2.0 * q2q4 - _2q1q3 - ax) + _2q2 * (2.0 * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5 - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5 - q2q2 - q3q3) - mz);
	s2 = _2q4 * (2.0 * q2q4 - _2q1q3 - ax) + _2q1 * (2.0 * q1q2 + _2q3q4 - ay) - 4.0 * q2 * (1.0 - 2.0 * q2q2 - 2.0 * q3q3 - az) + _2bz * q4 * (_2bx * (0.5 - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5 - q2q2 - q3q3) - mz);
	s3 = -_2q1 * (2.0 * q2q4 - _2q1q3 - ax) + _2q4 * (2.0 * q1q2 + _2q3q4 - ay) - 4.0 * q3 * (1.0 - 2.0 * q2q2 - 2.0 * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5 - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5 - q2q2 - q3q3) - mz);
	s4 = _2q2 * (2.0 * q2q4 - _2q1q3 - ax) + _2q3 * (2.0 * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5 - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5 - q2q2 - q3q3) - mz);
	norm = sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);    // normalise step magnitude
	norm = 1.0 / norm;
	s1 *= norm;
	s2 *= norm;
	s3 *= norm;
	s4 *= norm;

	// Compute rate of change of quaternion
	qDot1 = 0.5 * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
	qDot2 = 0.5 * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
	qDot3 = 0.5 * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
	qDot4 = 0.5 * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

	// Integrate to yield quaternion
	q1 += qDot1 * deltat;
	q2 += qDot2 * deltat;
	q3 += qDot3 * deltat;
	q4 += qDot4 * deltat;
	norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);    // normalise quaternion
	norm = 1.0 / norm;
	q[0] = q1 * norm;
	q[1] = q2 * norm;
	q[2] = q3 * norm;
	q[3] = q4 * norm;

	MMFLOAT ysqr = q3 * q3;


	// roll (x-axis rotation)
	MMFLOAT t0 = +2.0 * (q1 * q2 + q3 * q4);
	MMFLOAT t1 = +1.0 - 2.0 * (q2 * q2 + ysqr);
	*roll = atan2(t0, t1);

	// pitch (y-axis rotation)
	MMFLOAT t2 = +2.0 * (q1 * q3 - q4 * q2);
	t2 = t2 > 1.0 ? 1.0 : t2;
	t2 = t2 < -1.0 ? -1.0 : t2;
	*pitch = asin(t2);

	// yaw (z-axis rotation)
	MMFLOAT t3 = +2.0 * (q1 * q4 + q2 * q3);
	MMFLOAT t4 = +1.0 - 2.0 * (ysqr + q4 * q4);
	*yaw = atan2(t3, t4);

}
void MahonyQuaternionUpdate(MMFLOAT ax, MMFLOAT ay, MMFLOAT az, MMFLOAT gx, MMFLOAT gy, MMFLOAT gz, MMFLOAT mx, MMFLOAT my, MMFLOAT mz, MMFLOAT Ki, MMFLOAT Kp, MMFLOAT deltat, MMFLOAT* yaw, MMFLOAT* pitch, MMFLOAT* roll) {
	MMFLOAT q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
	MMFLOAT norm;
	MMFLOAT hx, hy, bx, bz;
	MMFLOAT vx, vy, vz, wx, wy, wz;
	MMFLOAT ex, ey, ez;
	MMFLOAT pa, pb, pc;

	// Auxiliary variables to avoid repeated arithmetic
	MMFLOAT q1q1 = q1 * q1;
	MMFLOAT q1q2 = q1 * q2;
	MMFLOAT q1q3 = q1 * q3;
	MMFLOAT q1q4 = q1 * q4;
	MMFLOAT q2q2 = q2 * q2;
	MMFLOAT q2q3 = q2 * q3;
	MMFLOAT q2q4 = q2 * q4;
	MMFLOAT q3q3 = q3 * q3;
	MMFLOAT q3q4 = q3 * q4;
	MMFLOAT q4q4 = q4 * q4;

	// Normalise accelerometer measurement
	norm = sqrt(ax * ax + ay * ay + az * az);
	if (norm == 0.0) return; // handle NaN
	norm = 1.0 / norm;        // use reciprocal for division
	ax *= norm;
	ay *= norm;
	az *= norm;

	// Normalise magnetometer measurement
	norm = sqrt(mx * mx + my * my + mz * mz);
	if (norm == 0.0) return; // handle NaN
	norm = 1.0 / norm;        // use reciprocal for division
	mx *= norm;
	my *= norm;
	mz *= norm;

	// Reference direction of Earth's magnetic field
	hx = 2.0 * mx * (0.5 - q3q3 - q4q4) + 2.0 * my * (q2q3 - q1q4) + 2.0 * mz * (q2q4 + q1q3);
	hy = 2.0 * mx * (q2q3 + q1q4) + 2.0 * my * (0.5 - q2q2 - q4q4) + 2.0 * mz * (q3q4 - q1q2);
	bx = sqrt((hx * hx) + (hy * hy));
	bz = 2.0 * mx * (q2q4 - q1q3) + 2.0 * my * (q3q4 + q1q2) + 2.0 * mz * (0.5 - q2q2 - q3q3);

	// Estimated direction of gravity and magnetic field
	vx = 2.0 * (q2q4 - q1q3);
	vy = 2.0 * (q1q2 + q3q4);
	vz = q1q1 - q2q2 - q3q3 + q4q4;
	wx = 2.0 * bx * (0.5 - q3q3 - q4q4) + 2.0 * bz * (q2q4 - q1q3);
	wy = 2.0 * bx * (q2q3 - q1q4) + 2.0 * bz * (q1q2 + q3q4);
	wz = 2.0 * bx * (q1q3 + q2q4) + 2.0 * bz * (0.5 - q2q2 - q3q3);

	// Error is cross product between estimated direction and measured direction of gravity
	ex = (ay * vz - az * vy) + (my * wz - mz * wy);
	ey = (az * vx - ax * vz) + (mz * wx - mx * wz);
	ez = (ax * vy - ay * vx) + (mx * wy - my * wx);
	if (Ki > 0.0)
	{
		eInt[0] += ex;      // accumulate integral error
		eInt[1] += ey;
		eInt[2] += ez;
	}
	else
	{
		eInt[0] = 0.0;     // prevent integral wind up
		eInt[1] = 0.0;
		eInt[2] = 0.0;
	}

	// Apply feedback terms
	gx = gx + Kp * ex + Ki * eInt[0];
	gy = gy + Kp * ey + Ki * eInt[1];
	gz = gz + Kp * ez + Ki * eInt[2];

	// Integrate rate of change of quaternion
	pa = q2;
	pb = q3;
	pc = q4;
	q1 = q1 + (-q2 * gx - q3 * gy - q4 * gz) * (0.5 * deltat);
	q2 = pa + (q1 * gx + pb * gz - pc * gy) * (0.5 * deltat);
	q3 = pb + (q1 * gy - pa * gz + pc * gx) * (0.5 * deltat);
	q4 = pc + (q1 * gz + pa * gy - pb * gx) * (0.5 * deltat);

	// Normalise quaternion
	norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
	norm = 1.0 / norm;
	q[0] = q1 * norm;
	q[1] = q2 * norm;
	q[2] = q3 * norm;
	q[3] = q4 * norm;
	MMFLOAT ysqr = q3 * q3;


	// roll (x-axis rotation)
	MMFLOAT t0 = +2.0 * (q1 * q2 + q3 * q4);
	MMFLOAT t1 = +1.0 - 2.0 * (q2 * q2 + ysqr);
	*roll = atan2(t0, t1);

	// pitch (y-axis rotation)
	MMFLOAT t2 = +2.0 * (q1 * q3 - q4 * q2);
	t2 = t2 > 1.0 ? 1.0 : t2;
	t2 = t2 < -1.0 ? -1.0 : t2;
	*pitch = asin(t2);

	// yaw (z-axis rotation)
	MMFLOAT t3 = +2.0 * (q1 * q4 + q2 * q3);
	MMFLOAT t4 = +1.0 - 2.0 * (ysqr + q4 * q4);
	*yaw = atan2(t3, t4);
}

/*Finding transpose of cofactor of matrix*/
void transpose(MMFLOAT** matrix, MMFLOAT** matrix_cofactor, MMFLOAT** m_inverse, int size)
{
	int i, j;
	MMFLOAT d;
	MMFLOAT** m_transpose = alloc2df(size, size);

	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			m_transpose[i][j] = matrix_cofactor[j][i];
		}
	}
	d = determinant(matrix, size);
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			m_inverse[i][j] = m_transpose[i][j] / d;
		}
	}

	dealloc2df(m_transpose, size, size);
}
/*calculate cofactor of matrix*/
void cofactor(MMFLOAT** matrix, MMFLOAT** newmatrix, int size)
{
	MMFLOAT** m_cofactor = alloc2df(size, size);
	MMFLOAT** matrix_cofactor = alloc2df(size, size);
	int p, q, m, n, i, j;
	for (q = 0; q < size; q++)
	{
		for (p = 0; p < size; p++)
		{
			m = 0;
			n = 0;
			for (i = 0; i < size; i++)
			{
				for (j = 0; j < size; j++)
				{
					if (i != q && j != p)
					{
						m_cofactor[m][n] = matrix[i][j];
						if (n < (size - 2))
							n++;
						else
						{
							n = 0;
							m++;
						}
					}
				}
			}
			matrix_cofactor[q][p] = pow(-1, q + p) * determinant(m_cofactor, size - 1);
		}
	}
	transpose(matrix, matrix_cofactor, newmatrix, size);
	dealloc2df(m_cofactor, size, size);
	dealloc2df(matrix_cofactor, size, size);

}
/*For calculating Determinant of the Matrix . this function is recursive*/
MMFLOAT determinant(MMFLOAT** matrix, int size)
{
	MMFLOAT s = 1, det = 0;
	MMFLOAT** m_minor = alloc2df(size, size);
	int i, j, m, n, c;
	if (size == 1)
	{
		return (matrix[0][0]);
	}
	else
	{
		det = 0;
		for (c = 0; c < size; c++)
		{
			m = 0;
			n = 0;
			for (i = 0; i < size; i++)
			{
				for (j = 0; j < size; j++)
				{
					m_minor[i][j] = 0;
					if (i != 0 && j != c)
					{
						m_minor[m][n] = matrix[i][j];
						if (n < (size - 2))
							n++;
						else
						{
							n = 0;
							m++;
						}
					}
				}
			}
			det = det + s * (matrix[0][c] * determinant(m_minor, size - 1));
			s = -1 * s;
		}
	}
	dealloc2df(m_minor, size, size);
	return (det);
}
