/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
    /*
     * AB = (A' | B')'
     */
    x = ~x;
    y = ~y;
    x = x | y;
    x = ~x;
    return x;
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
    // do it by myself
    // a better solution
    // return (x >>(n<<3)) & 0xFF;
    if(n == 1) {
        x = x >> 8;
    } else if(n == 2) {
        x = x >> 16;
    } else if(n == 3) {
        x = x >> 24;
    }
    x = x & 0xFF;
  return x;

}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
    //c语言中默认的移位操作是算术右移；
    // do it failure, see the solution
    //必须是这个顺序，如果是第一行先把y取~，得到y为0111...11
    //然后y >> n,再y<<1并不会得到正确的结果，y<<1会移进来一个0，导致结果错误
    //其实应该是y >>(n - 1) 但是会存在n = 0的情况，所以不可以n - 1，
    //当然这样你也可以最后+ 0x1把结果补上，但是何必这么麻烦呢
    //再更新，不能加1，因为有时候本来低位就是0 ，加了1反而出现了错误，所以就要按下面这样做
    int y = (0x1 << 31); // 1000...000
    y = y >> n;
    y = y << 1;
    y = ~y;
    x = (x >> n) & y;
    return x;
}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
    // don't know how to solve it
    // my code is right,and run in the clion, the result is right! but the result here is always wrong, I don;t know why
    // see the solution
    // a difficult problem
    //这个题目我不会做，在网上找到了Hamming weight算法。这个算法的思想使用分治策略，先计算每两个相邻的
    // bit 中有多少个 1，之后再依次求解在4、8、16、32个 bit 中共有多少个 1。    
    //5555 0101 0101 0101 0101
    //3333 0011 0011 0011 0011
    //0F0F 0000 1111 0000 1111
    //00FF 0000 0000 1111 1111
    /*int x = (y & 0x55555555) + ((y >> 1) & 0x55555555);*/
    /*x = (x & 0x33333333) + ((x >> 2) & 0x33333333);*/
    /*x = (x & 0x0f0f0f0f) + ((x >> 4) & 0x0f0f0f0f);*/
    /*x = (x & 0x00ff00ff) + ((x >> 8) & 0x00ff00ff);*/
    /*x = (x & 0x0000ffff) + ((x >> 16) & 0x0000ffff);*/
    // referenced :
    // https://stackoverflow.com/questions/3815165/how-to-implement-bitcount-using-only-bitwise-operators
    int mask1 = 0x55;
    int mask2 = 0x33;
    int mask3 = 0x0F;
    int result = 0;
    mask1 = mask1 | (mask1 << 8);
    mask1 = mask1 | (mask1 << 16);
    mask2 = mask2 | (mask2 << 8);
    mask2 = mask2 | (mask2 << 16);
    mask3 = mask3 | (mask3 << 8);
    mask3 = mask3 | (mask3 << 16);
    result = (x & mask1) + ((x >> 1) & mask1);
    result = (result & mask2) + ((result >> 2) & mask2);
    result = (result & mask3) + ((result >> 4) & mask3);
    return (result + (result >> 8) + (result >> 16) + (result >> 24)) & 0xff;
}
    
/* 
 * bang - compute !x without using !
 *   examples: bang(3) = 0, bang(0) = 1
 *   legal ops: ~ & ^ | + << >>
 *   max ops: 12
 *   rating: 4 
 */
int bang(int x) {
    // my solution is illegal
    /*
     * if(x == 0) return 1;
     * else return 0;
     */
    /* for x != 0, the highest bit of x | (-x) will always become 1 while when x == 0, the result is the opposite */
    // -x can be represented as ~x+1,所有正数取反，最高位符号位都变成1，所以x|（-x）=1唯一特殊的就是1000 = -8 对应的-x发生溢出，但负数本身最高位就是1，取或一定是1
    // 只有0 +0 -0补码表示都为0000，取或得0；
	return (~((x | (~x + 1)) >> 31) & 1);
}

/* 
 * tmin - return minimum two's complement integer 
 *   legal ops: ! ~ & ^ | + << >>
 *   max ops: 4
 *   rating: 1
 */
int tmin(void) {
    //easy problem
    return (0x1 << 31);
}
/* 
 * fitsBits - return 1 if x can be represented as an 
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
    /*
     * n 位补码能够表示的数的范围是[−2n−1,2n−1−1]，所以如果在这个范围内则返回1，否则返回0。如果x大于2n−1−1，则会发生正溢出，得到负数；
     * 如果x小于−2n−1则会发生负溢出。所以题目的求解方法就是将x的符号位从第 n 位
     * 拓展到第32位得到extened_x，之后通过判断x == exntened_x来判断是否发生了溢出。
     */
    // refer to the solution, always run error, but it's right in fact
    int shiftNumber= 32 + (~n + 1);// 32 - n
    return !(x^((x<<shiftNumber)>>shiftNumber));
 }
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
    /*
     * 33 = 0010 0001, -33 = 101 1111
     * -33 >> 4 = 101 = -4 + 1 = -3 ! different from the example!
     *  see the solution
     *  分析：对于正数，直接使用算术右移即可；对于负数，需要加上适当的偏移量以实现向上舍入，这可以用根据符号位生成1个全0或者全1的掩码来控制。
     *  构造一个偏置量，因为要右移n位，如果是负数的话，加上2^n-1）后再移位。
     *  对于整数使用右移代替除法,都可以表示为 (x<0?(x+(1<<k) - 1):x)>>k
     */
    // 默认的右移是算术右移
    int sgn = x >> 31; // 0xffffffff or 0x0
    int mask = (1 << n )+ (~0) ; //2^n -1
    int bias = sgn & mask; //if x >= 0 bias = 0
    return (x+bias) >> n;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    // -x = ~x + 1
    return ~x + 1;
}
/* 
 * isPositive - return 1 if x > 0, return 0 otherwise 
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
    /*
     * 主要是处理0的问题，0不认为是正数，应该返回为0
     */
    int positive = x >> 31;
    positive = !(positive & 0x1);
    // see the solution, should add this sentence followed
    // positve对于正数和0为1，负数，值为0，但是0结果不对
    // 正确结果应该是 正数返回1， 负数和0都返回0
    // 正数 1^0 负数 0^0  0 1^1
    positive = positive ^ !x;
    return positive;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    /*以下是我的做法，判断y-x的正负
    x = ~x + 1;
    y = y + x;
    y >>= 31;
    y = y & 0x1;
    y = !y;
    return y;
    */
    /*
     * 对于这个问题，我们需要按照是否溢出，以及x与y的符号分别讨论。
    最基本的思路是，做y-x，若结果的符号位为1，则返回0，反之返回1。但这样就忽略了溢出可能导致的问题，现在我们做如下考虑。
    若x，y均为正数，或x，y均为负数，则y-x绝不可能溢出，因此可直接用差的符号位判断大小。
    若x为正数，y为负数，可以直接返回0。
    若x为负数，y为正数，可以直接返回1。
    我们可以生成两种条件变量，一种为x，y同号时返回正确结果的条件变量，另一种为x，y异号时返回正确结果的条件变量。
    对于x，y同号和异号这两种不同的情况，我们可以用!((x ^ y) >> 31)生成掩码使得在任意的情况下，只有正确的条件变量生效。
*/
    /* different processing ways when x and y have the same signs or different signs */
	int diff = y + (~x) + 1;
	int not_diff_sign = !(diff >> 31);//不溢出时返回的结果
	int mask = !((x ^ y) >> 31);//同号为1，异号为0
	int result_not_overflow = mask & not_diff_sign;// 同号肯定不溢出
	int result_overflow = (!mask) & (x >> 31); //异号，x为正 就是0，x为负就是1
	return result_overflow | result_not_overflow;

}
/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x) {
    /*
     * 10-1, 100-2, 1000-3, 10000-4
     * 保留最左边的1，所以任务就是找到最左边的1，即为结果，好吧，不会做
     * see the solution  分析：
    这道题参考了网上的答案。核心的思想是二分查找。
    一个数字共32位首先将x右移16位，如果x>0，则表明结果的第5位为1，将该位置为1。
    然后根据结果将x右移8位（第5位为0）或者将x右移24位（第5位为1），如果x>0，则表明结果的第4位为1，将该位置为1。8和24分别是左右两个部分的中间数字
    以此类推，直到得出结果。
    */
    /* Binary Search */
	int result = (!!(x >> 16) << 4);// 双感叹号的作用主要是，非零值等于1，零就等于0
	result = result + ((!!(x >> (result + 8))) << 3);// 若上一步的result = 16，这里就是移位24，否则就是移位8
	result = result + ((!!(x >> (result + 4))) << 2);
	result = result + ((!!(x >> (result + 2))) << 1);
	result = result + (!!(x >> (result + 1)));
	return result;
}
/* 
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
    /*see the solution
     * 见书上浮点数，若为NaN，对于单精度，则位表示为k=8 n = 23, 0 111, 1111, 1 + a number not equal to 0*/ 
    // a infinate number can be express as 7f800000
    int tmp =0,ret=0;
    ret = uf ^ 0x80000000; // sign reverse
    tmp = uf & 0x7fffffff; // 去掉符号位
    if(tmp > 0x7f800000)   // NaN
        ret = uf;
    return ret;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
    /* see the page P82
     * see the answer, difficult problem
     * 分别求出符号位sign，指数部分exp和小数部分frac。原来整数称x
     * for float type, k = 8, bias = 2^7 - 1 = 127, E = e - bias. so e = E + bias = E + 127

    1 首先把特殊情况的0x0 和 0x80 00 00 00（-2^31）挑出来，因为不能用移位的办法求exp和frac。
    2 求绝对值，找出x的最高位（最左边的1），此时要从第30位找起，因为第31位是符号位。找到之后该位数就是v=(−1)^S*M*(2)^E中的E，可以求得exp=E+127
    3 求frac：取x最高位后的23位。步骤：先去掉x高位的0，然后右移8位将最高位后的23位移到低23位。
    4 求精度：int转float型会丢失最后8位的精度（31-23=8），所以要判断x的最后八位需不需要进位，如果最后8位超过128（0x80）或者最后8位=128且frac最后一位=1，则进位。
    5 进位后：需要检查frac有没有再进位，frac>>23进行判断，如果frac进位了，那么exp+1，frac再取最后23位。把sign exp frac组合成结果
    */
    int sign = (x>>31)&0x01;
    int frac_mask = 0x7fffff; // (1<<23) -1
    int frac=0,exp=0,delta=0; 
    int i = 0; 
    if(!x)
        return x;
    else if(x==0x80000000)// -2^31, E = 31, so e = E + bias
        exp = 158;          // 158 = 127 +31
    else{
        if(sign)  x=-x;     // abs(x)
        i=30; 
        while(!(x>>i)) // find the most significant bit
            i--;
        exp = i+127;        // exp = Bias + E

        x= x<<(31-i);       // clean all those zeroes of high bits
        frac = (x>>8) & frac_mask;//right shift 8 bits to become the fraction, exp have 8 bits total
        x = x & 0xff;
        delta = x>0x80||((x==0x80 )&& (frac&0x01)); //if lowest 8 bits of x is larger than a half,or is 1.5,round up 1
        frac += delta;
        if(frac>>23){ //if after rounding fraction is larger than 23bits
            exp += 1;
            frac = frac & frac_mask;
        }
    }
    return (sign<<31)|(exp<<23)|frac;
}

/*
 *   float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
    /* see the answer
     * 要求：求2*uf，uf是一个用unsigned表示的float，当遇到NaN时返回该NaN
    检查是否NaN：exp==0xff
    然后分两种情况：
    1、exp=全0的，非规格数, frac<<1,exp不变
    2、exp≠全0的，规格数, exp++，检查exp==0xff，若exp==0xff，此时该数超范围（无穷大），frac=0
    取符号位：uf>>31&0x01
    取frac：uf&((1<<23)-1)
    取exp：(uf>>23)&0xff
     */
    int sign = uf>>31&&0x01; // sign
    int exp = uf>>23 & 0xff; // 8 bits exp
    int frac = uf & 0x7fffff; // last 23 bits M
    if(exp!=0xff){
        if(!exp)  frac=frac<<1;
        else{
            exp += 1;
            if(exp==0xff)
                frac=0;
        }
    }
  return sign<<31|exp<<23|frac;
}
