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

int copy_file(char *src_path, char *dest_path){
    //open source file
    FILE *src_file = fopen(src_path, "rb");
    if(src_file == NULL){
        printf("error opening src_file: %s\n" , src_path);
        perror("fopen");
        return -1;
    }
    
    //get source file size
    fseek(src_file, 0 , SEEK_END);
    long src_flen = ftell(src_file);
    rewind(src_file);
    
    printf("size (bytes): %lu\n", src_flen);
    
    //open destination path
    FILE *dest_file = fopen(dest_path, "wb");
    if(dest_file == NULL){
        printf("error opening dest_file: %s\n" , dest_path);
        perror("fopen");
        return -1;
    }
    
    //if its a signed elf, decrypt it
    if(self_is_valid(src_path)){
        self_extract_elf(fileno(src_file), fileno(dest_file));
    }
    
    //otherwise just copy it over
    else {
        
        //copy source into buffer
        uint8_t *fbuf = malloc(src_flen * sizeof(uint8_t));
        fread(fbuf, 1, src_flen, src_file);
        
        //write buffer contents to destination
        fwrite(fbuf, src_flen, 1, dest_file);
        
        free(fbuf);
        fbuf = NULL;
    }
    //close files
    fclose(src_file);
    fclose(dest_file);
    return 1;
    
}


//copy all files and all nested folders/files
int copy_recursive(char *src_dir, char *dest_dir) {
    
    DIR *src = opendir(src_dir);
    struct dirent *entry = NULL;
    
    
    while((entry = readdir(src)) != NULL) {
        
        char src_path[PATH_MAX];
        char dest_path[PATH_MAX];

        
        
        
        
        if(strcmp(".", entry->d_name) && strcmp("..", entry->d_name)){
            
            //if the entry is a directory, mkdir at destination and go a level deeper
            if(entry->d_type == DT_DIR){
                
                printf("Copying: %s\n", src_path);
                
                snprintf(src_path, sizeof(src_path), "%s%s/", src_dir, entry->d_name);
                snprintf(dest_path, sizeof(dest_path), "%s%s/", dest_dir, entry->d_name);
                
                mkpath(dest_path, 0755);
                copy_recursive(src_path, dest_path);
            }
            //otherwise copy the file over
            else {
                
                snprintf(src_path, sizeof(src_path), "%s%s", src_dir, entry->d_name);
                snprintf(dest_path, sizeof(dest_path), "%s%s", dest_dir, entry->d_name);
                
                printf("%s\n", src_path);
                
                mkpath(dest_path, 0755);
                copy_file(src_path, dest_path);
            }
        }
        
    
        
    }
    closedir(src);
    return 0;
}



int main() {
    
    char* paths[] = {
        "/system_ex/common_ex/lib/",
        "/system/common/lib/", 
        "/system/priv/lib/", 
        "/system/vsh/", 
        "/system/sys/", 
        "/user/home/", 
         NULL};
         
    nlog("Module Dumper");
    
    uint16_t fw =  kernel_get_fw_version() >> 16;
    printf("Firmware Version: %04x\n", fw);
    
    char *dest_path = NULL;
    asprintf(&dest_path, "/mnt/usb0/PS5/libs/%04x/", fw);
    mkpath(dest_path, 0755);
        
    int i = 0;
    while(paths[i]){
        
        copy_recursive(paths[i], dest_path);
        i++;
    }
    free(dest_path);
    nlog("Done!");
    return 0;
    
}
