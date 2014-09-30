//----------Constants---------
#define HASH_BYTES 20
#define HASH_HEX_BITS 41

//----------Globals---------
struct cmp_key { // comparator used for identifying keys
	bool operator()(const char *first, const char *second) {
		return memcmp(first, second, sizeof(first)) < 0;
	}
};

typedef map<const char*, const char*, cmp_key> hashmap;

//****************Function Declarations*******************
void insertInMap(hashmap* myMap, char *hexHashKey, const char *data);
bool isPresentInMap(hashmap myMap, char *key);
const char* getFromMap(hashmap myMap, const char *key);
void printHashKey(unsigned char* key, int len);
unsigned int data2hexHash(const char* dataToHash, char* hexHash);
void getHashInHex(unsigned char* key, char* tempValue, int len);
int convert(char item);
void hexAddition(char* hexOne, char* hexTwo, char* hexSum, int len);
unsigned int hash(const char *mode, const char* dataToHash,
		unsigned char* outHash);

//****************Function Definitions*******************
//Used to insert entries in fingerTable
void insertInMap(hashmap* myMap, char *hexHashKey, const char *data) {
	(*myMap).insert(hashmap::value_type(hexHashKey, data));
}

bool isPresentInMap(hashmap myMap, char *key) {
	hashmap::iterator iter = myMap.find(key);
	if (iter != myMap.end()) {
		return true;
	}
	return false;
}

//use this function to getFromMap only if 'isPresentInMap == true'
const char* getFromMap(hashmap myMap, const char *key) {
	hashmap::iterator iter = myMap.find(key);
	return (*iter).second;
}

//prints the hash key in readable format, (requires len)
void printHashKey(unsigned char* key, int len) {
	for (int i = 0; i < len; i++) {
		printf("%02x", key[i]);
	}
}

unsigned int data2hexHash(const char* dataToHash, char* hexHash) {
	unsigned char outHash[HASH_BYTES];

	int len = hash("SHA1", dataToHash, outHash);
	if (len == -1) {
		return len;
	}
	getHashInHex(outHash, hexHash, HASH_BYTES);
	return strlen(hexHash);
}

//convert from unsigned char* 20 bytes to char* 40 hex digits
void getHashInHex(unsigned char* key, char* tempValue, int len) {
	for (int i = 0; i < len; ++i)
		sprintf(tempValue + 2 * i, "%02x", (unsigned char) key[i]);
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
	return (int) (item - 48);
}

//adds two hashes in hex and stores result in third
void hexAddition(char* hexOne, char* hexTwo, char* hexSum, int len) {
	char hexArr[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a',
			'b', 'c', 'd', 'e', 'f' };
	int carry = 0;
	int temp, i;
	for (i = len - 1; i >= 0; i--) {
		// convert to decimal and add both array values
		temp = convert(hexOne[i]) + convert(hexTwo[i]) + carry;
		// add values and if they are greater than F add 1 to next value
		carry = temp / 16;
		temp %= 16;
		hexSum[i] = hexArr[temp];
	}
	hexSum[len] = '\0';
}

//hash of the given data in given mode, hash is stored in outHash
unsigned int hash(const char *mode, const char* dataToHash,
		unsigned char* outHash) {
	unsigned int md_len = -1;
	size_t dataSize = strlen(dataToHash);
	OpenSSL_add_all_digests();
	const EVP_MD *md = EVP_get_digestbyname(mode);
	if (NULL != md) {
		EVP_MD_CTX mdctx;
		EVP_MD_CTX_init(&mdctx);
		EVP_DigestInit_ex(&mdctx, md, NULL);
		EVP_DigestUpdate(&mdctx, dataToHash, dataSize);
		EVP_DigestFinal_ex(&mdctx, outHash, &md_len);
		EVP_MD_CTX_cleanup(&mdctx);
	}
	return md_len;
}

bool keyBelongCheck(char startKey[HASH_HEX_BITS], char endKey[HASH_HEX_BITS],
		char searchKey[HASH_HEX_BITS]) {
	char min[] = "0000000000000000000000000000000000000000";
	char max[] = "ffffffffffffffffffffffffffffffffffffffff";

	if (strcmp(startKey, endKey) < 0) {
		if (strcmp(searchKey, startKey) > 0 && strcmp(searchKey, endKey) < 0)
			return true;
		else
			return false;
	} else //that means either startKey==endKey or  startKey > endKey
	{
		bool ans1 = false;
		bool ans2 = false;
		//case 1
		if (strcmp(startKey, max) < 0) {
			if (strcmp(searchKey, startKey) > 0 && strcmp(searchKey, max) <= 0)
				ans1 = true;
		}
		//case 2
		if (strcmp(min, endKey) < 0) {
			if (strcmp(searchKey, min) >= 0 && strcmp(searchKey, endKey) < 0)
				ans2 = true;
		}
		return ans1 || ans2;
	}
}
