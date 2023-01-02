//
// Created by 后藤一里 on 2023/1/1.
//

#ifndef DB5_MAIN_H
#define DB5_MAIN_H

typedef struct tuple {
    int a, b;
} Tuple;

// 工具函数
int StringToInt(const unsigned char*);
Tuple Get(const unsigned char*, int);
void IntToString(int a, unsigned char *ret);
void TupleSet(unsigned char *block, int index, Tuple t);
void Write(unsigned char *resBlock, Buffer *buf, int *output);
void Sort(int start, int end, Buffer *buf);
void TupleWrite(unsigned char *resBlock, int *index, Tuple t, Buffer *buf, int *resBlockId);

// 主要函数
void Task1(int start, int end, int key, int resBlockId);
void Task2(int start, int end, int resBlockId);
void Task3(int target);
void Task4(int resBlockId);
void Task5(int resBlockId);

#endif //DB5_MAIN_H
