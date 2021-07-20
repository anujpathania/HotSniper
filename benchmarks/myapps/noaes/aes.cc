#include <openssl/aes.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "sim_api.h"

using namespace std;
int main(int argc, char* argv[]){
    std::ifstream file("plaintext");
    // const unsigned char in[16] = "78765634sbast3w";
    const unsigned char userkey[16] = "0123456789abcdf";
    int outcounter = 0;

    unsigned char * out =  (unsigned char *)malloc(16*sizeof(unsigned char));  
     unsigned char * in =  (unsigned char *)malloc(16*sizeof(unsigned char));   
    int num_encrypts;

    if (argc > 1)
      num_encrypts = atoi(argv[1]);
    else
      num_encrypts = 400;
   
    SimRoiStart();  
    
    for (size_t i = 0; i < num_encrypts; i++){
        // Get plain text from file
        if (file.is_open()) {
            std::string line;
            std::getline(file, line);
            std::copy(line.begin(), line.end(), in);
        }

        //set KEY
        AES_KEY *newkey = new AES_KEY ;
        newkey->rounds = 10;
        int retval = AES_set_encrypt_key(userkey, 128, newkey);
        //encrypt
        AES_encrypt(in, out, newkey);
        outcounter += out[0];
        delete(newkey);
    }
    file.close();
    cout <<outcounter<<endl;
    SimRoiEnd();
return outcounter; 
}


