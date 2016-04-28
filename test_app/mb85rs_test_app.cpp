/*
 * This demonstrates use of the MB85RS driver userspace API class.
 * Write a string to the device and then read it back
 */
#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include "libmb85rs.h"

//path to the SPI device on beaglebone
MB85RS ram("/sys/devices/platform/ocp/48030000.spi/spi_master/spi1/spi1.0/");

int main ( int argc, char **argv )
{   
    unsigned char buf[] = "testinggg";
    ram.write(0x23, buf, sizeof(buf));
    
    unsigned char ret[sizeof(buf)];
    ram.read(0x23, ret, sizeof(buf));
    
    std::cout << ret << std::endl;
    
    return 0;
}