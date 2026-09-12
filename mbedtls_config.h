#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

// Core configuration for standalone cryptographic use
#define MBEDTLS_MD5_C

// Optional but required by mbedTLS source if limits.h isn't implicitly linked
#include <limits.h> 

#endif /* MBEDTLS_CONFIG_H */