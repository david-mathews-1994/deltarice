//do all the includes that are standard based on the example
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <hdf5.h>
#include "deltaRice.h" //include the header file for this compression library


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
	int q,r,sign;
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
		short val = 0;
		for(int i = 0; i < np; i++){
			q = 0;
			while(!getBit64(read,bit)){
				q++;
				bit++;
			}
			bit++;
			if(q == 8){ //this is the case when the quotient was too large
				val = (read >> (48-bit))&65535; //48 because of 64 - 16, 65535 = 2**16 - 1
				val = val - 32768;
				bit += 16;
			}
			else{
				//get the sign of the value now
				sign = ((getBit64(read, bit)^1)<<1)-1;
				bit++;
				//now get the remainder
				r = (read>>(64 - rShift - bit))&rShiftVal; //(read>>(64-rshift-bit))&rshifthigh;
				bit += rShift; //rShift;
				val = (q<<rShift)+r;
				val = sign*val;
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
		short q,r,sign;
		for(int i = 0; i < np; i++){
			//calculation quotient, remainder, and sign of the input values
			int orig = waveIn[i]; //cast to an integer temporarily for the absolute value operation
			sign = orig >=0;
			orig = abs(orig);
			q = orig>>rShift;
			r = orig&rShiftVal;
			int locshift = 0;
			if( q < giveup){
				//deal with quotient
				temp = (temp<<(q+1))|1;//shift to the left that many values + 1
				locshift = q+1; //keep track of the current location
				if (sign == 1)
					temp=temp<<1;
				else
					temp = (temp<<1)|1;
				//now handle the remainder
				temp = (temp<<rShift)|r;
				loc+= 1 + rShift + locshift;
			}
			else{ //if the quotient is too large, do the giveup signal
				temp = (temp << (giveup + 1))|1;
				temp = temp << 16;
				temp = temp|(waveIn[i]+32768);
				loc+= giveup + 17;// 8 for 8 zeros, 1 for ending the 8 zeros, and then 8 for binary value
			}
			if(loc>=end){//if we have filled 32 bits worth of stuff up, write that
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
		//can possibly be more stuff here for future functionality with specifying byte size of data
	}
}

//this function goes through and does the actual decompression
//it assumes another function has figured out the sizes of the stuff
int readWholeCompressedByteString(void *inputBuffer, short **outputBuffer, size_t cd_nelmts, const unsigned int* cd_values, int nbytes){
	int wavelength, M, filterLen;
	int *filter;
	parseCD_VALUES(cd_nelmts, cd_values, &wavelength, &M, &filterLen, &filter);
	unsigned int *buffer = inputBuffer;
	int totalNumberPoints = buffer[0];	
	if(wavelength == -1)
		wavelength = totalNumberPoints;
	int numWaveforms = totalNumberPoints / abs(wavelength);
	short *tempShortBuffer = (short*)malloc(wavelength * sizeof(short));
	superint outputloc = 0;
	int compressedLength;
	unsigned int *intBuf = (unsigned int*)buffer;
	superint intloc = 1;
	short *output = (short*)malloc(sizeof(short) * totalNumberPoints);
	for(int i = 0; i < numWaveforms; i++){
		compressedLength = intBuf[intloc];
		intloc +=1;
		//first figure out how long this compressed section is
		decompressWithRiceCoding(&buffer[intloc], tempShortBuffer, wavelength, compressedLength, M);
		decodeWaveform(tempShortBuffer, &output[outputloc], wavelength, filter, filterLen);
		outputloc += wavelength;
		intloc += compressedLength;
	}
	free(tempShortBuffer);
	free(filter);
	*outputBuffer = output;	
	return totalNumberPoints;
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
	else if(nbytes % 2 == 0){
		if((int)(nbytes / 2) % wavelength != 0){
			fprintf(stderr, "Wavelength doesn't divide evenly with size of dataset: %ld / %d has != remainder\n", nbytes/2, wavelength);
			//free(filter);
			return -1;
		}
	}
	else{
		fprintf(stderr, "Input Size somehow not divisible by two? %ld \n", nbytes);
		//free(filter);
		return -1;
	}
	int numWaves = totalNumber/wavelength;
	//now the compression can actually start
	//set up the output buffer, assume the compression doesn't expand the beyond 2 times the initial size
	unsigned int *outputBuffer = (unsigned int*)malloc(nbytes * 2 + 1); //with an additional value for the length of data being output
	//first write the total number of waveforms to the file
	outputBuffer[0] = (unsigned int)totalNumber;
	short *inputWaveforms = (short*)(*buf);
	short *tempEncodedBuffer = (short*)malloc(sizeof(short) * wavelength);
	superint intloc = 1;
	for(int i = 0; i < numWaves; i++){//iterate over each waveform now
		encodeWaveform(&inputWaveforms[i*wavelength], tempEncodedBuffer, wavelength, filter, filterLen);//encode the waveform
		int compressedSize = compressWithRiceCoding(tempEncodedBuffer, &outputBuffer[1+intloc], wavelength, M);//output shifted over by 1
		if(compressedSize == -1){
			fprintf(stderr, "Waveform compression failed.\n");
			//free(tempEncodedBuffer);
			//free(filter);
			return -1;
		}	
		outputBuffer[intloc] = compressedSize; //the compressed data size
		intloc += 1 + compressedSize;
	}
	*buf_size = 4 * intloc;
	//now reallocate the size to be the proper output size
	free(*buf);
	*buf = realloc(outputBuffer, *buf_size);
	free(tempEncodedBuffer);
	free(filter);
	return 0;	
}


size_t H5Z_filter_deltarice(unsigned int flags, size_t cd_nelmts,
			const unsigned int cd_values[], size_t nbytes,
			size_t *buf_size, void **buf)
{
	if (flags & H5Z_FLAG_REVERSE){ // Decompress the data
		//parse the optional parameters
		//now get the filter information
		//the number of data points is the first thing stored in the output
		short *outputBuffer;	
		int result = readWholeCompressedByteString(*buf, &outputBuffer, cd_nelmts, cd_values, nbytes);
		if(result == -1){
			fprintf(stderr, "De-compression failed\n");
			return -1;
		}	
		free(*buf);	
		*buf = outputBuffer;
		return result;
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



