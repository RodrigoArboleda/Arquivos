#ifndef READ_WRITE_H
#define READ_WRITE_H

/* Victor Giovannoni 10786159 */

void write_file(char *filename);

void read_binary_file(char *filename);

void search_binary_file(char *filename);

void remove_records(char *filename);

void insert_records(char *filename);

void update_records(char *filename);

void sort_file(char *filename);

void merge_files(char *filename);

void intersect_files(char *filename);

void create_index_file(char *filename);

#endif
