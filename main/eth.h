#ifndef ETH_H
#define ETH_H

//void ethernet(void);
esp_netif_t *ethernet(const char *ip_str, const char *gw_str);


#endif
