#include "read_write.h"
#include <stdio.h>

/* Victor Giovannoni 10786159 */

int main(){
    int option;
    char filename[30];

    scanf("%d", &option);
    scanf("%s", filename);

    switch (option){
        case 1:
            write_file(filename);
            break;
        case 2:
            read_binary_file(filename);
            break;
        case 3:
            search_binary_file(filename);
            break;
        case 4:
            remove_records(filename);
            break;
        case 5:
            insert_records(filename);
            break;
        case 6:
            update_records(filename);
            break;
        case 7:
            sort_file(filename);
            break;
        case 8:
            merge_files(filename);
            break;
        case 9:
            intersect_files(filename);
            break;
        case 10:
            create_index(filename);
    }
}
