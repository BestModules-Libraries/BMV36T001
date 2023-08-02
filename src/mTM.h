#ifndef _MTM_H
#define _MTM_H

#ifdef __cplusplus
 extern "C" {
#endif
void mTM_Configuration(uint32_t freq, void(*func)());
void stopTimer(void);
void startTimer(void);

#ifdef __cplusplus
}
#endif

#endif