/*
   Aux. methods used for os_printf floating point numbers.
   Don't use in production.
 */
 #include "util.h"

int ICACHE_FLASH_ATTR power(int base, int exp){
        int result = 1;
        while(exp) { result *= base; exp--; }
        return result;
}

char* ICACHE_FLASH_ATTR f2s(float num, uint8_t decimals) {
        static char* buf[16];
        int whole = num;
        int decimal = (num - whole) * power(10, decimals);
        if (decimal < 0) {
                // get rid of sign on decimal portion
                decimal -= 2 * decimal;
        }
        char* pattern[10]; // setup printf pattern for decimal portion
        os_sprintf(pattern, "%%d.%%0%dd", decimals);
        os_sprintf(buf, pattern, whole, decimal);
        return (char *)buf;
}
