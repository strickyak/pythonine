#ifndef _SAVER_H_
#define _SAVER_H_

#include "octet.h"
#include "standard.h"

void SaveClusterToFile(word top, word filename_str);
word LoadClusterFromFile(word filename_str);

#ifndef INF
#define INF 255
#endif

#endif // _SAVER_H_
