/*
Copyright (c) 2014, Olaf Schulz
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* 
 * Inline Motion Vector helper structs and functions 
 *
 *
 * */
#ifndef RASPIIMV_H
#define RASPIIMV_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RaspiVid.h"

typedef struct
{
	signed char x_vector;
	signed char y_vector;
	short sad;
} INLINE_MOTION_VECTOR; 

typedef struct 
{
	size_t imv_array_len;
	char *imv_array_in; //target array for video process
	char *imv_array_buffer; // source array for blob detection. Will be swapped with imv_array.
	unsigned char *imv_norm;
	size_t width;
	size_t height;
	char available; // 1 - New data from video process available
	char mutex; // mutex for array pointer swapping
} MOTION_DATA;

extern MOTION_DATA motion_data;// = motion_data_init;

void handle_imv_data(char *data, size_t data_len);

int init_motion_data(MOTION_DATA *md, RASPIVID_STATE *state);
void uninit_motion_data(MOTION_DATA *md, RASPIVID_STATE *state);
void imv_eval_norm(MOTION_DATA *md);



#ifdef __cplusplus
}
#endif

#endif
