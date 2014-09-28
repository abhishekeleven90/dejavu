#include <openssl/evp.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <map>

using namespace std;


#define M 2
#define M1 3

/**
* comparator used for identifying keys
* uses memcmp
*/
struct cmp_key
{
    bool operator()(char *first, char  *second)
    {
        return memcmp(first, second, sizeof(first)) < 0;
    }
};

/**
* generic data structure for hashtable/hashmap
* key is unsigned char*, value is string, uses comparator cmp_key 
*/
typedef map<char*, char*, cmp_key> hashmap;


/**
Full Usage:
hashmap myMap;
char in_data[] = "test";
char hashkeyhex[41]; //to-do constant change
hashToHex(in_data, hashkeyhex, strlen(in_data));
cout<<hashkeyhex<<endl;
insertInMap(&myMap, hashkeyhex, in_data);
*/
void insertInMap(hashmap* myMap, char *hashkeyhex, char *data)
{
    (*myMap).insert(hashmap::value_type(hashkeyhex, data));
}

/**
*search in a given map, with a key
*/
bool searchInMap(hashmap myMap, char *key)
{
    hashmap::iterator iter = myMap.find(key);
    if (iter != myMap.end())
        return true;
    return false;
}

/**
* call only after searchInMap() 
Usage:
if(searchInMap(myMap, hashkeyhex))
{
    cout << getFromMap(myMap, hashkeyhex2)<< endl;
}
*/
char* getFromMap(hashmap myMap, char *key)
{
    hashmap::iterator iter = myMap.find(key);
    return (*iter).second;
}

/**
* will remove this method after fully satisfied
* prints the hash key in readable format
* requires len, if SHA1 --> len is 20 anyhow
* does not print any new line character
*/
void printHashKey(unsigned char* key, int len) 
{
    for(int i=0; i<len; i++) 
        printf("%02x", key[i]);
     
}

/*
* convert from unsigned char* 20 bytes to char* 40 hex digits
*/
void getHashInHex(unsigned char* key, char* tempValue, int len)
{
    int i;
    for (i = 0; i < len; ++i)
        sprintf(tempValue + 2*i, "%02x", (unsigned char)key[i]);
}


int convert(char item) {
    switch (item) {
        case 'a':
            return 10;
            break;
        case 'b':
            return 11;
            break;
        case 'c':
            return 12;
            break;
        case 'd':
            return 13;
            break;
        case 'e':
            return 14;
            break;
        case 'f':
            return 15;
            break;
    }
    return (int)(item-48);
}

/**
* add two hashes in hex and stores result in third
Usage:
char a[]="6f";
char b[]="ff";
char c[M1];
addition(a,b,c, strlen(a));
cout<<c<<endl;
*/
void addition(char* hexDecOne, char* hexDecTwo, char* hexDecSum, int len) {
    //char* hexDecSum=(char*)malloc(sizeof(char));
    char hexArr[]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int carry=0;
    int temp,i;
    for (i = len-1; i >= 0; i--) {
        // convert to decimal and add both array values
            cout<<"conv1: "<<convert(hexDecOne[i])<<" conv2: "<<convert(hexDecTwo[i])<<endl;
            temp = convert(hexDecOne[i]) + convert(hexDecTwo[i])+carry;
            // add values and if they are greater than F add 1 to next value
            carry=temp/16;
            temp %= 16;
            hexDecSum[i]=hexArr[temp];
            cout<<"araaa: "<<hexDecSum[i]<<endl;
    }
    hexDecSum[len+1]='\0';
    //return hexDecSum;
}


/**
* usage: 
* char* in_data = "hello";
* int length = strlen(in_data);
* unsigned char outHash[20];
* int len = hash("SHA1", in_data, length, outHash);
* check if len returned is not -1
* hash is stored in outHash  
*/
unsigned int hash(const char *mode, const char* dataToHash, size_t dataSize, unsigned char* outHashed) {
    unsigned int md_len = -1;
    OpenSSL_add_all_digests();
    const EVP_MD *md = EVP_get_digestbyname(mode);
    if(NULL != md) {
        EVP_MD_CTX mdctx;
        EVP_MD_CTX_init(&mdctx);
        EVP_DigestInit_ex(&mdctx, md, NULL);
        EVP_DigestUpdate(&mdctx, dataToHash, dataSize);
        EVP_DigestFinal_ex(&mdctx, outHashed, &md_len);
        EVP_MD_CTX_cleanup(&mdctx);
    }
    return md_len;
}

/**
* see insertInMap()
*/
unsigned int hashToHex(const char* dataToHash, char* hashkeyhex, size_t dataSize)
{
    unsigned char outHash[20]; //to do constant change
    int len = hash("SHA1", dataToHash, dataSize, outHash);
    if(len==-1)
        return len;
    getHashInHex(outHash,hashkeyhex,20); //to-do constant change
    return strlen(hashkeyhex);   
}


int main()
{

hashmap myMap;
char in_data[] = "test";
char hashkeyhex[41];
hashToHex(in_data, hashkeyhex, strlen(in_data));
cout<<hashkeyhex<<endl;
insertInMap(&myMap, hashkeyhex, in_data);

//myMap.insert(hashmap::value_type(hashkeyhex, in_data));

if(searchInMap(myMap, hashkeyhex))
{
    cout << getFromMap(myMap, hashkeyhex)<< endl;
}
else
{
    cout<<"not found"<<endl;
}


/*
cout<<"holaa"<<endl;
char a[]="6f";
char b[]="ff";
char c[M1];
//cout<<strlen(a)<<endl;
addition(a,b,c, strlen(a));
cout<<a<<endl;
cout<<c<<endl;
*/
}