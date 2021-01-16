char* strcat(char *dst, const char *src)
{
    if(dst==0 || src==0) return 0;
    char *temp = dst;
    while (*temp != '\0')
        temp++;
    while ((*temp++ = *src++) != '\0');

    return dst;
}

//added by ran
char* strncpy(char * dest, const char *src, int n)
{
	if ((dest == 0) || (src == 0)) { /* for robustness */
		return dest;
	}
    int i;
    for (i = 0; i < n; i++)
    {
        dest[i] = src[i];
        if (!src[i])
        {
            break;
        }
    }
    return dest;
}

//added by ran
int strncmp(const char *s1, const char *s2, int n)
{
    if (!s1 || !s2)
    {
        return s1 - s2;
    }
    int i;
    for (i = 0; i < n; i++)
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
    }
    return 0;
}