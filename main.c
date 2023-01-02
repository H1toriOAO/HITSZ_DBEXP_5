//
// Created by 后藤一里 on 2023/1/1.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "extmem.h"
#include "main.h"

#define MAX_T 1000

/*
 * 将字符串转换为整数
 *
 * @param str 输入的字符串
 *
 * @return 输入字符串对应的整数
 */
int StringToInt(const unsigned char *str)
{
    int ret = 0;

    for (int i = 0; i < 4; i++) {
        if (*(str + i) == 0) {
            break;
        }

        ret *= 10;
        ret += *(str + i) - '0';
    }

    return ret;
}

/*
 * 从数据块中读取元组(Tuple)
 *
 * @param block 当前数据块
 * @param index 元组序号
 *
 * @return 读取的元组
 */
Tuple Get(const unsigned char *block, int index)
{
    Tuple t = {0, 0};

    unsigned char first[4], second[4];
    for (int i = 0; i < 4; i++) {
        first[i] = *(block + index * 8 + i);
        second[i] = *(block + index * 8 + 4 + i);
    }

    t.a = StringToInt(first);
    t.b = StringToInt(second);

    return t;
}

/*
 * 将输入的整数转换为字符串
 *
 * @param a   输入的整数
 * @param ret 指向输出的字符串的指针
 */
void IntToString(int a, unsigned char *ret)
{
    int length, base;

    if (a == 0) {
        length = 0;
        base = 0;
    } else if (a < 10) {
        length = 1;
        base = 1;
    } else if (a < 100) {
        length = 2;
        base = 10;
    } else if (a < 1000) {
        length = 3;
        base = 100;
    } else {
        length = 4;
        base = 1000;
    }

    for (int i = 0; i < length; i++) {
        ret[i] = a / base + '0';
        a %= base;
        base /= 10;
    }

    for (int i = length; i < 4; i++) {
        ret[i] = 0;
    }
}

/*
 * 将元组写入特定内容块的特定Index位置
 *
 * @param block 将要被写入的内容块
 * @param index 将要被写入的位置
 * @param t     输入的元组
 */
void TupleSet(unsigned char *block, int index, Tuple t)
{
    unsigned char strA[4], strB[4];
    IntToString(t.a, strA);
    IntToString(t.b, strB);

    for (int i = 0; i < 4; i++) {
        *(block + index * 8 + i) = strA[i];
    }

    for (int i = 4; i < 8; i++) {
        *(block + index * 8 + i) = strB[i - 4];
    }
}

/*
 * 将指定内容块写入到磁盘的特定位置中
 *
 * @param resBlock 输入的指定内容块
 * @param buf      缓冲区
 * @param output   将要输入的位置
 */
void Write(unsigned char *resBlock, Buffer *buf, int *output)
{
    // 设定后续磁盘块地址
    Tuple t = {(*output) + 1, 0};
    TupleSet(resBlock, 7, t);

    // 将resBlock写入到磁盘中
    if (writeBlockToDisk(resBlock, *output, buf) != 0) {
        perror("Writing Block Failed!\n");
        return;
    }

    // 结尾处理
    printf("结果写入磁盘 %d\n", *output);
    (*output)++;
    memset(resBlock, 0, 64);
}

/*
 * 将缓冲区中一部分内容分组进行排序
 *
 * @param start 进行排序的内容开始位置
 * @param end   进行排序的内容结束位置
 * @param buf   缓冲区
 */
void Sort(int start, int end, Buffer *buf)
{
    // 将缓冲区中的内容进行分组
    int groupCount = (end - start) / 6 + 1;

    // 排序过程中移动的指针
    int currentBlockId = start;
    int writeBlockId = start;

    // 初始化排序过程中的载体
    unsigned char *block, *blocks[6];

    // 分组进行排序
    for (int i = 0; i < groupCount; i++) {
        // 将当前组的内容块读取到 blocks 数组中
        int blockCount = 0;
        for (; blockCount < 6 && currentBlockId <= end; currentBlockId++, blockCount++) {
            blocks[blockCount] = readBlockFromDisk(currentBlockId, buf);
        }

        // 排序过程
        for (int j = 0; j < blockCount * 7 - 1; j++) {
            int blockTupleId1 = j / 7;
            int tupleIndex1 = j % 7;

            for (int k = j + 1; k < blockCount * 7; k++) {
                int blockTupleId2 = k / 7;
                int tupleIndex2 = k % 7;

                Tuple a = Get(blocks[blockTupleId1], tupleIndex1);
                Tuple b = Get(blocks[blockTupleId2], tupleIndex2);
                if (a.a > b.a) {
                    TupleSet(blocks[blockTupleId1], tupleIndex1, b);
                    TupleSet(blocks[blockTupleId2], tupleIndex2, a);
                }
            }
        }

        for (int j = 0; j < blockCount; j++) {
            block = blocks[j];
            Write(block, buf, &writeBlockId);
        }
    }
}

/*
 * 将元组写入到内容块中，如果内容块已满，则将内容块写入至外存
 *
 * @param resBlock   写入的目标内容块
 * @param index      即将写入内容块的位置
 * @param t          输入的即将被写入的元组
 * @param buf        缓冲区
 * @param resBlockId 目标块满后，目标块将被写入到外存的位置
 */
void TupleWrite(unsigned char *resBlock, int *index, Tuple t, Buffer *buf, int *resBlockId)
{
    TupleSet(resBlock, (*index)++, t);

    if (*index == 7) {
        Write(resBlock, buf, resBlockId);
        freeBlockInBuffer(resBlock, buf);
        resBlock = getNewBlockInBuffer(buf);
        memset(resBlock, 0, 64);
        *index = 0;
    }
}

/*
 * DEBUG用函数，输出特定区间的内容
 *
 * @param start 区间起点
 * @param end   区间终点
 */
void DebugPrint(int start, int end) {
    Buffer buf;
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    unsigned char *block;
    for (int i = start; i <= end; i++) {
        block = readBlockFromDisk(i, &buf);

        for (int j = 0; j < 7; j++) {
            Tuple t = Get(block, j);
            printf("%d %d\n", t.a, t.b);
        }

        freeBlockInBuffer(block, &buf);
    }

    freeBuffer(&buf);
}

void Task1(int start, int end, int key, int resBlockId)
{
    // 初始化缓冲区
    Buffer buf;
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    printf("////////////////////////\n");
    printf("// 基于线性搜索的选择算法 //\n");
    printf("////////////////////////\n");

    // 符合条件的元组数量
    int rCnt = 0;
    int resBlockIndex = 0;

    unsigned char *resBlock = getNewBlockInBuffer(&buf);
    for (int i = start; i <= end; i++) {
        // 从磁盘读取数据块
        unsigned char *block;
        printf("读入数据块 %d\n", i);
        if ((block = readBlockFromDisk(i, &buf)) == NULL) {
            perror("Reading Block Failed!\n");
            return;
        }

        for (int j = 0; j < 7; j++) {
            // 获取元组
            Tuple t = Get(block, j);

            // 如果符合SELECT条件，则
            if (t.a == key) {
                // 计数增加
                rCnt++;
                // 输出元组
                printf("(C = %d, D = %d)\n", t.a, t.b);
                // 将结果写入resBlock
                TupleSet(resBlock, resBlockIndex++, t);
                if (resBlockIndex == 7) {
                    Write(resBlock, &buf, &resBlockId);
                    resBlockIndex = 0;
                }
            }

            // 如果resBlock被写满 / 当前block的内容被读完
            if (i == end && j == 6) {
                Write(resBlock, &buf, &resBlockId);
            }
        }
        freeBlockInBuffer(block, &buf);
    }
    freeBuffer(&buf);

    printf("满足条件的元组共有 %d 个\n", rCnt);
    printf("IO读写次数: %lu\n", buf.numIO);
}

void Task2(int start, int end, int resBlockId)
{
    printf("////////////////////////\n");
    printf("// 两阶段多路归并排序算法 //\n");
    printf("////////////////////////\n");

    Buffer buf;
    unsigned char *block;

    // 初始化缓冲区
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    // 组数 (此处的分组方法应和Sort()中相同)
    int groupNum = (end - start) / 6 + 1;
    // 比较用的内容块
    unsigned char *compareBlock, *compareBlocks[8];
    int compareBlockIndex[8];
    // 写回用的内容块
    unsigned char *resBlock;
    int groupIndex[8];
    Tuple t, minTuple;
    int indexMin, minValue;
    int tupleIndex = 0;

    // 将缓冲区内start到end的内容进行 *分组* 排序
    Sort(start, end, &buf);

    compareBlock = getNewBlockInBuffer(&buf);
    memset(compareBlock, 0, 64);

    resBlock = getNewBlockInBuffer(&buf);
    memset(resBlock, 0, 64);

    t = (Tuple) {MAX_T, MAX_T};

    // 初始化compareBlock
    for (int i = 0; i < 8; i++) {
        TupleSet(compareBlock, i, t);
    }

    for (int i = 0; i < groupNum; i++) {
        // 初始化每一组的起始Index
        groupIndex[i] = start + i * 6;
        // 在compareBlocks[]数组中，初始化对应组的内容块
        block = readBlockFromDisk(groupIndex[i], &buf);
        compareBlocks[i] = block;
        t = Get(block, 0);
        // 将compareBlocks[]中对应的内容写入到compareBlock中
        TupleSet(compareBlock, i, t);
        compareBlockIndex[i] = 1;
    }

    while(true) {
        indexMin = 0;
        minValue = MAX_T;
        // 读取compareBlock中的最小值
        for (int i = 0; i < 8; i++) {
            t = Get(compareBlock, i);
            if (t.a < minValue) {
                minValue = t.a;
                minTuple = t;
                indexMin = i;
            }
        }

        // 如果排序已经结束，则退出排序过程
        if (minValue == MAX_T) {
            break;
        }

        // 将选出的最小的元组写入内容块(即写入外存的前一步)
        TupleWrite(resBlock, &tupleIndex, minTuple, &buf, &resBlockId);

        // 将被挑选出的元组使用该组的其他元组代替
        t = Get(compareBlocks[indexMin], compareBlockIndex[indexMin]++);

        // 如果挑选出的接替元组有效，则将其写入compareBlock
        if (t.a != 0 && compareBlockIndex[indexMin] < 8) {
            TupleSet(compareBlock, indexMin, t);
        } else {
            // 如果取出的元组无效且未到达组尽头，则读取下一块
            if ((groupIndex[indexMin] - start + 1) % 6 != 0 && groupIndex[indexMin] != end) {
                freeBlockInBuffer(compareBlocks[indexMin], &buf);
                block = readBlockFromDisk(++groupIndex[indexMin], &buf);
                compareBlocks[indexMin] = block;
                t = Get(block, 0);
                TupleSet(compareBlock, indexMin, t);
                compareBlockIndex[indexMin] = 1;
            } else {
                // 到达尽头
                t = (Tuple) {MAX_T, MAX_T};
                TupleSet(compareBlock, indexMin, t);
            }
        }
    }

    freeBuffer(&buf);
}

void Task3(int target)
{
    // 已经很多遍了所以就不敲注释了
    Buffer buf;
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    printf("////////////////////////\n");
    printf("// 基于索引的关系选择算法 //\n");
    printf("////////////////////////\n");

    // 建立索引
    // 所谓索引，即是在排序后，记录每一个内容块的第一个元组的第一条属性值
    // 方便后续通过升序来查找特定的值所在的内容块范围
    int resBlockId = 350, resBlockIndex = 0;
    unsigned char *resBlock = getNewBlockInBuffer(&buf), *block;
    for (int i = 0; i < 32; i++) {
        // 获取当前内容块中第一个元组，并初始化索引
        block = readBlockFromDisk(317 + i, &buf);
        Tuple t = Get(block, 0);
        freeBlockInBuffer(block, &buf);
        t.b = 317 + i;
        TupleSet(resBlock, resBlockIndex++, t);

        // 如果当前resBlock满了，则写回
        if (resBlockIndex == 7) {
            Write(resBlock, &buf, &resBlockId);
            freeBlockInBuffer(resBlock, &buf);
            resBlock = getNewBlockInBuffer(&buf);
            memset(resBlock, 0, 64);
            resBlockIndex = 0;
        }
    }
    Write(resBlock, &buf, &resBlockId);
    freeBuffer(&buf);

    // 重新初始化缓冲区，刷新IO次数
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    // 这里是读取上面写入的索引，通过线性查找的方法找到查找的具体范围
    int indexBlockId = 350;
    int targetStart = 0, targetEnd = 0;
    for (int i = 0; i < 5; i++) {
        printf("读入索引块 %d\n", indexBlockId);
        block = readBlockFromDisk(indexBlockId++, &buf);

        // 读取索引，确定想要选择的元组的范围
        Tuple t;
        for (int j = 0; j < 7; j++) {
            t = Get(block, j);
            if (t.a < target) {
                targetStart = t.b;
            } else if (t.a > target) {
                targetEnd = t.b;
                break;
            }
        }
        freeBlockInBuffer(block, &buf);

        // 如果已经确定范围，则退出这个过程
        if (targetEnd == t.b) {
            break;
        }
    }

    // 根据刚刚读出的范围，开始寻找目标元组
    // 很简单的查找过程，一看就懂
    resBlock = getNewBlockInBuffer(&buf);
    resBlockIndex = 0, resBlockId = 501;
    int rCount = 0;
    for (int i = targetStart; i < targetEnd; i++) {
        block = readBlockFromDisk(i, &buf);
        printf("读入数据块 %d\n", i);

        for (int j = 0; j < 7; j++) {
            Tuple t = Get(block, j);
            if (t.a == target) {
                rCount++;
                printf("(C = %d, D = %d)\n", t.a, t.b);
                TupleSet(resBlock, resBlockIndex++, t);
                if (resBlockIndex == 7) {
                    Write(resBlock, &buf, &resBlockId);
                    resBlockIndex = 0;
                }
            }
        }

        freeBlockInBuffer(block, &buf);
    }
    Write(resBlock, &buf, &resBlockId);
    freeBuffer(&buf);

    printf("满足条件的元组共有 %d 个\n", rCount);
    printf("IO读写次数: %lu\n", buf.numIO);
}

void Task4(int resBlockId)
{
    printf("////////////////////////\n");
    printf("// 基于排序的连接操作算法 //\n");
    printf("////////////////////////\n");

    // 排序过后，R和S的内容块范围
    int startR = 301, endR = 316;
    int startS = 317, endS = 348;

    Buffer buf;
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    // 初始化各种变量，接下来用得上
    // 带current的变量都是为了记录R指针的临时位置，用于回溯
    int currentBlockRId = startR, currentIndex = 0;
    int resBlockIndex = 0;
    // 在遍历R和S过程中用到的指针
    int blockSId = startS, blockRId = startR;
    // 用于记录相同S.C/R.A值的区间
    int valueS = 0, valueR = 0;
    // 遍历内容块用的
    int indexR = 0, indexS;
    unsigned char *blockS, *blockR, *resBlock = getNewBlockInBuffer(&buf);
    // 连接结果的统计
    int resCount = 0;

    for (; blockSId <= endS; blockSId++) {
        blockS = readBlockFromDisk(blockSId, &buf);
        for (indexS = 0; indexS < 7; indexS++) {
            Tuple tupleS = Get(blockS, indexS);

            // 如果上一个S中的元组C值相同，则需要把遍历R的指针回溯到记录的点
            // 具体原理实验课上的PPT有图，不再赘述
            if (valueS == tupleS.a) {
                blockRId = currentBlockRId;
                indexR = currentIndex;
            } else {
                valueS = tupleS.a;
            }

            Tuple tupleR;
            for (; blockRId <= endR; blockRId++) {
                blockR = readBlockFromDisk(blockRId, &buf);
                for (; indexR < 7; indexR++) {
                    tupleR = Get(blockR, indexR);
                    // 如果这里R.A已经大于S.C，说明已经遍历过头了，直接跳出
                    if (tupleR.a > tupleS.a) {
                        break;
                    }
                    // 遍历到了可以连接的元组
                    if (tupleR.a == tupleS.a) {
                        // 将相同A值的区间起点记录为回溯点，PPT上也有图，看那个更直观
                        if (valueR != tupleR.a) {
                            currentBlockRId = blockRId;
                            currentIndex = indexR;
                            valueR = tupleR.a;
                        }
                        // 符合条件就写入
                        TupleWrite(resBlock, &resBlockIndex, tupleR, &buf, &resBlockId);
                        TupleWrite(resBlock, &resBlockIndex, tupleS, &buf, &resBlockId);
                        resCount++;
                    }
                }

                if (indexR >= 7) {
                    indexR = 0;
                }
                freeBlockInBuffer(blockR, &buf);
                memset(blockR, 0, 64);

                if (tupleR.a > tupleS.a) {
                    break;
                }
            }
        }
        freeBlockInBuffer(blockS, &buf);
    }

    if (resBlockIndex != 0) {
        Write(resBlock, &buf, &resBlockId);
        freeBlockInBuffer(resBlock, &buf);
    }

    freeBuffer(&buf);

    printf("总共连接次数: %d\n", resCount);
}

void Task5(int resBlockId)
{
    printf("//////////////////////\n");
    printf("// 基于排序的集合差操作 //\n");
    printf("//////////////////////\n");

    // 整个流程与 Task4 基本一致，仅仅增加了部分判断
    // 故不写注释了
    int startR = 301, endR = 316;
    int startS = 317, endS = 348;

    Buffer buf;
    if (!initBuffer(520, 64, &buf)) {
        perror("Buffer Initialization Failed!\n");
        return;
    }

    int currentBlockRId = startR, currentIndex = 0, resBlockIndex = 0;
    int blockSId = startS, blockRId = startR;
    int valueS = 0, valueR = 0;
    int indexR = 0, indexS;
    unsigned char *blockS, *blockR, *resBlock = getNewBlockInBuffer(&buf);
    int resCount = 0;

    for (; blockSId <= endS; blockSId++) {
        blockS = readBlockFromDisk(blockSId, &buf);
        for (indexS = 0; indexS < 7; indexS++) {
            Tuple tupleS = Get(blockS, indexS);
            bool isReiterated = false;

            if (valueS == tupleS.a) {
                blockRId = currentBlockRId;
                indexR = currentIndex;
            } else {
                valueS = tupleS.a;
            }

            Tuple tupleR;
            for (; blockRId <= endR; blockRId++) {
                blockR = readBlockFromDisk(blockRId, &buf);
                for (; indexR < 7; indexR++) {
                    tupleR = Get(blockR, indexR);
                    if (tupleR.a > tupleS.a) {
                        break;
                    }
                    if (tupleR.a == tupleS.a) {
                        if (valueR != tupleR.a) {
                            currentBlockRId = blockRId;
                            currentIndex = indexR;
                            valueR = tupleR.a;
                        }

                        // 这里新增了判断，如果出现了完全相同的元组，则说明该元组不应出现在结果中
                        if (tupleR.b == tupleS.b) {
                            isReiterated = true;
                        }
                    }
                }

                if (indexR >= 7) {
                    indexR = 0;
                }
                freeBlockInBuffer(blockR, &buf);
                memset(blockR, 0, 64);

                if (tupleR.a > tupleS.a) {
                    break;
                }
            }

            if (!isReiterated) {
                TupleWrite(resBlock, &resBlockIndex, tupleS, &buf, &resBlockId);
                printf("(X = %d, Y = %d)\n", tupleS.a, tupleS.b);
                resCount++;
            }
        }
        freeBlockInBuffer(blockS, &buf);
    }

    if (resBlockIndex != 0) {
        Write(resBlock, &buf, &resBlockId);
        freeBlockInBuffer(resBlock, &buf);
    }

    freeBuffer(&buf);

    printf("S - R 有 %d 个元组\n", resCount);
}

int main()
{
    Task1(17, 48, 128, 100);
    printf("\n");

    printf("对关系R进行排序\n");
    Task2(1, 16, 301);
    printf("\n");

    printf("对关系S进行排序\n");
    Task2(17, 48, 317);
    printf("\n");

    Task3(128);
    printf("\n");

    Task4(400);
    printf("\n");

    Task5(500);
    printf("\n");

    return 0;
}