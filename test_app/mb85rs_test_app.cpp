/*
 * This demonstrates use of the MB85RS driver userspace API class.
 * Write a string to the device and then read it back
 */
#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include "libmb85rs.h"

MB85RS ram("/path/to/spi/driver");

int main ( int argc, char **argv )
{   
    unsigned char buf[] = "testinggg";
    ram.write(0x00, buf, sizeof(buf));
    
    unsigned char ret[sizeof(buf)];
    ram.read(0x00, ret, sizeof(buf));
    
    std::cout << ret << std::endl;
    
    return 0;
}