//----------Constants---------
#define IP_SIZE 40

//****************Function Declarations*******************
void intToChar(int intToChng, char* charToRet);
void tab(int count);
bool startsWith(const char *a, const char *b);
char* substring(char *string, int position, int length);
int countOccurence(char* string, char splitter);
void split(char* string, char splitter, char splittedArr[][IP_SIZE]);
int indexOf(char* string, char of);

//****************Function Definitions*******************
void intToChar(int intToChng, char* charToRet) {
	snprintf(charToRet, sizeof(charToRet), "%d", intToChng);
}

void tab(int count) {
	for (int i = 0; i < count; i++) {
		cout << "\t";
	}
}

bool startsWith(const char *a, const char *b) {
	if (strncmp(a, b, strlen(b)) == 0) {
		return 0;
	}
	return 1;
}

char* substring(char *string, int position, int length) {
	char *pointer;
	int c;

	pointer = (char*) malloc(length + 1);

	if (pointer == NULL) {
		printf("Inside substring : Unable to allocate memory.\n");
		exit( EXIT_FAILURE);
	}

	for (c = 0; c < position - 1; c++) {
		string++;
	}

	for (c = 0; c < length; c++) {
		*(pointer + c) = *string;
		string++;
	}

	*(pointer + c) = '\0';
	return pointer;
}

int countOccurence(char* string, char splitter) {
	int len = strlen(string);
	int count = 0;
	for (int i = 0; i < len; i++) {
		if (string[i] == splitter) {
			count++;
		}
	}
	return count;
}

void split(char* string, char splitter, char splittedArr[][160]) {
	int len = strlen(string);
	char tmp[len];
	memset(tmp,'\0',strlen(tmp));
	int j = 0; //pointer for tmp string
	int k = 0; //pointer for strings in splittedArr
	for (int i = 0; i < len; i++) {
		if (string[i] != splitter) {
			tmp[j] = string[i];
			j++;
		} else {
			tmp[j] = '\0';
			strcpy(splittedArr[k], tmp);
			j = 0;
			memset(tmp,'\0',strlen(tmp));
			k++;
		}
	}
	if (string[len - 1] != splitter) { //Adding this check to get the last array
		tmp[j] = '\0';
		strcpy(splittedArr[k], tmp);
	}
}

int indexOf(char* string, char of) {
	int len = strlen(string);
	for (int i = 0; i < len; i++) {
		if (string[i] == of) {
			return i;
		}
	}
	return len - 1;
}
