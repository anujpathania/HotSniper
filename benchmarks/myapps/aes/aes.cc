#include <openssl/aes.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <random>
#include "sim_api.h"


using namespace std;

std::random_device rd;  
std::mt19937 gen(rd());


// std::string  get16BRandomString() {
//   std::string tmp ("78765634sbast3w");
//   std::uniform_int_distribution<> dchar(0, 255);
//     for(int i = 0; i < 16;  i++){
//       tmp[i] = (unsigned char)(33 + (dchar(gen)%92));
//     }
//   return tmp;
// }


int main(int argc, char* argv[]){
    SimSetSecure();

    const unsigned char userkey[16] = "0123456789abcdf";
    int outcounter = 0;

    unsigned char * out = (unsigned char *)malloc(16*sizeof(unsigned char));
    unsigned char * in =  (unsigned char *)malloc(16*sizeof(unsigned char));   
    //mem allocation for inputs
   
    // unsigned char ** big_ins = (unsigned char **) malloc(LINES*sizeof(unsigned char *)); 
    // for(int i = 0; i < LINES; i++)
    //   big_ins[i] = (unsigned char *)(malloc (16*sizeof(unsigned char)));

    int num_encrypts;

    if (argc > 1)
      num_encrypts = atoi(argv[1]);
    else
      num_encrypts = 400;
   
    char ch;
    FILE *fp;
      
    fp = fopen("myapps/tools/plaintext", "r"); // read mode
    if (fp == NULL){
          perror("Error while opening the file.\n");
          exit(EXIT_FAILURE);
    }
    SimRoiStart(); 
    for (size_t i = 0; i < num_encrypts; i++){
        ch = fgetc(fp);
        if (ch == EOF) {
          fclose(fp);
          fp = fopen("myapps/tools/plaintext", "r");
          cout << "Starting again" <<endl;
          ch = fgetc(fp);
        }
        in[0] = ch;
        for (int c = 1; c < 16; c++) {
          in[c] = ch;
          ch = fgetc(fp);
        }
      
      // set KEY
      AES_KEY *newkey = new AES_KEY ;
      newkey->rounds = 10;
      int retval = AES_set_encrypt_key(userkey, 128, newkey);
      
      // encrypt
      AES_encrypt(in, out, newkey);
     
      outcounter +=(int)out[0];
      // if ((outcounter/100 > 0) && (outcounter%100 == 0))
      //   cout <<outcounter<<":"<<in<<endl;

      delete(newkey);
    }
  fclose(fp);
  cout <<outcounter<<endl;
  SimRoiEnd();

  return outcounter; 
}


