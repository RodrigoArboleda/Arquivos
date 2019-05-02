#include "read_write.h"
#include <stdio.h>

/* Victor Giovannoni 10786159 */

int main(){
    int option;
    char filename[100];

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
    }
}
