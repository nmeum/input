typedef void (*compfn)(const char *, size_t);

void addcomp(char *);
void initcomp(compfn, int);
unsigned char complete(EditLine *, int);
