/*_
 * Copyright (c) 2016 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <mki/driver.h>
#include "i40e.h"

/* Prototype declarations */
static int _setup_admin_queue(struct i40e_device *);

/*
 * Initialize an i40e device
 */
struct i40e_device *
i40e_init(uint16_t device_id, uint16_t bus, uint16_t slot, uint16_t func)
{
    struct i40e_device *dev;
    uint64_t pmmio;
    uint32_t m32;
    int ret;

    /* Allocate an i40e device data structure */
    dev = malloc(sizeof(struct i40e_device));
    if ( NULL == dev ) {
        return NULL;
    }
    dev->device_id = device_id;

    /* Allocate HMC */
    dev->hmc = malloc(I40E_HMC_SIZE);
    if ( NULL == dev->hmc ) {
        free(dev);
        return NULL;
    }

    /* Read MMIO */
    pmmio = pci_read_mmio(bus, slot, func);
    dev->mmio = driver_mmap((void *)pmmio, I40E_MMIO_SIZE);
    if ( NULL == dev->mmio ) {
        /* Error */
        free(dev);
        return NULL;
    }

    /* Initialize the PCI configuration space */
    m32 = pci_read_config(bus, slot, func, 0x4);
    pci_write_config(bus, slot, func, 0x4, m32 | 0x7);

    /* Get the device MAC address */
    i40e_read_mac_address(dev);

    /* Setup an admin queue */
    ret = _setup_admin_queue(dev);
    if ( ret < 0 ) {
        free(dev->hmc);
        free(dev);
        return NULL;
    }

    return dev;
}

/*
 * Get the device MAC address
 */
int
i40e_read_mac_address(struct i40e_device *dev)
{
    uint32_t m32;

    /* Read MAC address */
    m32 = rd32(dev->mmio, I40E_PRTPM_SAL(0));
    dev->macaddr[0] = m32 & 0xff;
    dev->macaddr[1] = (m32 >> 8) & 0xff;
    dev->macaddr[2] = (m32 >> 16) & 0xff;
    dev->macaddr[3] = (m32 >> 24) & 0xff;
    m32 = rd32(dev->mmio, I40E_PRTPM_SAH(0));
    dev->macaddr[4] = m32 & 0xff;
    dev->macaddr[5] = (m32) >> 8 & 0xff;

    return 0;
}

/*
 * Setup the admin queue
 */
static int
_setup_admin_queue(struct i40e_device *dev)
{
    int i;
    uint64_t m64;

    /* Allocate the descriptors of an admin queue and their buffers */
    dev->atq.len = I40E_AQ_LEN;
    dev->atq.descs = malloc(sizeof(struct i40e_aq_desc) * dev->atq.len);
    if ( NULL == dev->atq.descs ) {
        return -1;
    }
    dev->atq.tail = 0;
    dev->atq.bufset = malloc(I40E_AQ_BUF * dev->atq.len);
    if ( NULL == dev->atq.bufset ) {
        free(dev->atq.descs);
        return -1;
    }
    /* Resolve the physical address */
    dev->atq.base = NULL;       /* FIXME */
    dev->atq.pbufset = NULL;    /* FIXME */

    dev->arq.len = I40E_AQ_LEN;
    dev->arq.descs = malloc(sizeof(struct i40e_aq_desc) * dev->arq.len);
    if ( NULL == dev->arq.descs ) {
        free(dev->atq.descs);
        free(dev->atq.bufset);
        return -1;
    }
    dev->arq.tail = 0;
    dev->arq.bufset = malloc(I40E_AQ_BUF * dev->arq.len);
    if ( NULL == dev->arq.bufset ) {
        free(dev->atq.descs);
        free(dev->atq.bufset);
        free(dev->arq.descs);
        return -1;
    }
    /* Resolve the physical address */
    dev->arq.base = NULL;       /* FIXME */
    dev->arq.pbufset = NULL;    /* FIXME */

    /* Set up the Rx descriptors */
    for ( i = 0; i < dev->arq.len; i++ ) {
        dev->arq.descs[i].flags = (1 << 9) | (1 << 12);
        dev->arq.descs[i].opcode = 0;
        dev->arq.descs[i].len = I40E_AQ_BUF;
        dev->arq.descs[i].ret = 0;
        dev->arq.descs[i].cookieh = 0;
        dev->arq.descs[i].cookiel = 0;
        dev->arq.descs[i].param0 = 0;
        dev->arq.descs[i].param1 = 0;
        m64 = (uint64_t)dev->arq.pbufset + (i + I40E_AQ_BUF);
        dev->arq.descs[i].addrh = m64 >> 32;
        dev->arq.descs[i].addrl = m64;
    }

    /* Set the physicall addres of the Tx queue base */
    wr32(dev->mmio, I40E_PF_ATQH, 0);
    wr32(dev->mmio, I40E_PF_ATQT, 0);
    m64 = (uint64_t)dev->atq.base;
    wr32(dev->mmio, I40E_PF_ATQBAL, (uint32_t)m64);
    wr32(dev->mmio, I40E_PF_ATQBAH, m64 >> 32);
    wr32(dev->mmio, I40E_PF_ATQLEN, dev->atq.len | (1ULL << 31));

    /* Set the physical addresss of the Rx queue base */
    wr32(dev->mmio, I40E_PF_ARQH, 0);
    wr32(dev->mmio, I40E_PF_ARQT, 0);
    m64 = (uint64_t)dev->arq.base;
    wr32(dev->mmio, I40E_PF_ARQBAL, (uint32_t)m64);
    wr32(dev->mmio, I40E_PF_ARQBAH, m64 >> 32);
    wr32(dev->mmio, I40E_PF_ARQLEN, dev->arq.len | (1ULL << 31));

    return 0;
}

/*
 * Get version by admin command
 */
int
i40e_ac_get_ver(struct i40e_device *dev, int *major, int *minor, int *build,
                int *subbuild)
{
    int idx;
    int ret;
    struct i40e_aq_desc *desc;

    /* Save the tail pointer */
    idx = dev->atq.tail;

    /* Disable LLDP */
    desc = &dev->atq.descs[idx];
    desc->flags = 0;
    desc->opcode = 0x0001;
    desc->len = 0;
    desc->ret = 0;
    desc->cookieh = 0x1234;
    desc->cookiel = 0x5678;
    desc->param0 = 0;
    desc->param1 = 0;
    desc->addrh = 0;
    desc->addrl = 0x00010001;

    /* Advance the tail pointer */
    dev->atq.tail = dev->atq.tail < dev->atq.len ? dev->atq.tail + 1 : 0;

    /* Write the tail pointer to the NIC */
    wr32(dev->mmio, I40E_PF_ATQT, dev->atq.tail);

    /* Wait until the response is received */
    while ( !(dev->atq.descs[idx].flags & 0x1) ) {
    }

    /* Get the return value */
    ret = dev->atq.descs[idx].ret;
    if ( 0 == ret ) {
        *major = desc->param0 & 0xff;
        *minor = (desc->param0 >> 8) & 0xff;
        *build = (desc->param0 >> 16) & 0xff;
        *subbuild = (desc->param0 >> 24) & 0xff;
    }

    return ret;
}

/*
 * Clear PXE mode
 */
int
i40e_ac_clear_pxe(struct i40e_device *dev)
{
    int idx;
    int ret;
    struct i40e_aq_desc *desc;

    /* Save the tail pointer */
    idx = dev->atq.tail;

    /* Disable LLDP */
    desc = &dev->atq.descs[idx];
    desc->flags = 0;
    desc->opcode = 0x0110;
    desc->len = 0;
    desc->ret = 0;
    desc->cookieh = 0x5678;
    desc->cookiel = 0x9abc;
    desc->param0 = 2;
    desc->param1 = 0;
    desc->addrh = 0;
    desc->addrl = 0;

    /* Advance the tail pointer */
    dev->atq.tail = dev->atq.tail < dev->atq.len ? dev->atq.tail + 1 : 0;

    /* Write the tail pointer to the NIC */
    wr32(dev->mmio, I40E_PF_ATQT, dev->atq.tail);

    /* Wait until the response is received */
    while ( !(dev->atq.descs[idx].flags & 0x1) ) {
    }

    /* Get the return value */
    ret = dev->atq.descs[idx].ret;

    return ret;
}

/*
 * Disable LLDP
 */
int
i40e_ac_disable_lldp(struct i40e_device *dev)
{
    int idx;
    int ret;
    struct i40e_aq_desc *desc;

    /* Save the tail pointer */
    idx = dev->atq.tail;

    /* Disable LLDP */
    desc = &dev->atq.descs[idx];
    desc->flags = 0;
    desc->opcode = 0x0a05;
    desc->len = 0;
    desc->ret = 0;
    desc->cookieh = 0xabcd;
    desc->cookiel = 0xef01;
    desc->param0 = 0x00000001UL;
    desc->param1 = 0;
    desc->addrh = 0;
    desc->addrl = 0;

    /* Advance the tail pointer */
    dev->atq.tail = dev->atq.tail < dev->atq.len ? dev->atq.tail + 1 : 0;

    /* Write the tail pointer to the NIC */
    wr32(dev->mmio, I40E_PF_ATQT, dev->atq.tail);

    /* Wait until the response is received */
    while ( !(dev->atq.descs[idx].flags & 0x1) ) {
    }

    /* Get the return value */
    ret = dev->atq.descs[idx].ret;

    return ret;
}

/*
 * Set MAC configuration
 */
int
i40e_set_mac_config(struct i40e_device *dev, int mtu)
{
    int idx;
    int ret;
    struct i40e_aq_desc *desc;

    /* Save the tail pointer */
    idx = dev->atq.tail;

    /* Set MAC configuration */
    desc = &dev->atq.descs[idx];
    desc->flags = 0;
    desc->opcode = 0x0603;
    desc->len = 0;
    desc->ret = 0;
    desc->cookieh = 0x9abc;
    desc->cookiel = 0xdef0;
    desc->param0 = mtu | (1 << 18) | (0 << 24);
    desc->param1 = 0;
    desc->addrh = 0;
    desc->addrl = 0;

    /* Advance the tail pointer */
    dev->atq.tail = dev->atq.tail < dev->atq.len ? dev->atq.tail + 1 : 0;

    /* Write the tail pointer to the NIC */
    wr32(dev->mmio, I40E_PF_ATQT, dev->atq.tail);

    /* Wait until the response is received */
    while ( !(dev->atq.descs[idx].flags & 0x1) ) {
    }

    /* Get the return value */
    ret = dev->atq.descs[idx].ret;

    return ret;
}

/*
 * Set promiscuous mode
 */
int
i40e_ac_set_promisc_vsi(struct i40e_device *dev, uint16_t seid)
{
    int idx;
    int ret;
    struct i40e_aq_desc *desc;

    /* Save the tail pointer */
    idx = dev->atq.tail;

    /* Set promiscuous mode */
    desc = &dev->atq.descs[idx];
    desc->flags = 0;
    desc->opcode = 0x0254;
    desc->len = 0;
    desc->ret = 0;
    desc->cookieh = 0x1234;
    desc->cookiel = 0x5678;
    desc->param0 = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 4)
        | (((1 << 0) | (1 << 1) | (1 << 2) | (1 << 4)) << 16);
    desc->param1 = seid;
    desc->addrh = 0;
    desc->addrl = 0;

    /* Advance the tail pointer */
    dev->atq.tail = dev->atq.tail < dev->atq.len ? dev->atq.tail + 1 : 0;

    /* Write the tail pointer to the NIC */
    wr32(dev->mmio, I40E_PF_ATQT, dev->atq.tail);

    /* Wait until the response is received */
    while ( !(dev->atq.descs[idx].flags & 0x1) ) {
    }

    /* Get the return value */
    ret = dev->atq.descs[idx].ret;

    return ret;
}

int
i40e_ac_set_promisc(struct i40e_device *dev)
{
    int idx;
    struct i40e_aq_desc *desc;
    struct i40e_adm_get_switch_config_hdr *hdr;
    struct i40e_adm_get_switch_config_elem *elem;
    int ret;
    int i;
    uint64_t m64;

    /*  */
    idx = dev->atq.tail;

    /* Get the switch configuration first to resolve the default VSI */
    desc = &dev->atq.descs[idx];
    desc->flags = (1 << 9) | (1 << 12);
    desc->opcode = 0x0200;
    desc->len = I40E_AQ_BUF;
    desc->ret = 0;
    desc->cookieh = 0x3456;
    desc->cookiel = 0x789a;
    desc->param0 = 0;
    desc->param1 = 0;
    m64 = (uint64_t)dev->atq.pbufset + (idx * I40E_AQ_BUF);
    desc->addrh = m64 >> 32;
    desc->addrh = m64;

    /* Advance the tail pointer */
    dev->atq.tail = dev->atq.tail < dev->atq.len ? dev->atq.tail + 1 : 0;

    /* Write the tail pointer to the NIC */
    wr32(dev->mmio, I40E_PF_ATQT, dev->atq.tail);

    /* Wait until the response is received */
    while ( !(dev->atq.descs[idx].flags & 0x1) ) {
    }

    /* Get the return value */
    ret = dev->atq.descs[idx].ret;
    if ( 0 != ret ) {
        /* An error occurs */
        return ret;
    }

    /* Get the header and elements */
    hdr = (struct i40e_adm_get_switch_config_hdr *)
        (dev->atq.bufset + (idx * I40E_AQ_BUF));
    elem = (struct i40e_adm_get_switch_config_elem *)(hdr + 1);

    /* Check the number of elements */
    if ( hdr->num_elem < 1 ) {
        return -1;
    }
    /* Find out VSI */
    for ( i = 0; i < hdr->num_elem; i++ ) {
        if ( 0x13 == elem[i].elem_type ) {
            /* This is a VSI, then try to set promiscuous mode */
            ret = i40e_ac_set_promisc_vsi(dev, elem[i].seid);
        }
    }

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
