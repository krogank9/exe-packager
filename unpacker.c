
// the packager will output a modified version of this file
//  with the files, directories, & file contents in base64
//  stored in c arrays to be compiled by tiny c compiler

#include <stdio.h>
#include <sys/stat.h>

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


int main () {
    for(int i=0; i<numDirs; i++ ) {
        // todo make platform agnostic
        mkdir(dirsToMake[i], S_IRWXU);
    }

    for(int i=0; i<fileCount; i++) {
        FILE *f;
        f = fopen(fileNames[i], "rb+");
        if(f == NULL) // if file does not exist, create it
        {
            f = fopen(fileNames[i], "wb");
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

        fclose(f);
    }

    char *chmod = "chmod +x ";
    char linux_perms_cmd[strlen(chmod) + strlen(exeName)];
    asprintf(&linux_perms_cmd, "cd %s && chmod +x %s && ./%s", baseDir, exeName, exeName);
    system(linux_perms_cmd);
}
