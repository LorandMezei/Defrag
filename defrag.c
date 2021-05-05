// Header Files ==========================================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
//=======================================================================================================================^

// Structs ===============================================================================================================
typedef struct binary_file 
{
	char *name;
    char *path_name;
	long offset;

} FRAGMENT;
//=======================================================================================================================^

// Function Declarations =================================================================================================
void * thread_traverse_directory(void *value);
void bubble_sort(FRAGMENT **fragments, int size_of_fragments);
void swap_cards(FRAGMENT **fragments, int fragment1_index, int fragment2_index);
//=======================================================================================================================^

// Global Variables ======================================================================================================
FRAGMENT **pointers_to_fragments;
int fragments_index = 0;

pthread_mutex_t lock; 
//=======================================================================================================================^

// Main ==================================================================================================================
/* 
   Create a thread for each folder at the top of the directory tree. 
   Each thread will recursively traverse its corresponding folder.
   When traversing, collect all of the ##.bin files and add them to an array of structs.
   Sort the array of ##.bin files, and then append them to a music.mp3 file.
*/
int main(int argc, char *argv[]) 
{

    // Initialize mutex lock =====================================
    if (pthread_mutex_init(&lock, NULL) != 0) 
    { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
    //==========================================================-^

    printf("\nEnter the location of the directory tree.");
    printf("\nFor example, something like /home/lorandmezei/a2-LorandMezei/dirs/\n");

    // Directory =================================================
    // Location of top level directory tree, should be given as a command line argument.
    char *directory_path = argv[1];

    char buf[100];
    char buf1[100];

    getcwd(buf, 100);

    char str[80];
    strcpy (str,buf);
    strcpy(buf1, buf);
    strcat(buf1, "/music.mp3");

    printf("%s\n", buf1);

    strcat(str, "/dirs/");


    printf("%s\n", str);

    // Change directory to the location of the top level directory tree. 
    chdir(str); 
    //===========================================================^
    
    // Array of file fragments ===================================
    pointers_to_fragments = malloc(100 * sizeof(FRAGMENT*));
    //===========================================================^
    
    // Threads ===================================================
    //------------------------------
    // Open the directory at path_name.
    DIR *directory = opendir(str);
	
    // Used to store the current entry in the directory.
	struct dirent *entry; 

    // Used to store stats of the current entry in the directory.
	struct stat stats;
    //-----------------------------^

    //------------------------------
    // Array of threads for each of folders/directories at the top of the directory tree.
    pthread_t *threads = malloc(sizeof(pthread_t) * 5);
    int thread_index = 0;
    //-----------------------------^

    // While there are more entries to be read in current directory.
    while ((entry = readdir(directory)) != NULL) 
    {
        // Create an array of threads, and after each one is create()d, then do join() for each thread.

        // To check if entry is a symbolic link.
        lstat(entry->d_name, &stats);
        
        // If entry is a directory.
        if (S_ISDIR(stats.st_mode))
        {  
            // Skip current directory (.) and parent directory (..).
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            //------------------------------
            // Create/Start each thread for each folder at the top of the directory tree.
            char *thread_value = entry->d_name;

            pthread_create(&threads[thread_index], NULL, thread_traverse_directory, thread_value); //<<<<<<<<<<<<<<<<<<<<<<<
            
            thread_index++;
            //-----------------------------^
        }  
    }

    //------------------------------
    // Wait for all threads to finish.
    for (int i = 0; i < 5; i++)
    {
        pthread_join(threads[i], NULL);
    }
    //-----------------------------^

    // Free threads.
    free(threads);

    // Close the directory.
    closedir(directory);
    //===========================================================^

    // Sort array of fragments ===================================
    bubble_sort(pointers_to_fragments, fragments_index);
    //===========================================================^

    // Destroy mutex lock ========================================
    pthread_mutex_destroy(&lock); 
    //===========================================================^

    // BINARY/MP3 FILE ===========================================
    FILE *song_binary = fopen("music.bin", "w");

    for (int i = 0; i < fragments_index; i++) 
    {
        struct stat info;
        char *filename = pointers_to_fragments[i]->path_name;
        stat(filename, &info);

        char *content = malloc(info.st_size);
      
        FILE *file = fopen(pointers_to_fragments[i]->path_name, "r");

        /* Try to read a single block of info.st_size bytes */
        fread(content, info.st_size, 1, file);

        // Write data of current fragment to song.bin.
        fwrite(content, info.st_size, 1, song_binary);

        free(content);

        fclose(file);
    }


    // Rename song file to music.mp3.
    rename("music.bin", buf1);

    fclose(song_binary);
    //===========================================================^

    // Free memory ===============================================
    for (int i = 0; i < fragments_index; i++) 
    {
        free(pointers_to_fragments[i]->name);
        free(pointers_to_fragments[i]->path_name);
        free(pointers_to_fragments[i]);
    }
    free(pointers_to_fragments);
    //===========================================================^
    
    return 0;
}
//=======================================================================================================================^

//========================================================================================================================
/*
    Will recursively traverse each subdirectory in the provided directory.
*/
void * thread_traverse_directory(void *value) 
{
    char *path_name = (char *) value;
    
    //printf("%s\n", path_name);
    
    // Open the directory at path_name.
    DIR *directory = opendir(path_name);
	
    // Used to store the current entry in the directory.
	struct dirent *entry; 

    // Used to store stats of the current entry in the directory.
	struct stat stats;

    // While there are more entries to be read in current directory.
    while ((entry = readdir(directory)) != NULL) 
    {
        lstat(entry->d_name, &stats);
        
        // If entry is a directory.
        if (entry->d_type == DT_DIR) 
        {
            char path[1024];

            // Skip current directory (.) and parent directory (..).
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            snprintf(path, sizeof(path), "%s/%s", path_name, entry->d_name);

            // Recursively call function.
            thread_traverse_directory(path); 
        } 
 
        // If entry is not a directory.
	    else 
        {
            // If entry is a file and not a symbolic link ("dir0").
            if (strcmp("dir0", entry->d_name) != 0) 
            {
                //------------------------------
                // Lock the mutex to prevent all of the threads from 
                // appending to the end of the pointers_to_fragments array.
                pthread_mutex_lock(&lock); 

                char str[80];
                strcpy (str,path_name);
                strcat (str, "/");
                strcat (str, entry->d_name);
                
                //printf("%s\n\n", str);

                pointers_to_fragments[fragments_index] = malloc(sizeof(FRAGMENT));
                pointers_to_fragments[fragments_index]->name = strdup(entry->d_name);
                pointers_to_fragments[fragments_index]->path_name = strdup(str);
                
                fragments_index++; 

                // Unlock the mutex.
                pthread_mutex_unlock(&lock);
                //-----------------------------^
            }
        }
    }

    // Close the directory.
    closedir(directory);

    return NULL;
}
//========================================================================================================================^

//========================================================================================================================
void bubble_sort(FRAGMENT **fragments, int size_of_fragments) {

	for (int i = 0; i < (size_of_fragments - 1); i++) {
		
		for (int j = 0; j < (size_of_fragments - 1); j++) {
			
			if (strcmp(fragments[j]->name, fragments[j + 1]->name) > 0) {

				swap_cards(fragments, j, j + 1);
			}
		}
	}
}

void swap_cards(FRAGMENT **fragments, int fragment1_index, int fragment2_index) {

	FRAGMENT *temp_fragment = fragments[fragment1_index];
	fragments[fragment1_index] = fragments[fragment2_index];
	fragments[fragment2_index] = temp_fragment;
}
//=======================================================================================================================^