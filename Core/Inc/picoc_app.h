#ifndef __PICOC_APP_H__
#define __PICOC_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

void PicocApp_Init(void);
void PicocApp_Task(void);
int PicocApp_ConsoleGetCharBlocking(void);

#ifdef __cplusplus
}
#endif

#endif /* __PICOC_APP_H__ */
