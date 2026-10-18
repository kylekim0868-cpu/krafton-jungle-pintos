#ifndef __LIB_ROUND_H
#define __LIB_ROUND_H

/* X를 STEP의 배수 중 가장 가까운 값으로 올림한 결과를 낸다.
 * X >= 0, STEP >= 1 일 때만 사용할 것. */
#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))

/* X를 STEP으로 나눈 몫을 올림한 결과를 낸다.
 * X >= 0, STEP >= 1 일 때만 사용할 것. */
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

/* X를 STEP의 배수 중 가장 가까운 값으로 내림한 결과를 낸다.
 * X >= 0, STEP >= 1 일 때만 사용할 것. */
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

/* DIV_ROUND_DOWN은 없다. 그냥 X / STEP 하면 되기 때문이다. */

#endif /* lib/round.h */