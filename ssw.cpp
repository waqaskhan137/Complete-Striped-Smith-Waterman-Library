/*
 *  ssw.cpp
 *
 *  Created by Mengyao Zhao on 6/22/10.
 *  Copyright 2010 Boston College. All rights reserved.
 *	Version 0.3
 *	Revised by Mengyao Zhao on 8/27/10.
 *	New features: Record the highest score of each reference position. 
 *	Geve out the most possible 2nd distinguished best alignment score as well as the best alignment score and
 *	its ending position. 
 *
 */

using namespace std;
#include <emmintrin.h>
#include "ssw.h"

// for Weight matrix
#define A 0
#define C 1
#define G 2
#define T 3
#define K 4  // G or T
#define M 5  // A or C
#define R 6  // A or G
#define Y 7  // C or T
#define S 8  // A or T
#define B 9  // C or G or T
#define V 10 // A or C or G
#define H 11 // A or C or T
#define D 12 // A or G or T
#define N 13 // any
#define X 14 // any, X mask on Y chromosome

// Transform amino acid to numbers for Weight Matrix.
unsigned int amino2num (char amino) {
	unsigned int num;
	switch (amino) {
		case 'A':
		case 'a':
			num = 0;
			break;
		case 'C':
		case 'c':
			num = 1;
			break;
		case 'G':
		case 'g':
			num = 2;
			break;
		case 'T':
		case 't':
			num = 3;
			break;
		case 'K': // G or T
		case 'k':
			num = 4;
			break;
		case 'M': // A or C
		case 'm':
			num = 5;
			break;
		case 'R': // A or G
		case 'r':
			num = 6;
			break;
		case 'Y': // C or T
		case 'y':
			num = 7;
			break;
		case 'S': // A or T
		case 's':
			num = 8;
			break;
		case 'B': // C or G or T
		case 'b':
			num = 9;
			break;
		case 'V': // A or C or G
		case 'v':
			num = 10;
			break;
		case 'H': // A or C or T
		case 'h':
			num = 11;
			break;
		case 'D': // A or G or T
		case 'd':
			num = 12;
			break;
		case 'N': // any
		case 'n':
			num = 13;
			break;
		case 'X': // any, x mask on Y chromosome
		case 'x':
			num = 14;
			break;
		default:
			fprintf(stderr, "Wrong sequence. \n");
			break;
	}
	return num;
}

// Create scoring matrix W.
char** matrixScore_constructor (unsigned char weight_match, 
								unsigned char weight_mismatch) {
	
	char** W = (char**)calloc(15, sizeof(char*));
	for (unsigned int i = 0; i < 15; i ++) {
		W[i] = (char*) calloc(15, sizeof(char));
	}
	for (unsigned int i = 0; i < 15; i ++) {
		for (unsigned int j = 0; j < 15; j ++) {
			if (i == j) {
				W[i][j] = weight_match;
			} else {
				W[i][j] = - weight_mismatch;
			}
		}
	}
	
	W[G][K] = W[K][G] = weight_match; // K
	W[T][K] = W[K][T] = weight_match;
	
	W[A][M] = W[M][A] = weight_match; // M
	W[C][M] = W[M][C] = weight_match;
	
	W[A][R] = W[R][A] = weight_match; // R
	W[G][R] = W[R][G] = weight_match;
	
	W[C][Y] = W[Y][C] = weight_match; // Y
	W[T][Y] = W[Y][T] = weight_match;
	
	W[A][S] = W[S][A] = weight_match; // S
	W[T][S] = W[S][T] = weight_match;
	
	W[C][B] = W[B][C] = weight_match; // B
	W[G][B] = W[B][G] = weight_match;
	W[T][B] = W[B][T] = weight_match;
	
	W[A][V] = W[V][A] = weight_match; // V
	W[C][V] = W[V][C] = weight_match; 
	W[G][V] = W[V][G] = weight_match;
	
	W[A][H] = W[H][A] = weight_match; // H
	W[C][H] = W[H][C] = weight_match; 
	W[T][H] = W[H][T] = weight_match;
	
	W[A][D] = W[D][A] = weight_match; // D
	W[G][D] = W[D][G] = weight_match;
	W[T][D] = W[D][T] = weight_match;
	
	return W;
}

// Generate query profile：rearrange query sequence & calculate the weight of match/mismatch.
char** queryProfile_constructor (const char* read,
								 unsigned char weight_match,	// will be used as +
								 unsigned char weight_mismatch, // will be used as -
								 unsigned char bias) { 
					
	char** W = matrixScore_constructor (weight_match, weight_mismatch); // Create scoring matrix.
	
	// Generate query profile：rearrange query sequence & calculate the weight of match/mismatch.
	unsigned int readLen = strlen(read);
	unsigned int
	segLen = (readLen + 15) / 16; // Split the 128 bit register into 16 pieces. 
								  // Each piece is 8 bit. Split the read into 16 segments. 
								  // Calculat 16 segments in parallel.
	char** queryProfile = (char**)calloc(15, sizeof(char*)); // Could be negative value.
	for (unsigned int i = 0; i < 15; i ++) {
		queryProfile[i] = (char*)calloc(segLen * 16, sizeof(char));
	}
	for (unsigned int amino = 0; amino < 15; amino ++) {
		unsigned int Hseq = 0;
		for (unsigned int i = 0; i < segLen; i ++) {
			unsigned int j = i;
			for (unsigned int segNum = 0; segNum < 16; segNum ++) {
				if (j >= readLen) { 
				    // We are beyond the length of the query string, 
					// so pad the weights with neutral scores.
					queryProfile[amino][Hseq] = 0; 
				} else {
				    // Set the score to the weight in the scoring matrix.
					queryProfile[amino][Hseq] = W[amino][amino2num(read[j])] + bias;  
				}                             
				Hseq ++;
				j += segLen;
			}
		}
	}
	
	for (unsigned int i = 0; i < 15; i ++) {
		free(W[i]);
	}
	free(W);
	
	return queryProfile;
}

// Free the memory of queryProfile.
void queryProfile_destructor (char** queryProfile) {
	for (unsigned int i = 0; i < 15; i ++) {
		free(queryProfile[i]);
	}
	free(queryProfile);
}

// Trace the max score from the max score vector.
unsigned char findMax (__m128i vMaxScore, 
					   unsigned int* pos_vector) { // pos_vector should be originally 0.
	
	unsigned char max = 0;
	unsigned int two[8];
	two[0] = _mm_extract_epi16(vMaxScore, 0);
	two[1] = _mm_extract_epi16(vMaxScore, 1);
	two[2] = _mm_extract_epi16(vMaxScore, 2);
	two[3] = _mm_extract_epi16(vMaxScore, 3);
	two[4] = _mm_extract_epi16(vMaxScore, 4);
	two[5] = _mm_extract_epi16(vMaxScore, 5);
	two[6] = _mm_extract_epi16(vMaxScore, 6);
	two[7] = _mm_extract_epi16(vMaxScore, 7);
	for (int i = 0; i < 8; i ++) {
		unsigned int low = two[i] & 0x00ff;
		if (low > max) {
			*pos_vector = i * 2;
			max = low;
		}
		unsigned int high = two[i] & 0xff00;
		high = high >> 8;
		if (high > max) {
			*pos_vector = i * 2 + 1;
			max = high;
		}
	}
	return max;
}

// Striped Smith-Waterman
// Record the highest score of each reference position. 
// Return the alignment score and ending position of the best alignment, 2nd best alignment, etc. 
// Gap begin and gap extention are different. 
// wight_match > 0, all other weights < 0. 
alignment_end* smith_waterman_sse2 (const char* ref, 
								    unsigned int readLen, 
								    unsigned char weight_insertB, // will be used as -
								    unsigned char weight_insertE, // will be used as -
								    unsigned char weight_deletB,  // will be used as -
								    unsigned char weight_deletE,  // will be used as -
								    char** queryProfile,
								    unsigned int* end_seg,        // 0-based segment number of ending  
								                                  // alignment; The return value is  
																  // meaningful only when  
									  							  // the return value != 0.
	 							    unsigned char bias) {         // Shift 0 point to a positive value.
	
	*end_seg = 0; // Initialize as aligned at num. 0 segment.
	unsigned char max = 0;		                     // the max alignment score
	unsigned int end_read = 0;
	unsigned int end_ref = 0; // 1_based best alignment ending point; Initialized as isn't aligned - 0.
	unsigned int refLen = strlen(ref);
	unsigned int segLen = (readLen + 15) / 16;
	
	// array to record the largest score of each reference position
	char* maxColumn = (char*) calloc(refLen, 1); 
	
	// array to record the alignment read ending position of the largest score of each reference position 
	unsigned int* end_read_column = (unsigned int*) calloc(refLen, sizeof(unsigned int));
	
	// Define 16 byte 0 vector.
	__m128i vZero; 
	vZero = _mm_xor_si128(vZero, vZero);

	__m128i* pvHStore = (__m128i*) calloc(segLen, sizeof(__m128i));
	__m128i* pvHLoad = (__m128i*) calloc(segLen, sizeof(__m128i));
	__m128i* pvE = (__m128i*) calloc(segLen, sizeof(__m128i));
	for (unsigned int i = 0; i < segLen; i ++) {
		pvHStore[i] = vZero;
		pvE[i] = vZero;
	}
	
	// 16 byte insertion begin vector
	__m128i vInserB = _mm_set_epi8(weight_insertB, weight_insertB, weight_insertB, weight_insertB, 
								   weight_insertB, weight_insertB, weight_insertB, weight_insertB,
								   weight_insertB, weight_insertB, weight_insertB, weight_insertB, 
								   weight_insertB, weight_insertB, weight_insertB, weight_insertB);
	
	// 16 byte insertion extention vector
	__m128i vInserE = _mm_set_epi8(weight_insertE, weight_insertE, weight_insertE, weight_insertE, 
								   weight_insertE, weight_insertE, weight_insertE, weight_insertE, 
								   weight_insertE, weight_insertE, weight_insertE, weight_insertE, 
								   weight_insertE, weight_insertE, weight_insertE, weight_insertE);
	
	// 16 byte deletion begin vector
	__m128i vDeletB = _mm_set_epi8(weight_deletB, weight_deletB, weight_deletB, weight_deletB, 
								   weight_deletB, weight_deletB, weight_deletB, weight_deletB, 
								   weight_deletB, weight_deletB, weight_deletB, weight_deletB, 
								   weight_deletB, weight_deletB, weight_deletB, weight_deletB);

	// 16 byte deletion extention vector
	__m128i vDeletE = _mm_set_epi8(weight_deletE, weight_deletE, weight_deletE, weight_deletE, 
								   weight_deletE, weight_deletE, weight_deletE, weight_deletE, 
								   weight_deletE, weight_deletE, weight_deletE, weight_deletE, 
								   weight_deletE, weight_deletE, weight_deletE, weight_deletE);

	// 16 byte bias vector
	__m128i vBias = _mm_set_epi8(bias, bias, bias, bias, bias, bias, bias, bias,  
								 bias, bias, bias, bias, bias, bias, bias, bias);

	__m128i vMaxScore = vZero; // Trace the highest score of the whole SW matrix.
	__m128i vMaxMark = vZero; // Trace the highest score till the previous column.	
	__m128i vTemp;
	
	// outer loop to process the reference sequence
	for (unsigned int i = 0; i < refLen; i ++) {
		__m128i vF = vZero; // Initialize F value to 0. 
							// Any errors to vH values will be corrected in the Lazy_F loop. 
		__m128i vH = pvHStore[segLen - 1];
		vH = _mm_slli_si128 (vH, 1); // Shift the 128-bit value in vH left by 1 byte.
		
		// Swap the 2 H buffers.
		__m128i* pv = pvHLoad;
		pvHLoad = pvHStore;
		pvHStore = pv;
		
		unsigned int Hseq = 0;
		//__m128i vMaxSame = vZero; // vMaxSame is used to count the number of best alignments.
		__m128i vMaxColumn = vZero; // vMaxColumn is used to record the max values of column i.
		
		// inner loop to process the query sequence
		for (unsigned int j = 0; j < segLen; j ++) {
			
			// Add the scoring profile to vH
			// vProfile can include negative value.
			__m128i vProfile = _mm_set_epi8(queryProfile[amino2num(ref[i])][Hseq + 15], 
											queryProfile[amino2num(ref[i])][Hseq + 14], 
											queryProfile[amino2num(ref[i])][Hseq + 13], 
											queryProfile[amino2num(ref[i])][Hseq + 12], 
											queryProfile[amino2num(ref[i])][Hseq + 11], 
											queryProfile[amino2num(ref[i])][Hseq + 10], 
											queryProfile[amino2num(ref[i])][Hseq + 9], 
											queryProfile[amino2num(ref[i])][Hseq + 8], 
											queryProfile[amino2num(ref[i])][Hseq + 7], 
											queryProfile[amino2num(ref[i])][Hseq + 6], 
											queryProfile[amino2num(ref[i])][Hseq + 5], 
											queryProfile[amino2num(ref[i])][Hseq + 4], 
											queryProfile[amino2num(ref[i])][Hseq + 3], 
											queryProfile[amino2num(ref[i])][Hseq + 2], 
											queryProfile[amino2num(ref[i])][Hseq + 1], 
											queryProfile[amino2num(ref[i])][Hseq]); 
			Hseq += 16;

			vH = _mm_adds_epu8(vH, vProfile);
			vH = _mm_subs_epu8(vH, vBias); // vH will be always > 0

			// Get max from vH, vE and vF.
			vH = _mm_max_epu8(vH, pvE[j]);
			vH = _mm_max_epu8(vH, vF);
			
			// Update highest score encountered this far.
			vMaxScore = _mm_max_epu8(vMaxScore, vH);
			vMaxColumn = _mm_max_epu8(vMaxColumn, vH);
			
			// Save vH values.
			pvHStore[j] = vH;

			// Update vE value.
			vH = _mm_subs_epu8(vH, vInserB); // saturation arithmetic, result >= 0
			pvE[j] = _mm_subs_epu8(pvE[j], vInserE);
			pvE[j] = _mm_max_epu8(pvE[j], vH);
			
			// Update vF value.
			vF = _mm_subs_epu8(vF, vDeletE);
			vF = _mm_max_epu8(vF, vH);
			
			// Load the next vH.
			vH = pvHLoad[j];
		}
		
		// Lazy_F loop
		// The computed vF value is for the given column. 
		// Since we are at the end, we need to shift the vF value over to the next column.
		vF = _mm_slli_si128 (vF, 1);
		
		// Correct the vH values until there are no element in vF that could influence the vH values.
		unsigned int j = 0;
		vTemp = _mm_subs_epu8(pvHStore[j], vDeletB);
		vTemp = _mm_subs_epu8(vF, vTemp);
		vTemp = _mm_cmpeq_epi8(vTemp, vZero); // result >= 0
		unsigned int cmp = _mm_movemask_epi8(vTemp);
		
		while (cmp != 0xffff) {
			pvHStore[j] = _mm_max_epu8(pvHStore[j], vF);
			
			// Update highest score incase the new vH value would change it. (New line I added!)
			vMaxScore = _mm_max_epu8(vMaxScore, pvHStore[j]);
			vMaxColumn = _mm_max_epu8(vMaxColumn, pvHStore[j]);
			
			// Update vE incase the new vH value would change it.
			vH = _mm_subs_epu8(pvHStore[j], vInserB);
			pvE[j] = _mm_max_epu8(pvE[j], vH);
			
			// Update vF value.
			vF = _mm_subs_epu8(vF, vDeletE);
			
			j ++;
			if (j >= segLen) {
				j = 0;
				vF = _mm_slli_si128 (vF, 1);
			}
			vTemp = _mm_subs_epu8(pvHStore[j], vDeletB);
			vTemp = _mm_subs_epu8(vF, vTemp);
			vTemp = _mm_cmpeq_epi8(vTemp, vZero); // result >= 0
			cmp = _mm_movemask_epi8(vTemp);
		}		
		// end of Lazy-F loop
		
		// loop for tracing the ending point of the best alignment
		vTemp = vMaxMark;
		vMaxMark = _mm_max_epu8(vMaxScore, vMaxMark);
		vTemp = _mm_cmpeq_epi8(vMaxMark, vTemp);
		cmp = _mm_movemask_epi8(vTemp);
		j = 0;
		
		while (cmp != 0xffff) {
			vMaxMark = _mm_max_epu8(vMaxMark, pvHStore[j]);
			end_ref = i + 1; // Adjust to 1-based position.
			unsigned int p = 0;
			unsigned char temp = findMax(vMaxMark, &p);
			if (temp > max) {
				*end_seg = j;
				end_read = p * segLen + j + 1;  
				max = temp;
			}
			vTemp = _mm_cmpeq_epi8(vMaxMark, vMaxScore);
			cmp = _mm_movemask_epi8(vTemp);
			j ++;
			if (j >= segLen) {
				j = 0;
			}
		}
		
		// loop to record the max score of each column as well as the alignment ending position on the read
		j = 0;
		maxColumn[i] = 0;
		vTemp = pvHStore[j];
		cmp = _mm_movemask_epi8(vTemp);
		while (cmp != 0xffff) {
			vTemp = _mm_max_epu8(vTemp, pvHStore[j]);
			//			*end_ref = i + 1; // Adjust to 1-based position.
			unsigned int p = 0;
			unsigned char temp = findMax(vTemp, &p);
			if (temp > maxColumn[i]) {
				//				*end_seg = j;
				end_read_column[i] = p * segLen + j + 1;  
				maxColumn[i] = temp;
			}
			vTemp = _mm_cmpeq_epi8(vTemp, vMaxColumn);
			cmp = _mm_movemask_epi8(vTemp);
			j ++;
			if (j >= segLen) {
				j = 0;
			}
		}	
		
	} 	
	
	for (unsigned int i = 0; i < refLen; i ++) {
		fprintf(stderr, "%d\t", maxColumn[i]);
	}
	fprintf(stderr, "\n");
	
	// Find the most possible 2nd best alignment.
	alignment_end* bests = (alignment_end*) calloc(2, sizeof(alignment_end));
	bests[0].score = max;
	bests[0].ref = end_ref;
	bests[0].read = end_read;
	
	bests[1].score = 0;
	bests[1].ref = 0;
	bests[1].read = 0;
	
	//unsigned char max2 = 0;
	unsigned int edge = (end_ref - readLen / 2 - 1) > 0 ? (end_ref - readLen / 2 - 1) : 0;
	for (unsigned int i = 0; i < edge; i ++) {
		if (maxColumn[i] > bests[1].score) {
			bests[1].score = maxColumn[i];
			bests[1].ref = i + 1;
			bests[1].read = end_read_column[i];
		}
	}
	edge = (end_ref + readLen / 2 + 1) > refLen ? refLen : (end_ref + readLen / 2 + 1);
	for (unsigned int i = edge; i < refLen; i ++) {
		if (maxColumn[i] > bests[1].score) {
			bests[1].score = maxColumn[i];
			bests[1].ref = i + 1;
			bests[1].read = end_read_column[i];
		}		
	}
	
	free(pvHStore);
	free(maxColumn);
	free(end_read_column);
	return bests;
}

// Reverse Smith-Waterman beginning from the end of the best alignment. 
// Alignment beginning position will be signed to end_ref & end_read when finish running.
/*
void reverse_smith_waterman_sse2 (const char* ref, 
								  unsigned int segLen, 
								  unsigned char weight_insertB, // will be used as -
								  unsigned char weight_insertE, // will be used as -
								  unsigned char weight_deletB, // will be used as -
								  unsigned char weight_deletE, // will be used as -
								  char** queryProfile,
								  unsigned int end_ref, // 1-based position of the alignment ending. 0 is not aligned.
								  unsigned int* begin_ref, // 1-based position of the alignment begin
								  unsigned int* begin_read, // 1-based position of the alignment begin
								  unsigned int end_seg, // 0-based segment number of ending alignment
								  unsigned char max, // the best alignment score 
								  unsigned char bias) { // Shift 0 point to a positive value.
	
	// Define 16 byte 0 vector.
	__m128i vZero; 
	vZero = _mm_xor_si128(vZero, vZero);
	
	__m128i* pvHStore = (__m128i*) calloc(segLen, sizeof(__m128i));
	__m128i* pvHLoad = (__m128i*) calloc(segLen, sizeof(__m128i));
	__m128i* pvE = (__m128i*) calloc(segLen, sizeof(__m128i));
	for (unsigned int i = 0; i < segLen; i ++) {
		pvHStore[i] = vZero;
		pvE[i] = vZero;
	}
	
	// 16 byte insertion begin vector
	__m128i vInserB = _mm_set_epi8(weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB, weight_insertB);
	
	// 16 byte insertion extention vector
	__m128i vInserE = _mm_set_epi8(weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE, weight_insertE);
	
	// 16 byte deletion begin vector
	__m128i vDeletB = _mm_set_epi8(weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB, weight_deletB);
	
	// 16 byte deletion extention vector
	__m128i vDeletE = _mm_set_epi8(weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE, weight_deletE);
	
	// 16 byte bias vector
	__m128i vBias = _mm_set_epi8(bias, bias, bias, bias, bias, bias, bias, bias, bias, bias, bias, bias, bias, bias, bias, bias);
	
	__m128i vMaxScore = vZero;
	
	// 16 byte max vector
	__m128i vMaxMark = _mm_set_epi8(max, max, max, max, max, max, max, max, max, max, max, max, max, max, max, max);
	
	__m128i vTemp;
	
	// outer loop to process the reference sequence
	for (unsigned int i = end_ref - 1; i >= 0; i --) {
		__m128i vF = vZero; // Initialize F value to 0. Any errors to vH values will be corrected in the Lazy_F loop. 
		__m128i vH = pvHStore[segLen - 1];
		vH = _mm_srli_si128 (vH, 1); // Shift the 128-bit value in vH right by 1 byte.
		
		// Swap the 2 H buffers.
		__m128i* pv = pvHLoad;
		pvHLoad = pvHStore;
		pvHStore = pv;
		
		unsigned int Hseq = 0;
		// inner loop to process the query sequence
		for (int j = end_seg; j >= 0; j --) {
			
			// Add the scoring profile to vH
			// vProfile can include negative value.
			__m128i vProfile = _mm_set_epi8(queryProfile[amino2num(ref[i])][Hseq + 15], queryProfile[amino2num(ref[i])][Hseq + 14], queryProfile[amino2num(ref[i])][Hseq + 13], queryProfile[amino2num(ref[i])][Hseq + 12], queryProfile[amino2num(ref[i])][Hseq + 11], queryProfile[amino2num(ref[i])][Hseq + 10], queryProfile[amino2num(ref[i])][Hseq + 9], queryProfile[amino2num(ref[i])][Hseq + 8], queryProfile[amino2num(ref[i])][Hseq + 7], queryProfile[amino2num(ref[i])][Hseq + 6], queryProfile[amino2num(ref[i])][Hseq + 5], queryProfile[amino2num(ref[i])][Hseq + 4], queryProfile[amino2num(ref[i])][Hseq + 3], queryProfile[amino2num(ref[i])][Hseq + 2], queryProfile[amino2num(ref[i])][Hseq + 1], queryProfile[amino2num(ref[i])][Hseq]); 
			Hseq += 16;
			
			vH = _mm_adds_epu8(vH, vProfile);
			vH = _mm_subs_epu8(vH, vBias); // vH will be always > 0
			
			// Get max from vH, vE and vF.
			vH = _mm_max_epu8(vH, pvE[j]);
			vH = _mm_max_epu8(vH, vF);
			
			// Update highest score encountered this far.
			vMaxScore = _mm_max_epu8(vMaxScore, vH);
			
			// Save vH values.
			pvHStore[j] = vH;
			
			// Update vE value.
			vH = _mm_subs_epu8(vH, vInserB); // saturation arithmetic, result >= 0
			pvE[j] = _mm_subs_epu8(pvE[j], vInserE);
			pvE[j] = _mm_max_epu8(pvE[j], vH);
			
			// Update vF value.
			vF = _mm_subs_epu8(vF, vDeletE);
			vF = _mm_max_epu8(vF, vH);
			
			// Load the next vH.
			vH = pvHLoad[j];
		}
		
		// loop for tracing the beginning point of the best alignment
		vTemp = _mm_cmpeq_epi8(vMaxMark, vMaxScore);
		unsigned int cmp = _mm_movemask_epi8(vTemp);
		if (cmp != 0x0000) { // The max score apears in the vMaxScore.
			unsigned int j = segLen - 1;
			while (cmp == 0x0000) {
				vTemp = _mm_cmpeq_epi8(vMaxMark, pvHStore[j]);
				cmp = _mm_movemask_epi8(vTemp);
				j --;
			}
			unsigned int p = 0;
			findMax(pvHStore[j + 1], &p);
			*begin_read = p * segLen + j + 1;
			*begin_ref = i + 1;
			break;
		}
		
		// Lazy_F loop
		
		// The computed vF value is for the given column. Since we are at the end, we need to shift the vF value over to the next column.
		vF = _mm_srli_si128 (vF, 1);
		
		// Correct the vH values until there are no element in vF that could influence the vH values.
		int j = segLen - 1;
		vTemp = _mm_subs_epu8(pvHStore[j], vDeletB);
		vTemp = _mm_subs_epu8(vF, vTemp);
		vTemp = _mm_cmpeq_epi8(vTemp, vZero); // result >= 0
		cmp = _mm_movemask_epi8(vTemp);
		
		while (cmp != 0xffff) {
			pvHStore[j] = _mm_max_epu8(pvHStore[j], vF);
			
			// Update vE incase the new vH value would change it.
			vH = _mm_subs_epu8(pvHStore[j], vInserB);
			pvE[j] = _mm_max_epu8(pvE[j], vH);
			
			// Update vF value.
			vF = _mm_subs_epu8(vF, vDeletE);
			
			j --;
			if (j < 0) {
				j = segLen - 1;
				vF = _mm_srli_si128 (vF, 1);
			}
			vTemp = _mm_subs_epu8(pvHStore[j], vDeletB);
			vTemp = _mm_subs_epu8(vF, vTemp);
			vTemp = _mm_cmpeq_epi8(vTemp, vZero); // result >= 0
			cmp = _mm_movemask_epi8(vTemp);
		}
	}
	
	free(pvHStore);
}
*/