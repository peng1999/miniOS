
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            string.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

PUBLIC	void*	memcpy(void* p_dst, void* p_src, int size);
PUBLIC	void	memset(void* p_dst, char ch, int size);
PUBLIC	char*	strcpy(char* p_dst, char* p_src);

//added by zcr
PUBLIC	int	strlen(char* p_str);
#define	phys_copy	memcpy
#define	phys_set	memset

//added by ran
char* strncpy(char *dest, const char *src, int n);
int strncmp(const char *s1, const char *s2, int n);