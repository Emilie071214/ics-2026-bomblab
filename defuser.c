#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}


int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    // 1. 拆分 challenge
    uint32_t high32 = (uint32_t)(challenge >> 32);
    uint32_t mid16  = (uint32_t)(challenge >> 16);
    uint32_t low32  = (uint32_t)challenge;

    // 2. 计算 step1
    uint32_t step1 = (high32 + 4 * mid16) ^ 0x15c8;

    // 3. 计算 step0
    uint32_t step0 = (5 * low32 + high32) ^ 0x55a6;

    // 4. 计算最终结果
    uint32_t result = (5 * step0) ^ (step1 + 0xa639);

    // 5. 存入结果
    values[0] = result;
    count = 1;

    return write_answer(out, cap, values, count);
}   
    

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;

    // 1. 计算第 1 个数 (严格使用 uint32_t 截断)
    uint32_t temp = (uint32_t)(challenge >> 4);
    values[0] = (((temp * 2) & 0x7e) + 0x19);

    // 2. 循环推导第 2 ~ 6 个数
    uint32_t shift = 10;
    for (size_t i = 1; i < 6; i++) {
        // 提取对应位置的 challenge 片段
        uint32_t part = ((uint32_t)(challenge >> shift)) & 0x1f;
        
        // 上一个数的 3 倍，强制 32 位运算
        uint32_t prev_3x = values[i - 1] * 3;
        
        // 拼接公式
        values[i] = (part + i) ^ prev_3x;
        
        shift += 2;
    }

    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    
    // 1. 计算 v1 和 x
    uint32_t v1 = (uint32_t)(challenge & 0x7);
    uint32_t x = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    
    uint32_t v2 = 0;

    // 2. 根据终端里的真实跳转表映射计算 v2
    switch (v1) {
        case 0: v2 = 3 * x + 0xf57a; break;          // 索引 0 -> p3_case_6
        case 1: v2 = (x ^ 0xedc7) + (x >> 2); break; // 索引 1 -> p3_case_4
        case 2: v2 = 5 * x + 0x11440; break;         // 索引 2 -> p3_case_1
        case 3: v2 = (2 * x) ^ 0x265d; break;        // 索引 3 -> p3_case_5
        case 4: v2 = (2 * x) ^ 0xe291; break;        // 索引 4 -> p3_case_3
        case 5: v2 = 5 * x + 0x32d84; break;         // 索引 5 -> p3_case_7
        case 6: v2 = (x ^ 0xe419) + (x >> 2); break; // 索引 6 -> p3_case_2
        case 7: v2 = 5 * x + 0x2b8d; break;          // 索引 7 -> p3_case_0
        default: return 1;
    }

    // 3. 存入数组并调用 write_answer 输出
    values[0] = v1;
    values[1] = v2;
    
    return write_answer(out, cap, values, count);
}

// 辅助函数：复刻 p4_walk 的递归逻辑
static uint32_t p4_walk(uint32_t val, uint32_t low, uint32_t high, uint32_t mask) {
    uint32_t mid = low + (high - low) / 2;
    
    if (mid == val) {
        return (mid + 31) ^ mask;
    }
    
    if (val > mid) {
        uint32_t ret = p4_walk(val, mid + 1, high, mask);
        return ((3 * ret) ^ (mid + mask)) ^ 0x1e;
    } else {
        uint32_t ret = p4_walk(val, low, mid - 1, mask);
        return ((3 * ret) ^ (mid + mask)) ^ 0x1b;
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1; // 第4关只需要输入 1 个整数
    
    // 1. 计算四个传入参数
    uint32_t val = (uint32_t)(challenge & 0x7f);
    uint32_t low = 0;
    uint32_t high = 0x7f; // 127
    uint32_t mask = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    
    // 2. 调用复刻的递归函数求出正确值
    uint32_t ans = p4_walk(val, low, high, mask);
    
    values[0] = ans;
    
    return write_answer(out, cap, values, count);
}

// 第5关的节点结构体
typedef struct Node5 {
    uint32_t val1;
    uint32_t val2;
    int next_idx;
} Node5;

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2; // 这一关需要输出 2 个数字
    
    // 从 GDB 拿到的真实数据（按地址从小到大排序）
    Node5 nodes[] = {
        {4019u, 2164u, 11},  // 索引 0 (地址 0x405ba0)
        {41611u, 1549u, 9},  // 索引 1 (地址 0x405bb0)
        {35427u, 2629u, 10}, // 索引 2 (地址 0x405bc0)
        {50610u, 827u, 5},   // 索引 3 (地址 0x405bd0)
        {50895u, 1115u, 1},  // 索引 4 (地址 0x405be0)
        {19462u, 78u, 8},    // 索引 5 (地址 0x405bf0)
        {4996u, 315u, 4},    // 索引 6 (地址 0x405c00)
        {52611u, 2932u, 3},  // 索引 7 (地址 0x405c10)
        {30308u, 1847u, 6},  // 索引 8 (地址 0x405c20)
        {45699u, 1337u, 2},  // 索引 9 (地址 0x405c30)
        {59281u, 554u, 0},   // 索引 10 (地址 0x405c40)
        {377u, 2322u, -1},   // 索引 11 (地址 0x405c50)
    };

    // 关键修复：映射真正的头节点索引
    int head_idx[8] = {7, 3, 5, 8, 6, 4, 1, 9};

    // 1. 计算起始索引和循环次数
    int index = challenge & 0x7;
    int loop_count = (challenge >> 3) & 0x3;

    // 2. 计算初始的 x
    uint32_t x = (uint32_t)((challenge >> 6) ^ (challenge >> 32)) ^ 0x55c0;

    // 3. 初始化第一个节点 (使用映射好的 head_idx)
    int cur = head_idx[index];
    uint32_t ans = (x * 3 + nodes[cur].val1) ^ nodes[cur].val2;
    uint32_t out_val = nodes[cur].val2;

    // 4. 根据 loop_count 进行遍历
    for (int i = 0; i < loop_count; i++) {
        cur = nodes[cur].next_idx;
        ans = (ans * 3 + nodes[cur].val1) ^ nodes[cur].val2;
        out_val = nodes[cur].val2;
    }

    // 5. 填入结果并输出
    values[0] = ans;
    values[1] = out_val;
    
    return write_answer(out, cap, values, count);
}

static uint32_t p6_op(uint32_t idx, uint32_t x) {
    switch (idx) {
        case 0: return ((x >> 2) ^ x) + 0x7328;
        case 1: return (x ^ 0x4788) + 4 * x;
        case 2: return ((x << 2) | (x >> 30)) ^ 0xe31c; // rol 2
        case 3: return (4 * x + 0xc34f) ^ (5 * x);
        case 4: return (x + 0xa7) ^ (x >> 7);
        case 5: return (x >> 1) | (x << 31);            // ror 1
        case 6: return (x + 0xa7) ^ (x >> 7);
        case 7: return (4 * x + 0xc34f) ^ (5 * x);
        default: return 0;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1; 
    
    uint32_t idx1 = (uint32_t)(challenge & 0x7);
    uint32_t idx2 = (uint32_t)((challenge >> 3) & 0x7); // 这里绝对是对的
    
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    
    uint32_t ans1 = p6_op(idx1, x);
    uint32_t ans2 = p6_op(idx2, ans1 ^ 0x4db4);
    
    values[0] = ans2 + 0x306a;
    
    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t ch_low = (uint32_t)challenge;
    uint32_t ch_high = (uint32_t)(challenge >> 32);
    uint32_t c_low = (uint32_t)context;
    uint32_t c_high = (uint32_t)(context >> 32);

    // 1. v1 = context 的低 2 位
    uint32_t v1 = c_low & 0x3;

    // 2. r9d 初始计算
    uint32_t r9d = v1 * 0x5f9e;

    // 3. esi = (challenge >> 32) ^ challenge_low
    uint32_t esi = ch_high ^ ch_low;

    // 4. r8d = (context >> 32) ^ context_low
    uint32_t r8d = c_high ^ c_low;

    // 5. r13 右移 16 位并异或到 r9d
    r9d ^= (uint32_t)(context >> 16);

    // 6. 传给 secret_mix 的参数 edi
    uint32_t edi = esi ^ r8d;

    // 7. 调用 secret_mix
    uint32_t mix_ret = (edi >> 3) | (edi << 29);
    mix_ret ^= 0xc4b9;

    // 8. 计算 v2
    uint32_t v2 = mix_ret ^ r9d;

    // 9. 计算 v3
    uint32_t edx = r8d + v1 + 0x9b27;
    edx += (esi >> 3);
    uint32_t v3 = v2 ^ edx;

    // 10. 格式化输出：32 个 X + OPEN + 三个数字 (去掉 %010u，直接正常输出即可)
    int size = snprintf(out, cap, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXOPEN %u %u %u", v1, v2, v3);
    return (size < 0 || (size_t)size >= cap) ? 1 : 0;
}