//do all the includes that are standard based on the example
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <hdf5.h>
#include "nabCompression.h" //include the header file for this compression library


H5Z_class_t H5Z_NAB[1] = {{
	H5Z_CLASS_T_VERS, /* H5Z_class_t version */
	(H5Z_filter_t)H5Z_FILTER_NAB, /*filter id number*/
	1, /*encoder present flag (set to true)*/
	1, /*decoder present flag (set to true)*/
	"nab", /*filter name for debugging*/
	NULL, /*the "can apply" callback */
	NULL, /*the "set local" callback */
	(H5Z_func_t)H5Z_filter_nab, /*the actual filter function */
}};

size_t H5Z_filter_nab(unsigned int flags, size_t cd_nelmts,
			const unsigned int cd_values[], size_t nbytes,
			size_t *buf_size, void **buf);

extern int determinePowerOf2(int M);
extern bool checkIfDeltaFilter(short *filter, int filt_len);
extern void encodeWaveform(short *input, short *output, int input_len, short * filt, int filt_len);

extern bool checkIfDeltaFilter(short *filter, int filt_len){
	if(filt_len == 2){
		if(filter[0] == 1 && filter[1] == -1)
			return true;
		else
			return false;
	}
	return false;
}


extern void encodeWaveform(short *input, short *output, int input_len, short * filt, int filt_len){
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

extern void decodeWaveform(short *input, short *output, int input_len, short * filt, int filt_len){
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
				val = (read >> (48-bit))&32767; //48 because of 64 - 16, 32767 = 2**15 - 1
				if(val > 16383)
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
			short orig = waveIn[i];
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
				if (waveIn[i] >= 0)
					temp = temp|waveIn[i];
				else
					temp = temp|(waveIn[i]+32768); //this is 2**15, our values must be smaller than 2**15 so this lets the decoder know it was negative
				loc+= giveup + 17;// 8 for 8 zeros, 1 for ending the 8 zeros, and then 8 for binary value
				// CHECK THIS TO BE SURE, COULD BE WRONG
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

//this function goes through and does the actual decompression
//it assumes another function has figured out the sizes of the stuff
int readWholeCompressedByteString(void *inputBuffer, short *outputBuffer, superint numWaveforms, superint wavelength){
	char *buffer = inputBuffer;
	short *buffer16;
	superint loc = 0;
	loc += 16; //jump past the waveform length and number of waves information
	//figure out the filter length and size information
	short filterLength;
	buffer16 = (short*)(buffer+loc);	
	filterLength = buffer16[0];
	loc += 2;
	short *filter = (short*)malloc(sizeof(short) * filterLength);
	for(int f = 0; f < filterLength; f++){
		filter[f] = buffer16[f+1];
		loc += 2;
	}	
	short M = buffer16[1 + filterLength];
	loc += 2;
	//the outputBuffer has been allocated at this point
	//now iterate through the compressed data
	short *tempShortBuffer = (short*)malloc(wavelength * sizeof(short));
	superint outputloc = 0;
	int compressedLength;
	int *intBuf = (int*)(buffer+loc);
	unsigned int *uIntBuf = (unsigned int*)(buffer+loc);
	superint intloc = 0;
	for(unsigned int i = 0; i < numWaveforms; i++){
		compressedLength = intBuf[intloc];
		intloc +=1;
		//first figure out how long this compressed section is
		decompressWithRiceCoding(&uIntBuf[intloc], tempShortBuffer, wavelength, compressedLength, M);
		decodeWaveform(tempShortBuffer, &outputBuffer[outputloc], wavelength, filter, filterLength);
		outputloc += wavelength;
		intloc += compressedLength;
	}
	free(tempShortBuffer);	
	return 0;
}

int writeWholeCompressedByteString(size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf){
	//first verify the inputs actually make sense
	//first need to parse the input parameters to determine how to write the data
	if(cd_nelmts < 2){
		fprintf(stderr, "Improper number of options passed to filter creation. \n This code requires M, waveform length, and if applicable a custom filter for encoding.\n Input number of elements must be >=2 \n Received %ld \n", cd_nelmts);
		return -1;	
	}
	int M = (int)cd_values[0];
	int wavelength = (int)cd_values[1];
	//now check if there is a filter or not
	int filterLength = cd_nelmts - 2;
	short *filter;
	if(filterLength == 0){
		filterLength = 2;
		filter = (short*)malloc(sizeof(short)*filterLength);
		filter[0] = 1;
		filter[1] = -1;
	}
	else{
		filter = (short*)malloc(sizeof(short)*filterLength);
		for(int i = 0; i < filterLength; i++){
			filter[i] = (short)cd_values[2+i];	
		}
	}	
	//now with the filter parsed and the M and wavelength parameters known, we can do the stuff
	//make sure the waveform format makes sense
	if(nbytes % 2 == 0){
		if ((int)(nbytes / 2) % wavelength != 0){
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
	int numWaves = (nbytes/2)/wavelength;
	//now the compression can actually start
	//set up the output buffer, assume the compression doesn't expand the data
	char *outputBuffer = (char*)malloc(nbytes*2);
	superint loc = 0;
	superint* bit64buf = (superint*)outputBuffer;
	bit64buf[0] = numWaves;
	bit64buf[1] = wavelength;
	loc+=16;
	short *shortBuffer = (short*)(outputBuffer+loc);
	shortBuffer[0] = (short)filterLength;
	loc+=2;
	for(int i = 0; i < filterLength; i++){
		shortBuffer[i+1] = (short)filter[i];	
	}
	loc+=filterLength * 2;
	shortBuffer[1+filterLength] = (short)M;
	loc+=2;
	short *inputWaveforms = (short*)(*buf);
	short *tempEncodedBuffer = (short*)malloc(sizeof(short) * wavelength);
	
	unsigned int *outBufUInt = (unsigned int *)(outputBuffer+loc);
	int *outBufInt = (int*)(outputBuffer+loc);
	superint intloc = 0;
	for(int i = 0; i < numWaves; i++){//iterate over each waveform now
		encodeWaveform(&inputWaveforms[i*wavelength], tempEncodedBuffer, wavelength, filter, filterLength);//encode the waveform
		int compressedSize = compressWithRiceCoding(tempEncodedBuffer, &outBufUInt[1+intloc], wavelength, M);//output shifted over by 1
		if(compressedSize == -1){
			fprintf(stderr, "Waveform compression failed.\n");
			//free(tempEncodedBuffer);
			//free(filter);
			return -1;
		}	
		outBufInt[intloc] = compressedSize; //the compressed data size
		intloc += 1 + compressedSize;
	}
	loc += 4 * intloc;
	*buf_size = loc;
	//free(buf);
	*buf = outputBuffer;
	//free(tempEncodedBuffer);
	//free(filter);
	return 0;	
}

size_t H5Z_filter_nab(unsigned int flags, size_t cd_nelmts,
			const unsigned int cd_values[], size_t nbytes,
			size_t *buf_size, void **buf)
{
	if (flags & H5Z_FLAG_REVERSE){ // Decompress the data
		//in the case of decoding, first need to understand the output sizes and whatnot		
		//get that information first
		//first figure out how many waveforms and how long each one is
		superint *tempBuf = (superint*)(*buf);
		superint numWaves, wavelength;
		numWaves = tempBuf[0];
		wavelength = tempBuf[1];
		//now get the filter information
		short *outputBuffer = (short*)malloc(sizeof(short)*numWaves*wavelength);	
		int result = readWholeCompressedByteString(*buf, outputBuffer, numWaves, wavelength);
		if(result == -1){
			fprintf(stderr, "De-compression failed\n");
			return -1;
		}	
		//free(*buf);	
		*buf = outputBuffer;
		return numWaves * wavelength;
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
extern int nab_register_h5filter(void){
	int retval;
	retval = H5Zregister(H5Z_NAB);
	if(retval<0){
		fprintf(stderr, "nab_register_h5filter: can't register nab filter");
	}
	return retval;
}



