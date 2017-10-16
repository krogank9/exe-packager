
// the packager will output a modified version of this file
//  with the files, directories, & file contents in base64
//  stored in c arrays to be compiled by tiny c compiler

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

int main () {
    for(int i=0; i<numDirs; i++ ) {
        // todo make platform agnostic
#ifdef _WIN32
        char cmd[999];
        sprintf(cmd, "mkdir %s", dirsToMake[i]);
        system(cmd);
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
    char win_cmd[9999];
    sprintf(win_cmd, "cd %s && %s", baseDir, exeName);
    system(win_cmd);
#else
    char linux_perms_cmd[9999];
    asprintf(&linux_perms_cmd, "cd %s && chmod +x %s && ./%s", baseDir, exeName, exeName);
    system(linux_perms_cmd);
#endif
}
