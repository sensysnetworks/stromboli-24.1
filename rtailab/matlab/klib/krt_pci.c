/*
COPYRIGHT (C) 2003  Roberto Bucher (roberto.bucher@die.supsi.ch)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include "linux/module.h"
#include "linux/pci.h"

int get_pci_addr(unsigned int vendor, unsigned int device, unsigned int addr_mat[])
{
struct pci_dev *pdev=NULL;
unsigned int base_pci, base_status, base_adc, base_dio, base_dac;

  pdev=pci_find_device(vendor,device,pdev);
  if(pdev) {
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_0, &base_pci);
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_1, &base_status);
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_2, &base_adc);
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_3, &base_dio);
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_4, &base_dac);
                                           
    base_pci    &= PCI_BASE_ADDRESS_IO_MASK;
    base_status &= PCI_BASE_ADDRESS_IO_MASK;
    base_adc    &= PCI_BASE_ADDRESS_IO_MASK;
    base_dio    &= PCI_BASE_ADDRESS_IO_MASK;
    base_dac    &= PCI_BASE_ADDRESS_IO_MASK;

    addr_mat[0]=base_pci;
    addr_mat[1]=base_status;
    addr_mat[2]=base_adc;
    addr_mat[3]=base_dio;
    addr_mat[4]=base_dac;
  }
  return 0;
}
