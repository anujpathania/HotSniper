#ifndef HOOKS_H
#define HOOKS_H

#ifdef __cplusplus
extern "C" {
#endif

void parmacs_roi_begin(void);
void parmacs_roi_end(void);
void parmacs_iter_begin(int iter);
void parmacs_iter_end(int iter);

#ifdef __cplusplus
}
#endif

#endif // HOOKS_H
