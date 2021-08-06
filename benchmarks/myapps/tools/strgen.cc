/* Generates a 16Mb file with random readable ASCII characters*/
#include<iostream>
#include <stdlib.h>
#include<random>

#define FSIZE 1024*1024
using namespace std;

std::random_device rd;  
std::mt19937 gen(rd());

void get16BRandomString(unsigned char * tmp) {
    std::uniform_int_distribution<> dchar(0, 255);
    for(int i = 0; i < 16;  i++){
      tmp[i] = (unsigned char)(33 + (dchar(gen)%92));
    }
}

int main() {
    char ch;
    FILE *fp;
      
    fp = fopen("plaintext", "w"); 
    if (fp)
        for(int i = 0; i < FSIZE; i++){
            unsigned char * tmp = (unsigned char*)malloc(16*sizeof (unsigned char));
            get16BRandomString(tmp);
            for(int j = 0; j < 16; j++)
                fputc (tmp[j] , fp);
        }
    fclose(fp);
    return 1;
}
