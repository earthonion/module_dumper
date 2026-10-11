#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <ps5/kernel.h>
#include "util.h"
#include "self.h"


int strendswith(char* str, char* suffix) {
      
        size_t len = strlen(str);
        size_t sufflen = strlen(suffix);

        if(len < sufflen) {return 0;}

        for(int i = 0; i <= sufflen; i++){
            //printf("%c == %c ?\n",str[len-i], suffix[sufflen-i]);
            if (str[len-i] != suffix[sufflen-i]){
                    
                return 0;
            }
        }
        
        return 1;
}

// Source - https://stackoverflow.com/a/9210960
// Posted by Yaroslav Stavnichiy, modified by community. See post 'Timeline' for change history
// Retrieved 2026-10-09, License - CC BY-SA 4.0

int mkpath(char* file_path, mode_t mode) {
    assert(file_path && *file_path);
    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}




int main() {
    
    char* paths[] = {"/", "/system_ex/common_ex/lib/","/system/common/lib/", "/system/priv/lib/", "/system/vsh/", NULL};
    nlog("Module Dumper");
    
    uint16_t fw =  kernel_get_fw_version() >> 16;
    printf("Firmware Version: %04x\n", fw);
    
    //~ uint8_t privcaps[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                            //~ 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    //~ pid_t mypid = getpid();
    //~ uint8_t caps[16];
    //~ uint64_t authid = kernel_get_ucred_authid(mypid);
    //~ kernel_get_ucred_caps(mypid, caps);
    //~ kernel_set_ucred_authid(mypid, 0x4800000000000007L);
    //~ kernel_set_ucred_caps(mypid, privcaps);
    
    int i = 0;
    while(paths[i]){
        
        
        
        DIR *dir = opendir(paths[i]);
    
        struct dirent *entry;
        while((entry = readdir(dir)) != NULL) {
            
            //printf("%d", strendswith("app",".prx"));
            
            if(strendswith(entry->d_name, ".prx") 
            || strendswith(entry->d_name, ".sprx") 
            || strendswith(entry->d_name, ".elf") 
            || strendswith(entry->d_name, ".self") 
            || strendswith(entry->d_name, ".sbin")){
                
                //strcmp return 0 if equal
                if (strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")) {
                    int src_path_sz = snprintf(NULL, 0, "%s%s", paths[i], entry->d_name);
                    char src_path[src_path_sz + 1];
                    snprintf(src_path, src_path_sz+1, "%s%s", paths[i], entry->d_name);
                    
                    int dest_path_sz = snprintf(NULL, 0, "/mnt/usb0/modules/%04x%s%s", fw, paths[i], entry->d_name);
                    char dest_path[dest_path_sz + 1];
                    snprintf(dest_path, dest_path_sz+1 , "/mnt/usb0/modules/%04x%s%s", fw , paths[i], entry->d_name);
                    
                    
                    //create folders on usb
                    mkpath(dest_path, 0777);
                    
                    printf("src: %s\ndest: %s\n--------------------------\n", src_path, dest_path);
                    
                    FILE *src_file = fopen(src_path, "rb");
                    if(src_file == NULL){
                        printf("error opening src_file: %s\n" , src_path);
                        perror("fopen");
                    }
                    
                    //get file size
                    fseek(src_file, 0 , SEEK_END);
                    long src_flen = ftell(src_file);
                    rewind(src_file);
                    
                    printf("size (bytes): %lu\n", src_flen);
                    
                    
                    FILE *dest_file = fopen(dest_path, "wb");
                    if(dest_file == NULL){
                        printf("error opening dest_file: %s\n" , dest_path);
                        perror("fopen");
                    }
                    
                    
                    
                    if(self_is_valid(src_path)){

                        self_extract_elf(fileno(src_file), fileno(dest_file));

                    }
                    else {
                        //copy into buffer
                        uint8_t *fbuf = malloc(src_flen * sizeof(uint8_t));
                        fread(fbuf, 1, src_flen, src_file);
                        
                        //write buffer contents to destination
                        fwrite(fbuf, src_flen, 1, dest_file);
                        free(fbuf);
                        fbuf = NULL;
                    }
                    fclose(src_file);
                    fclose(dest_file);
                    
                }
            }
            
            
            
            //printf("%s\n",entry->d_name);
        }
        closedir(dir);
        //printf("%s\n", paths[i]);
        
        i++;    
    }
    
    //~ kernel_set_ucred_authid(mypid, authid);
    //~ kernel_set_ucred_caps(mypid, caps);
    
    nlog("Done!");
    return 0;
    
}
