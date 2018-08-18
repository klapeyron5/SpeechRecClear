#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "prim_type.h"

/**
 * ������� ����������� � ����:
 * ad - audio
 */

/*
 * Continuous recognition from a file
 */
static void recognise_from_file() {
    int16 adbuf[2048];	//����� ��� �����
    const char* fname;	//��� ����� �� �������� ������������
    const char* hyp;	//�������� ������������� �� �����
	FILE* rawf;			//����, �� �������� ���� ������������

	fname = "../test.txt";
	
    if ((rawf = fopen(fname, "rb")) == NULL) { //��������� ��� ������ � �������� ����
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