/**
 * @file i2c_a7.c
 * @author NXP Semiconductors
 * @version 1.0
 * @par License
 * Copyright 2017 NXP
 *
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 *
 * @par Description
 * i.MX6UL board specific i2c code
 * @par History
 *
 **/
#include "i2c_a7.h"
#include <stdio.h>
#include <string.h>

#ifdef LINUX
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <errno.h>
#include <time.h>
#elif defined(ZEPHYR)
#include <i2c.h>
#endif

//#define LOG_I2C 1
#if 1
#define dbg_print(...)
#define dbg_perror(...)
#else
#define dbg_print(fmt, ...) printf(fmt, __VA_ARGS__)
#define dbg_perror(fmt, ...) perror(fmt, __VA_ARGS__)
#endif

#ifdef LINUX
static int axSmDevice;
#elif defined(ZEPHYR)
static struct device *axSmDevice;
#endif
static int axSmDevice_addr = 0x48;      // 7-bit address
#ifdef LINUX
static char devName[] = "/dev/i2c-1";   // Change this when connecting to another host i2c master port
#elif defined(ZEPHYR)
static char devName[] = "I2C_0";
#endif

/**
* Opens the communication channel to I2C device
*/
i2c_error_t axI2CInit()
{
#ifdef LINUX
    unsigned long funcs;
#endif

    /*
     * Open the file in /dev/i2c-1
     */
    dbg_print("I2CInit: opening %s\n", devName);

#ifdef LINUX
    if ((axSmDevice = open(devName, O_RDWR)) < 0)
    {
        dbg_print("opening failed...\n");
        dbg_perror("Failed to open the i2c bus");
        return I2C_FAILED;
    }

    if (ioctl(axSmDevice, I2C_SLAVE, axSmDevice_addr) < 0)
    {
        dbg_print("I2C driver failed setting address\n");
    }

    // clear PEC flag
    if (ioctl(axSmDevice, I2C_PEC, 0) < 0)
    {
        dbg_print("I2C driver: PEC flag clear failed\n");
    }
    else
    {
        dbg_print("I2C driver: PEC flag cleared\n");
    }

    // Query functional capacity of I2C driver
    if (ioctl(axSmDevice, I2C_FUNCS, &funcs) < 0)
    {
        dbg_print("Fatal: Cannot get i2c adapter functionality\n");
        return I2C_FAILED;
    }
    else
    {
        if (funcs & I2C_FUNC_I2C)
        {
            dbg_print("I2C driver supports plain i2c-level commands.\n");
            if ( (funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA) == I2C_FUNC_SMBUS_READ_BLOCK_DATA )
            {
                dbg_print("I2C driver supports Read Block.\n");
            }
            else
            {
                dbg_print("Fatal: I2C driver does not support Read Block!\n");
                return I2C_FAILED;
            }
        }
        else
        {
            dbg_print("Fatal: I2C driver CANNOT support plain i2c-level commands!\n");
            return I2C_FAILED;
        }
    }
#elif defined(ZEPHYR)
    if (!(axSmDevice = device_get_binding(devName)))
    {
        dbg_print("opening failed...\n");
        dbg_perror("Failed to open the i2c bus");
        return I2C_FAILED;
    }

    if (i2c_configure(axSmDevice, I2C_SPEED_SET(I2C_SPEED_STANDARD)) < 0)
    {
        dbg_print("I2C driver failed setting speed\n");
    }
#endif

   return I2C_OK;
}

/**
* Closes the communication channel to I2C device (not implemented)
*/
void axI2CTerm(int mode)
{
    AX_UNUSED_ARG(mode);
    dbg_print("axI2CTerm: not implemented.\n");
    return;
}

/**
 * Write a single byte to the slave device.
 * In the context of the SCI2C protocol, this command is only invoked
 * to trigger a wake-up of the attached secure module. As such this
 * wakeup command 'wakes' the device, but does not receive a valid response.
 * \note \par bus is currently not used to distinguish between I2C masters.
*/
i2c_error_t axI2CWriteByte(unsigned char bus, unsigned char addr, unsigned char *pTx)
{
    int nrWritten = -1;
    i2c_error_t rv;

    if (bus != I2C_BUS_0)
    {
        dbg_print("axI2CWriteByte on wrong bus %x (addr %x)\n", bus, addr);
    }

#ifdef LINUX
    nrWritten = write(axSmDevice, pTx, 1);
#elif defined(ZEPHYR)
    nrWritten = i2c_write(axSmDevice, pTx, 1, axSmDevice_addr);
#endif
    if (nrWritten < 0)
    {
        // dbg_print("Failed writing data (nrWritten=%d).\n", nrWritten);
        rv = I2C_FAILED;
    }
    else
    {
#ifdef LINUX
        if (nrWritten == 1)
        {
            rv = I2C_OK;
        }
        else
        {
            rv = I2C_FAILED;
        }
#elif defined(ZEPHYR)
        rv = I2C_OK;
#endif
    }

    return rv;
}

i2c_error_t axI2CWrite(unsigned char bus, unsigned char addr, unsigned char * pTx, unsigned short txLen)
{
    int nrWritten = -1;
    i2c_error_t rv;
#ifdef LOG_I2C
    int i = 0;
#endif

    if (bus != I2C_BUS_0)
    {
        dbg_print("axI2CWrite on wrong bus %x (addr %x)\n", bus, addr);
    }
#ifdef LOG_I2C
    dbg_print("TX (axI2CWrite): ");
    for (i = 0; i < txLen; i++)
    {
        dbg_print("%02X ", pTx[i]);
    }
    dbg_print("\n");
#endif

#ifdef LINUX
   nrWritten = write(axSmDevice, pTx, txLen);
#elif defined(ZEPHYR)
   nrWritten = i2c_write(axSmDevice, pTx, txLen, axSmDevice_addr);
#endif
   if (nrWritten < 0)
   {
      dbg_print("Failed writing data (nrWritten=%d).\n", nrWritten);
      rv = I2C_FAILED;
   }
   else
   {
#ifdef LINUX
        if (nrWritten == txLen) // okay
        {
            rv = I2C_OK;
        }
        else
        {
            rv = I2C_FAILED;
        }
#elif defined(ZEPHYR)
	rv = I2C_OK;
#endif
   }
#ifdef LOG_I2C
    dbg_print("    Done with rv = %02x ", rv);
    dbg_print("\n");
#endif

   return rv;
}

i2c_error_t axI2CWriteRead(unsigned char bus, unsigned char addr, unsigned char * pTx,
      unsigned short txLen, unsigned char * pRx, unsigned short * pRxLen)
{
#ifdef LINUX
    struct i2c_rdwr_ioctl_data packets;
#endif
    struct i2c_msg messages[2];
    int r = 0;
#ifdef LOG_I2C
    int i = 0;
#endif

    if (bus != I2C_BUS_0) // change if bus 0 is not the correct bus
    {
        dbg_print("axI2CWriteRead on wrong bus %x (addr %x)\n", bus, addr);
    }

#ifdef LINUX
    messages[0].addr  = axSmDevice_addr;
    messages[0].flags = 0;
#elif defined(ZEPHYR)
    messages[0].flags = I2C_MSG_WRITE;
#endif
    messages[0].len   = txLen;
    messages[0].buf   = pTx;

#ifdef LINUX
    // NOTE:
    // By setting the 'I2C_M_RECV_LEN' bit in 'messages[1].flags' one ensures
    // the I2C Block Read feature is used.
    messages[1].addr  = axSmDevice_addr;
    messages[1].flags = I2C_M_RD | I2C_M_RECV_LEN;
#elif defined(ZEPHYR)
    messages[1].flags = I2C_MSG_READ | I2C_MSG_RECV_LEN | I2C_MSG_STOP;
#endif
    messages[1].len   = 256;
    messages[1].buf   = pRx;
    messages[1].buf[0] = 1;

#ifdef LINUX
    // NOTE:
    // By passing the two message structures via the packets structure as
    // a parameter to the ioctl call one ensures a Repeated Start is triggered.
    packets.msgs      = messages;
    packets.nmsgs     = 2;
#ifdef LOG_I2C
    dbg_print("TX (%d byte): ", txLen);
    for (i = 0; i < txLen; i++)
    {
        dbg_print("%02X ", packets.msgs[0].buf[i]);
    }
    dbg_print("\n");
#endif

    // Send the request to the kernel and get the result back
    r = ioctl(axSmDevice, I2C_RDWR, &packets);

    // NOTE:
    // The ioctl return value in case of a NACK on the write address is '-1'
    // This impacts the error handling routine of the caller.
    if (r < 0)
    {
#ifdef LOG_I2C
        dbg_print("axI2CWriteRead: ioctl cmd I2C_RDWR fails with value %d (errno = %d)\n", r, errno);
        dbg_perror("axI2CWriteRead (Errorstring)");
#endif
        // dbg_print("axI2CWriteRead: ioctl cmd I2C_RDWR fails with value %d (errno = %d)\n", r, errno);
        // dbg_perror("axI2CWriteRead (Errorstring)");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
#define E_NACK_I2C_IMX ENXIO
// #warning "ENXIO"
#else
#define E_NACK_I2C_IMX EIO
// #warning "EIO"
#endif // LINUX_VERSION_CODE
        // In case of IMX, errno == E_NACK_I2C_IMX is not exclusively bound to NACK on address,
        // it can also signal a NACK on a data byte        
        if (errno == E_NACK_I2C_IMX) {
            // dbg_print("axI2CWriteRead: ioctl signals NACK (errno = %d)\n", errno);
            return I2C_NACK_ON_ADDRESS;
        }
        else {
            // dbg_print("axI2CWriteRead: ioctl error (errno = %d)\n", errno);
            return I2C_FAILED;
        }
    }
    else
    {
        int rlen = packets.msgs[1].buf[0]+1;

        //dbg_print("packets.msgs[1].len is %d \n", packets.msgs[1].len);
#ifdef LOG_I2C
        dbg_print("RX  (%d): ", rlen);
        for (i = 0; i < rlen; i++)
        {
            dbg_print("%02X ", packets.msgs[1].buf[i]);
        }
        dbg_print("\n");
#endif
        for (i = 0; i < rlen; i++)
        {
            pRx[i] = packets.msgs[1].buf[i];
        }
        *pRxLen = rlen;
    }
#elif defined(ZEPHYR)
#ifdef LOG_I2C
    dbg_print("TX (%d byte): ", txLen);
    for (i = 0; i < txLen; i++)
    {
        dbg_print("%02X ", messages[0].buf[i]);
    }
    dbg_print("\n");
#endif

    // Send the request to the kernel and get the result back
    r = i2c_transfer(axSmDevice, &messages[0], 2, axSmDevice_addr);

    if (r < 0)
    {
#ifdef LOG_I2C
        dbg_print("axI2CWriteRead: i2c_transfer fails with value %d\n", r);
        dbg_perror("axI2CWriteRead (Errorstring)");
#endif
        return I2C_FAILED;
    }
    else
    {
        int rlen = pRx[0]+1;

        //dbg_print("messages[1].len is %d \n", messagese[1].len);
#ifdef LOG_I2C
        dbg_print("RX  (%d): ", rlen);
        for (i = 0; i < rlen; i++)
        {
            dbg_print("%02X ", pRx[i]);
        }
        dbg_print("\n");
#endif
        *pRxLen = rlen;
    }
#endif

    return I2C_OK;
}
