/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	// know the reason, see the OneNote
	// separate the array into 8 X 8, decrease the misses number
        int i, j, tmp, index;
	int rowBlock, colBlock;
	if(M == 32) {
		for(rowBlock = 0; rowBlock < N; rowBlock += 8) {
			for(colBlock = 0; colBlock < M; colBlock += 8) {
				for(i = rowBlock; i < rowBlock + 8; i++) {
					for(j = colBlock; j < colBlock + 8; j++) {
						if(i != j) {
							B[j][i] = A[i][j];
						} else {
							/* important
							 * //i==j means is the diagonal. if we set B right now ,the  misses and evictions will increase .
							 * because the cache set of B is same to A.
							 */
							//我们在函数每次循环的碰到对角线元素时先存储好，然后在本次循环结束时才处理对角线元素。
							tmp = A[i][j];
							index = i;
						}
					}
					// after the j for loop, the tmp value has been updated
					//just set B on the diagonal(colBlock == rowBlock). other than shouldn't set the B
					if(colBlock == rowBlock){
						B[index][index] = tmp;
					}
				}
			}
		}
	} else if(M == 64) {
		/*
		for(rowBlock = 0;rowBlock < N ;rowBlock += 4){
			for(colBlock = 0; colBlock < M; colBlock += 4){
				for(i = rowBlock; i < rowBlock + 4; i++){
					for(j = colBlock; j < colBlock + 4; j++){
						if(i != j){
							B[j][i] = A[i][j];
						} else {
							tmp = A[i][j];                  //i==j means is the diagonal. if we set B right now ,the  misses and evictions will increase .
			   												// because the cache set of B is same to A.
							index = i;
						}
					}
					if(colBlock == rowBlock){             //just set B on the diagonal. other than shouldn't set the B
						B[index][index] = tmp;
					}
				}
			}
		}
		 */
		/*
		 * 题目要求我们使用不超过12个变量，我们在遍历的过程中需要使用4个变量，那么还有8个变量可以供我们使用
		 * 我们首先对A和B的上半部分操作，这样除了第一次以外缓存全部命中。我们把B的上半部分填满，不能浪费。

			首先B的左上角当然可以正确转置。
			而B的右上角也填上A的右上角中的数。这里放上去的也是转置过的，我们最后只需要把这部分平移到B的左下角就好。
			整个过程按照A和B的行来遍历操作。
			现在，B的左上已经Ok，而右上则需要平移到左下。接着，我们处理右上和左下。

			首先用四个变量存储A的左下角的一列。
			再用四个变量存储B的右上角的一行。
			把四个变量存储的A的左下角的一列移动到B右上角的一行
			把四个变量存储的B的右上角的一行平移到B的左下角的一行
			经过上面的操作，矩阵B的左下，右上下，左上就都完成了。

			最后，我们直接对右下角转置即可。

			这样跑出来，我们的结果是miss：1179，已经满分了。
			但是后来看到一个解答，把最后对右下角的转置移动到了第二大步骤的最后，更加减少了miss，原因是第二大步骤本来就是在对下半部分操作，顺带处理这一行当然不会出现miss，就放在里面按列处理了。
		 */
		int k, l, t1, t2, t3, t4, t5, t6, t7, t8;
		for (i = 0; i < N; i += 8) {
			for (j = 0; j < M; j += 8) {
				for (k = i; k < i + 4; k++) {
					t1 = A[k][j];
					t2 = A[k][j + 1];
					t3 = A[k][j + 2];
					t4 = A[k][j + 3];
					t5 = A[k][j + 4];
					t6 = A[k][j + 5];
					t7 = A[k][j + 6];
					t8 = A[k][j + 7];

					B[j][k] = t1;
					B[j + 1][k] = t2;
					B[j + 2][k] = t3;
					B[j + 3][k] = t4;
					B[j][k + 4] = t5;
					B[j + 1][k + 4] = t6;
					B[j + 2][k + 4] = t7;
					B[j + 3][k + 4] = t8;
				}
				for (l = j + 4; l < j + 8; l++) {
					// 存储A的左下角的一列
					t5 = A[i + 4][l - 4];
					t6 = A[i + 5][l - 4];
					t7 = A[i + 6][l - 4];
					t8 = A[i + 7][l - 4];
					// 存储B的右上角的一行
					t1 = B[l - 4][i + 4];
					t2 = B[l - 4][i + 5];
					t3 = B[l - 4][i + 6];
					t4 = B[l - 4][i + 7];
					// 右上角和左下角有set冲突，所以不能直接访问，需要这样用局部变量中转
					// 把四个变量存储的A的左下角的一列移动到B右上角的一行
					B[l - 4][i + 4] = t5;
					B[l - 4][i + 5] = t6;
					B[l - 4][i + 6] = t7;
					B[l - 4][i + 7] = t8;
					// 把四个变量存储的B的右上角的一行平移到B的左下角的一行
					B[l][i] = t1;
					B[l][i + 1] = t2;
					B[l][i + 2] = t3;
					B[l][i + 3] = t4;
					// 最后，我们直接对右下角转置即可
					B[l][i + 4] = A[i + 4][l];
					B[l][i + 5] = A[i + 5][l];
					B[l][i + 6] = A[i + 6][l];
					B[l][i + 7] = A[i + 7][l];
				}
			}
		}
	} else {
                for(rowBlock = 0; rowBlock < N; rowBlock += 16){
			for(colBlock = 0; colBlock < M; colBlock += 16){
                                for(i = rowBlock; i < rowBlock + 16 && (i < N); i++){
					for(j = colBlock; j < colBlock + 16 &&(j < M); j++){
						if(i != j){
							B[j][i] = A[i][j];
						} else {
							tmp = A[i][j];                  //i==j means is the diagonal. if we set B right now ,the  misses and evictions will increase . because the cache set of B is same to A.
							index = i;
						}
					}
					if(colBlock == rowBlock){             //just set B on the diagonal. other than shouldn't set the B
						B[index][index] = tmp;
					}
				}
			}
		}
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

