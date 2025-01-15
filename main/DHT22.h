#ifndef _DHT22_H_  
#define _DHT22_H_

void DHTinit(gpio_num_t gpio_num);
int DHTget(uint16_t*, uint16_t*);

#endif
// END OF FILE