//do all the includes that are standard based on the example
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <hdf5.h>
#include "deltaRice.h" //include the header file for this compression library

//first thing to do is to determine if openmp was used or not in the compilation

//define a variable to see if openMP was used or not

#if defined(_OPENMP)
	#include <omp.h>
#endif

H5Z_class_t H5Z_DELTARICE[1] = {{
	H5Z_CLASS_T_VERS, /* H5Z_class_t version */
	(H5Z_filter_t)H5Z_FILTER_DELTARICE, /*filter id number*/
	1, /*encoder present flag (set to true)*/
	1, /*decoder present flag (set to true)*/
	"deltarice", /*filter name for debugging*/
	NULL, /*the "can apply" callback */
	NULL, /*the "set local" callback */
	(H5Z_func_t)H5Z_filter_deltarice, /*the actual filter function */
}};

size_t H5Z_filter_deltarice(unsigned int flags, size_t cd_nelmts,
			const unsigned int cd_values[], size_t nbytes,
			size_t *buf_size, void **buf);

extern int determinePowerOf2(int M);
extern bool checkIfDeltaFilter(int *filter, int filt_len);
extern void encodeWaveform(short *input, short *output, int input_len, int * filt, int filt_len);

extern bool checkIfDeltaFilter(int *filter, int filt_len){
	if(filt_len == 2){
		if(filter[0] == 1 && filter[1] == -1)
			return true;
		else
			return false;
	}
	return false;
}


extern void encodeWaveform(short *input, short *output, int input_len, int * filt, int filt_len){
	/* Go through and check if the filter is the special case or not */
	if (checkIfDeltaFilter(filt, filt_len)){
		//In this case we can do the far faster version of the delta encoding function
		short lastVal = input[0];
		output[0] = lastVal;
		short thisVal = 0;
		short temp = 0;
		for(int i = 1; i < input_len; i++){
			thisVal = input[i];
			temp = thisVal - lastVal;
			output[i] = temp;
			lastVal = thisVal;
		}
	}
	else{
		short out;
		for(int i = 0; i < input_len; i++){
			out = input[i] * filt[0];
			for(int j = 1; j < filt_len; j++){
				if ((i - j)>=0)
					out += input[i-j]*filt[j];
			output[i] = out;	
			}
		}
	}
	return;
}

extern void decodeWaveform(short *input, short *output, int input_len, int * filt, int filt_len){
	if (checkIfDeltaFilter(filt, filt_len)){
		short lastVal = input[0];
		output[0] = lastVal;
		short thisVal = 0;
		short temp = 0;
		for(int i = 1; i<input_len;i++){
			thisVal = input[i];
			temp = thisVal + lastVal;
			output[i] = temp;
			lastVal = temp;
		}
	}
	else{
		short temp;
		for(int i = 0; i < input_len; i++){
			temp = input[i];
			for(int j = 1; j < filt_len; j++){
				if ((i - j)>=0)
					temp -= output[i-j]*filt[j];
			}
			temp = temp / filt[0];
			output[i] = temp;
		}
	}
}

extern inline int getBit(superint cont, int bitnum, int end){
	return (cont >> (end - bitnum))&1;
}

extern inline int getBit64(superint cont, int bitnum){
	return ((cont>>(63-bitnum))&1);
}


int determinePowerOf2(int M){
	if(M<=0){
		fprintf(stderr, "Improper M Value: Must be >0: %d\n", M);
		return -1;
	}
	else if ((M&(M-1)) != 0){
		fprintf(stderr, "Improper M Value: Must be a power of 2: %d\n", M);
		return -1;
	}
	else{
		int i = 0;
		while(i < 32){
			if (M==1)
				return i;
			else{
				M = M>>1;
				i += 1;	
			}
		}	
	}
	fprintf(stderr, "Improper M Value: Must be < 2^32: %d\n", M);
	return -1;
}

void decompressWithRiceCoding(unsigned int *compressedIn, short *waveOut, int np, int compressedLength, int M){
	superint read = 0;
	int bit = 0;
	int q,r;
	int bitloc = 0;
	//load in the first buffer
	read = compressedIn[0];
	read = read << 32;
	read = read|(compressedIn[1]);
	//set up the remainder shift values, assuming that m = 8
	int rShift = determinePowerOf2(M);
	if(rShift == -1)
		return;
	else{
		int rShiftVal = (1 << (rShift)) - 1;
		int val = 0; //have to cast as an integer to make this work nicely for large values
		for(int i = 0; i < np; i++){
			q = 0;
			while(!getBit64(read,bit)){
				q++;
				bit++;
			}
			bit++;
			if(q == 8){ //this is the case when the quotient was too large
				val = (read >> (48-bit))&65535; //48 because of 64 - 16, 65535 = 2**16 - 1
				bit += 16;
			}
			else{
				//get the sign of the value now
				//now get the remainder
				r = (read>>(64 - rShift - bit))&rShiftVal; //(read>>(64-rshift-bit))&rshifthigh;
				bit += rShift; //rShift;
				val = (q<<rShift)+r;
			}
			if((val&1)==0) //if an even number divide by 2
				val = val>>1;
			else{
				val = val + 1; //if odd, add 1 and then divide by 2
				val = -1*(val>>1);
			}
			waveOut[i] = val;
			if(bit >= 32){//read in more to the buffer
				bit = bit - 32;
				bitloc++;
				read = compressedIn[bitloc];
				read = read << 32;
				if(bitloc != compressedLength - 1)
					read = read|compressedIn[bitloc + 1];
			}
		}
	}
}

int compressWithRiceCoding(short *waveIn, unsigned int *compressedOut, int np, int M){
	//we assume that the size of the compressedOut container is np datapoints
	unsigned int print = 0;//the value that will be output
	superint temp = 0;
	int len = 0;
	int loc = 0;
	int end = 32;
	int rShift = determinePowerOf2(M);
	if(rShift == -1)
		return -1;
	else{
		int rShiftVal = (1<<rShift)-1;
		int giveup = 8;
		short q,r;
		for(int i = 0; i < np; i++){
			//calculation quotient, remainder, and sign of the input values
			int orig = waveIn[i]; //cast to an integer temporarily for the absolute value operation
			orig = orig<<1;
			if(orig<0){ //if it's negative then make it the absolute value - 1
				orig=abs(orig)-1;
			}
			q = orig>>rShift;
			r = orig&rShiftVal;
			int locshift = 0;
			if( q < giveup){
				//deal with quotient
				temp = (temp<<(q+1))|1;//shift to the left that many values + 1
				locshift = q+1; //keep track of the current location
				//now handle the remainder
				temp = (temp<<rShift)|r;
				loc+= rShift + locshift;
			}
			else{ //if the quotient is too large, do the giveup signal
				temp = (temp << (giveup + 1))|1;
				temp = temp << 16;
				temp = temp|orig;
				loc+= giveup + 17;// 8 for 8 zeros, 1 for ending the 8 zeros, and then 8 for binary value
			}
			if(loc>=end){//if we have filled 32 bits worth of stuff up, write that to the output buffer
				print=temp >> (loc - end);
				temp = temp&((1<<(loc-end))-1);
				compressedOut[len] = print;
				loc = loc - end;
				len+=1;
			}
		}
		if(loc!=0){//if at the end we haven't freed the 32 bit buffer yet
			print = temp<<(end - loc);
			compressedOut[len] = print;
			len+=1;
		}
		return len;
	}
}

//this function goes through and parses the optional parameters that were given to the system
//it figures out what the user meant when they passed these values in
void parseCD_VALUES(size_t cd_nelmts, const unsigned int cd_values[], int *waveformLength, int *M, int *filterLen, int **filter){
	if(cd_nelmts == 0){
		//in this case, just do the defaults
		*waveformLength = -1;
		*M = 8;
		*filterLen = 2;
		int *tempfilter = (int*)malloc(sizeof(int) * *filterLen);
		tempfilter[0] = 1;
		tempfilter[1] = -1;
		*filter = tempfilter;
	}
	else if(cd_nelmts == 1){ //in this case, assume the first parameter passed is the tuning parameter
		*M = (int)cd_values[0];
		*waveformLength = -1;
		*filterLen = 2;
		int *tempfilter = (int*)malloc(sizeof(int) * *filterLen);
		tempfilter[0] = 1;
		tempfilter[1] = -1;
		*filter = tempfilter;
	}
	else if(cd_nelmts == 2){//first is the tuning parameter, second is the waveform length
		*M = (int)cd_values[0];
		*waveformLength = (int)cd_values[1];
		*filterLen = 2;
		int *tempfilter = (int*)malloc(sizeof(int) * *filterLen);
		tempfilter[0] = 1;
		tempfilter[1] = -1;
		*filter = tempfilter;
	}
	else{
		//in this case we passed something for the filter
		*M = (int)cd_values[0];
		*waveformLength = (int)cd_values[1];
		*filterLen = (int)cd_values[2];
		if(*filterLen > 0){
			int *tempfilter = (int*)malloc(sizeof(int) * *filterLen);
			for(int f = 0; f < *filterLen; f++){
				tempfilter[f] = (int)cd_values[3+f];
			}
			*filter = tempfilter;
		}
		//can possibly be more stuff here for future functionality with specifying byte size of data and threading
	}
}

void perWaveDecompression(short *outputBuffer, short *tempBuffer, unsigned int *inputBuffer, int wavelength, int *filter, int filterLen, int M){
	int compressedLength = inputBuffer[0];
	decompressWithRiceCoding(&inputBuffer[1], tempBuffer, wavelength, compressedLength, M);
	decodeWaveform(tempBuffer, outputBuffer, wavelength, filter, filterLen);
}

//this function goes through and does the actual decompression
//it assumes another function has figured out the sizes of the stuff
int readWholeCompressedByteString(size_t cd_nelmts, const unsigned int* cd_values, int nbytes, size_t *buf_size, void **buf){
	int wavelength, M, filterLen;
	int *filter;
	parseCD_VALUES(cd_nelmts, cd_values, &wavelength, &M, &filterLen, &filter);
	unsigned int *intBuf = (unsigned int*)(*buf); //cast the data being read in to unsigned integers
	int totalNumberPoints = intBuf[0];
	if(wavelength == -1)
		wavelength = totalNumberPoints;
	int numWaveforms = totalNumberPoints / abs(wavelength);
        int leftover = totalNumberPoints - wavelength * numWaveforms;
        if(leftover != 0)
            numWaveforms++;
	#if defined(_OPENMP)
	{
		//in the case that we want to do this with openmp, this is what we do
		short *tempShortBuffer = (short*)malloc(totalNumberPoints * sizeof(short));
		short *output = (short*)malloc(sizeof(short)*totalNumberPoints);//output buffer
		//first figure out the starting locations
		int *startLocs = (int*)malloc(sizeof(int)*numWaveforms);
		int curloc = 1; //starting read location
		startLocs[0] = curloc;
		for(int i = 1; i < numWaveforms; i++){
			curloc += intBuf[curloc] + 1;
			startLocs[i] = curloc;
		}
		int i;
		#pragma omp parallel for
		for(i = 0; i < numWaveforms; i++){
                        if((i==numWaveforms-1) && leftover!=0){//in the case that it's the last one to run and it's at the end of the file, just do the leftover bits
                		perWaveDecompression(&output[i*wavelength], &tempShortBuffer[i*wavelength], &intBuf[startLocs[i]], leftover, filter, filterLen, M);
		        }
                        else{
                            perWaveDecompression(&output[i*wavelength], &tempShortBuffer[i*wavelength], &intBuf[startLocs[i]], wavelength, filter, filterLen, M);
                        }
                }
		*buf = output;
		*buf_size = totalNumberPoints * 2;
		free(tempShortBuffer);
		free(intBuf);
		return totalNumberPoints;
	}
	#else
	{
		short *tempShortBuffer = (short*)malloc(wavelength * sizeof(short)); //buffer to undo the encoding inside
		short *output = (short*)malloc(sizeof(short) * totalNumberPoints); //output buffer
		int curloc = 1;
		for(int i = 0; i < numWaveforms; i++){
			perWaveDecompression(&output[i*wavelength], tempShortBuffer, &intBuf[curloc], wavelength, filter, filterLen, M);
			curloc += intBuf[curloc] + 1;
		}
		free(tempShortBuffer);
		free(filter);
		*buf = output;
		*buf_size = totalNumberPoints * 2;
		return totalNumberPoints;
	}
	#endif
}


//function to do the compression for each waveform
//this should be thread-safe
//input is the uncompressed buffer
//output is a compressed buffer for that waveform
size_t perWaveCompression(unsigned int *outputBuffer, short *inputWaveform, short *tempEncodedBuffer, int wavelength, int *filter, int filterLen, int M){
	//tempBuffer = output location
	//inputWaveform is the data to be processed
	//wavelength is how much data to be processed
	//filter is the compression/encoding filter
	//filterLen is the length of that filter
	encodeWaveform(inputWaveform, tempEncodedBuffer, wavelength, filter, filterLen);
	//now to the golomb/rice encoding
	int compressedSize = compressWithRiceCoding(tempEncodedBuffer, &outputBuffer[1], wavelength, M);
	if(compressedSize == -1){
		fprintf(stderr, "Waveform compression failed.\n");
		return 0;
	}
	size_t t = compressedSize + 1;
	outputBuffer[0] = compressedSize; //store the size of this section compressed
	return t;
}

int writeWholeCompressedByteString(size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf){
	//first verify the inputs actually make sense
	//first need to parse the input parameters to determine how to write the data
	int M, wavelength, filterLen;
	int *filter;
	parseCD_VALUES(cd_nelmts, cd_values, &wavelength, &M, &filterLen, &filter);
	int totalNumber = nbytes / 2; //total number of data points to unpack
	//if the waveform length is set to -1, then treat it all as one big ol batch
	if(wavelength == -1){
		wavelength = totalNumber;
	}
	else if(nbytes % 2 != 0){//check to make sure we have something that can properly fit within 2-byte numbers
		fprintf(stderr, "Input Size somehow not divisible by two? %ld \n", nbytes);
		//free(filter);
		return -1;
	}//otherwise the system should be good to go
	int numWaves = totalNumber/wavelength;
        int leftover = totalNumber - numWaves * wavelength; //if this is zero, the system divides evenly and we're fine
        if(leftover != 0){
            numWaves++;
        }
        //otherwise there is leftover and the last instance will need to deal with it
	//check if OPENMP was used during compilation, if so do this stuff instead
	#if defined(_OPENMP)
	{
		// now do the openMP version of the compression/decompression
		//define the various output buffers and whatnot
		short *inputWaveforms = (short*)(*buf);
		size_t maxBufferSize = nbytes * 2; //most they can be is twice the initial size if it completely fails cause giveup = 8 so 9 bits of fail + 16 bits of the original value < 32 total
		unsigned int *outputBuffer = (unsigned int*)malloc(maxBufferSize+1*numWaves+1); //the whole output buffer, bonus 1 for the initial output values
		short *tempEncodedBuffer = (short*)malloc(nbytes);
		size_t *outputSizes = (size_t*)malloc(sizeof(size_t)*numWaves); //one per waveform
		outputBuffer[0] = totalNumber; //the total number of datapoints we expect to unpack on the receiving end
		int i;
		#pragma omp parallel for
		for(i = 0; i < numWaves; i++){ //this should run in parallel now across all threads available on the PC
			//first allocate the space
                        if((i==numWaves-1) && (leftover != 0)){
                            outputSizes[i] = perWaveCompression(&outputBuffer[i*wavelength+i+1], &inputWaveforms[i*wavelength], &tempEncodedBuffer[i*wavelength], leftover, filter, filterLen, M);
		        }
                        else{
                            outputSizes[i] = perWaveCompression(&outputBuffer[i*wavelength+i+1], &inputWaveforms[i*wavelength], &tempEncodedBuffer[i*wavelength], wavelength, filter, filterLen, M);
                        }
                }
		size_t currloc = 1;
		//#pragma omp parallel for ordered
		for(int i = 0; i < numWaves; i++){
			memcpy(&outputBuffer[currloc], &outputBuffer[i*wavelength+i+1], outputSizes[i]*4);
			currloc += outputSizes[i];
		}
		*buf_size = currloc*4;
		outputBuffer = realloc(outputBuffer, *buf_size);
		free(*buf); //free the original buffer
		*buf = outputBuffer; //replace it with the new buffer
		free(outputSizes);
		free(filter);
		free(tempEncodedBuffer);
		return 0;
	}
	#else
	{
		short *inputWaveforms = (short*)(*buf);
		size_t totalSize = 0;
		size_t maxBufferSize = nbytes * 2; //most they can be is twice the initial size if it completely fails cause giveup = 8 so 9 bits of fail + 16 bits of the original value < 32 total
		unsigned int *outputBuffer = (unsigned int*)malloc(maxBufferSize+1*numWaves+1); //the whole output buffer, bonus 1 for the initial output values
		short *tempEncodedBuffer = (short*)malloc(sizeof(short)*wavelength);
		outputBuffer[0] = totalNumber; //the total number of datapoints we expect to unpack on the receiving end
		size_t currloc = 1;
		for(int i = 0; i < numWaves; i++){
			//first allocate the space
			currloc += perWaveCompression(&outputBuffer[currloc], &inputWaveforms[i*wavelength], tempEncodedBuffer, wavelength, filter, filterLen, M);
		}
		*buf_size = currloc*4;
		outputBuffer = realloc(outputBuffer, *buf_size);
		
		free(*buf); //free the original buffer
		*buf = outputBuffer; //replace it with the new buffer
		free(filter);
		free(tempEncodedBuffer);
		return 0;
	}
	#endif
}


size_t H5Z_filter_deltarice(unsigned int flags, size_t cd_nelmts,
			const unsigned int cd_values[], size_t nbytes,
			size_t *buf_size, void **buf)
{
	
	if (flags & H5Z_FLAG_REVERSE){ // Decompress the data
		int result = readWholeCompressedByteString(cd_nelmts, cd_values, nbytes, buf_size, buf);
		if(result == -1){
			fprintf(stderr, "De-compression failed\n");
			return -1;
		}
		return *buf_size;
	}
	else{ //compress the data
		int result = writeWholeCompressedByteString(cd_nelmts, cd_values, nbytes, buf_size, buf);
		if(result == -1){
			fprintf(stderr, "Compression failed\n");
			//free(buf);
			return -1;
		}
		return *buf_size;
	}
}


//modeled this function off of bshuf_h5filter.c from the bitshuffle gitlab
extern int deltarice_register_h5filter(void){
	int retval;
	retval = H5Zregister(H5Z_DELTARICE);
	if(retval<0){
		fprintf(stderr, "deltarice_register_h5filter: can't register deltarice filter");
	}
	return retval;
}



