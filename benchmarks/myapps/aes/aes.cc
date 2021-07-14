#include <openssl/aes.h>
#include <iostream>
#include <stdlib.h>
#include "sim_api.h"


/* void AES_encrypt(const unsigned char *in, unsigned char *out,
                 const AES_KEY *key);
void AES_decrypt(const unsigned char *in, unsigned char *out,
                 const AES_KEY *key); */


using namespace std;
int main(int argc, char* argv[]){
    SimSetSecure();
    const unsigned char in[16] = "78765634sbast3w";
    const unsigned char userkey[16] = "0123456789abcdf";

    unsigned char * out = (unsigned char *)malloc(16*sizeof(unsigned char));  
    int num_encrypts;

    if (argc > 1)
      num_encrypts = atoi(argv[1]);
    else
      num_encrypts = 400;
   
    SimRoiStart();  
    
    for (size_t i = 0; i < num_encrypts; i++){
        //set KEY
        AES_KEY *newkey = new AES_KEY ;
        newkey->rounds = 10;
        int retval = AES_set_encrypt_key(userkey, 128, newkey);
        //encrypt
        AES_encrypt(in, out, newkey);
        delete(newkey);
    }
   /*  int retval = AES_set_encrypt_key(userkey, 128, newkey);
    AES_encrypt(in, out, newkey);

    retval = AES_set_decrypt_key(userkey, 128, newkey);

    unsigned char * newout = (unsigned char *)malloc(16*sizeof(unsigned char));

    AES_decrypt(out, newout, newkey); 
    for (size_t i = 0; i < 5; i++)
    {
       cout<<newout[i]<<endl;
    }
    
    delete(newkey); */
    SimRoiEnd();
return (int) out[15]; 
}


