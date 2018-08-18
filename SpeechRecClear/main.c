#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "prim_type.h"

/**
 * словарь обозначений в коде:
 * ad - audio
 */

/*
 * Continuous recognition from a file
 */
static void recognise_from_file() {
    int16 adbuf[2048];	//буфер для волны
    const char* fname;	//имя файла из которого распознавать
    const char* hyp;	//гипотеза распознавания из файла
	FILE* rawf;			//файл, из которого надо распознавать

	fname = "../test.txt";
	
    if ((rawf = fopen(fname, "rb")) == NULL) { //открываем для чтения в бинарном виде
        E_FATAL_SYSTEM("Failed to open file '%s' for reading", fname);
	}
}

/*
 * 
 */
int main() {
	recognise_from_file();
	return 0;
}