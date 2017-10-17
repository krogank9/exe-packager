
// the packager will output a modified version of this file
//  with the files, directories, & file contents in base64
//  stored in c arrays to be compiled by tiny c compiler

const static char *miniz_license = "Copyright 2013-2014 RAD Game Tools and Valve Software\
Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC\n\
All Rights Reserved.\n\
\n\n\
Permission is hereby granted, free of charge, to any person obtaining a copy\
of this software and associated documentation files (the \"Software\"), to deal\
in the Software without restriction, including without limitation the rights\
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\
copies of the Software, and to permit persons to whom the Software is\
furnished to do so, subject to the following conditions:\
\n\n\
The above copyright notice and this permission notice shall be included in\
all copies or substantial portions of the Software.\
\n\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\
THE SOFTWARE.\n";

#include <stdio.h>
#ifdef _WIN32
    #include "windows.h"
#else
    #include <sys/stat.h>
#endif

unsigned int b64_int(unsigned int ch) {
    if (ch==43) return 62;
    if (ch==47) return 63;
    if (ch==61) return 64;
    if ((ch>47) && (ch<58)) return ch + 4;
    if ((ch>64) && (ch<91)) return ch - 'A';
    if ((ch>96) && (ch<123)) return (ch - 'a') + 26;
    return 0;
}

unsigned int b64_decode(const unsigned char in[], unsigned char out[]) {

    unsigned int i=0, j=0, k=0, s[4];

    for (i=0;in[i] != '\0';i++) {
        s[j++]=b64_int(*(in+i));
        if (j==4) {
            out[k+0] = (s[0]<<2)+((s[1]&0x30)>>4);
            if (s[2]!=64) {
                out[k+1] = ((s[1]&0x0F)<<4)+((s[2]&0x3C)>>2);
                if ((s[3]!=64)) {
                    out[k+2] = ((s[2]&0x03)<<6)+(s[3]); k+=3;
                } else {
                    k+=2;
                }
            } else {
                k+=1;
            }
            j=0;
        }
    }

    return k;
}


// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

#ifdef _WIN32
    void runCmd(char *cmd) {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        CreateProcess(cmd, "", NULL,NULL,FALSE, CREATE_NEW_CONSOLE, NULL,NULL, &si, &pi);
    }
#endif

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdSHow)  {
#else
int main () {
#endif

    printf(miniz_license);
    for(int i=0; i<numDirs; i++ ) {
#ifdef _WIN32
        char *dName = str_replace(dirsToMake[i], "%USERPROFILE%", getenv("USERPROFILE"));
        if(dName == NULL) {
            printf("Error getting dir absolute path\n");
            return 1;
        }
        CreateDirectory(dName, NULL);
        free(dName);
#else
        mkdir(dirsToMake[i], S_IRWXU);
#endif
    }

    for(int i=0; i<fileCount; i++) {
        FILE *f;
        char *fName = str_replace(fileNames[i], "%USERPROFILE%", getenv("USERPROFILE"));
        if(fName == NULL) {
            printf("Error getting file absolute path\n");
            return 1;
        }

        f = fopen(fName, "rb+");
        if(f == NULL) // if file does not exist, create it
        {
            f = fopen(fName, "wb");
            if (f == NULL)
            {
                printf("Error opening file %s\n", fileNames[i]);
                return 1;
            }
        }
        else {
            fclose(f);
            continue; // skip file if already created
        }

        unsigned char *unbased = malloc(fileSizesUncompressed[i]);
        b64_decode(fileContentsB64[i], unbased);

        unsigned char *uncompressed = malloc(fileSizesUncompressed[i]);
        unsigned long uncomp_len = (unsigned long) fileSizesUncompressed[i];
        uncompress(uncompressed, &uncomp_len, unbased, strlen(fileContentsB64[i]));

        fwrite(uncompressed, sizeof(char), fileSizesUncompressed[i], f);

        free(unbased);
        free(uncompressed);
        free(fName);

        fclose(f);
    }

#ifdef _WIN32
    char cmd[9999];
    sprintf(cmd, "%s%s", baseDir, exeName);
    char *pName = str_replace(cmd, "%USERPROFILE%", getenv("USERPROFILE"));
    printf("%s\n", pName);
    runCmd(pName);
    free(pName);
#else
    char linux_perms_cmd[9999];
    asprintf(&linux_perms_cmd, "cd %s && chmod +x %s && ./%s", baseDir, exeName, exeName);
    system(linux_perms_cmd);
#endif
}
