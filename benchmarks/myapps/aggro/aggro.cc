#include <openssl/aes.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <random>
#include <chrono>
#include "sim_api.h"

#define LINES 8*16*1024

using namespace std;
using namespace std::chrono;

std::random_device rd;  
std::mt19937 gen(rd());


void get16BRandomString(unsigned char * tmp) {
  std::uniform_int_distribution<> dchar(0, 255);
    for(int i = 0; i < 16;  i++){
      tmp[i] = (unsigned char)(33 + (dchar(gen)%92));
    }
}


int main(int argc, char* argv[]){
    

    std::uniform_int_distribution<> distrib(0, LINES-1);


    const unsigned char userkey[16] = "0123456789abcdf";
    int outcounter = 0;

    unsigned char * out = (unsigned char *)malloc(16*sizeof(unsigned char));
    unsigned char * in =  (unsigned char *)malloc(16*sizeof(unsigned char));   
    //mem allocation for inputs
   
    SimRoiStart();
    double ** big_ins = (double **) malloc(LINES*sizeof(double *)); 
   while (1) {
    for(int i = 0; i < LINES; i++)
      big_ins[i] = (double *)(malloc (128*sizeof(double)));

    }
    long int lcount = LINES - 1;
  
    //Generating random input 
   
    // while (1) {
    //   if (lcount == 0) lcount = LINES - 1;
    //   get16BRandomString(big_ins[lcount]);
    //   lcount--;
    // }
    SimRoiEnd();

  //   int num_encrypts;
  //   if (argc > 1)
  //     num_encrypts = atoi(argv[1]);
  //   else
  //     num_encrypts = 400;
   
  //   int j = 0;
  //   //SimRoiStart(); 
  //   auto start = high_resolution_clock::now();
  //   int line_number = 0; 
  //   for(int i = 0; i < num_encrypts; i++) {
  //     //line_number=distrib(gen);
  //     //cout <<"Linea "<<in<<endl;
      
  //     // set KEY
  //     AES_KEY *newkey = new AES_KEY ;
  //     newkey->rounds = 10;
  //     int retval = AES_set_encrypt_key(userkey, 128, newkey);
      
  //     // encrypt
  //     AES_encrypt(big_ins[line_number], out, newkey);

  //     outcounter += (int)out[0];
  //     line_number = (line_number +2*256 +1) % LINES;
  //     delete(newkey);
  //   }

  // cout <<outcounter<<endl;
  // //SimRoiEnd();
  // auto stop = high_resolution_clock::now(); 
  // auto duration = duration_cast<microseconds>(stop - start);
  

  // cout <<"Duration (us): " <<duration.count() << endl;

  return lcount; 
}


