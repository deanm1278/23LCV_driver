/*
 * Userspace API for spi ram device
 *
 * Copyright (C) 2016 Dean Miller <deanm1278@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "lib23lcv.h"
#include <iostream>
#include <fstream>

using namespace std;

R_23LCV::R_23LCV(string path){
    this->path = path;
}

int R_23LCV::read(uint16_t addr, void *buf, size_t size){
   ofstream sz, ad;
   //set read size
   sz.open((path + "size").c_str());
   if (!sz.is_open()){
    perror("R_23LCV: failed to set read size ");
    return -1;
   }
   sz << (uint16_t)size;
   sz.close();
   
   //set address
   ad.open((path + "addr").c_str());
   if (!ad.is_open()){
    perror("R_23LCV: failed to set read address");
    return -1;
   }
   ad << hex << addr;
   ad.close();
   
   ifstream is((path + "data").c_str(), std::ios::binary);
   is.read((char *)buf, size);
   
   return 0;
}

int R_23LCV::write(uint16_t addr, void *buf, size_t size){
   ofstream fs, ad;
   
   //set address
   ad.open((path + "addr").c_str()); 
   if (!ad.is_open()){
    perror("R_23LCV: failed to set read address");
    return -1;
   }
   ad << hex << addr;
   ad.close();
   
   //write the data!
   fs.open((path + "data").c_str());
   if (!fs.is_open()){
    perror("R_23LCV: write failed to open file ");
    return -1;
   }
   for(int i = 0; i < size; i++)
   {
      fs << ((unsigned char *)buf)[i]; 
   }
   fs.close();
   return 0;
}

int R_23LCV::listen(bool state){
    ofstream fs;
   
   //set address
   fs.open((path + "listen").c_str());
   if (!fs.is_open()){
    perror("R_23LCV: failed to start listen");
    return -1;
   }
   fs << state;
   fs.close();
}

R_23LCV::~R_23LCV(){
    
}
