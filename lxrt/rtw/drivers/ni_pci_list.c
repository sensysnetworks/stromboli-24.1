
#include <stdio.h>
#include <fcntl.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include "ni_pci_lxrt.h"
#include "ni_pci.h"

int main(int argc, char *argv[])
{
	int ni_boards, n;
	int board[MAX_NI_BOARDS];
	unsigned short id_board[MAX_NI_BOARDS];
	char *name = NULL;

	ni_get_n_boards(&ni_boards);
	for (n = 0; n < ni_boards; n++) {
		ni_get_board_device_id(n, &id_board[n]);
		ni_get_board_list_index(id_board[n], &board[n]);
		switch (id_board[n]) {
			case DEVICE_ID_PCI_6071E:
				name = "PCI 6071e";
				break;
			case DEVICE_ID_PCI_MIO_16E_1:
				name = "PCI MIO 16e1";
				break;
			case DEVICE_ID_PCI_MIO_16E_4:
				name = "PCI MIO 16e4";
				break;
			case DEVICE_ID_PCI_6023E:
				name = "PCI 6023e";
				break;
			case DEVICE_ID_PCI_6024E:
				name = "PCI 6024e";
				break;
			case DEVICE_ID_PCI_6025E:
				name = "PCI 6025e";
				break;
			case DEVICE_ID_PCI_6711:
				name = "PCI 6711";
				break;
			case DEVICE_ID_PCI_6713:
				name = "PCI 6713";
				break;
		}
		printf("BOARD: %s (id 0x%x)\nINDEX : %d\n", name, id_board[n], board[n]);
	}

	return 0;
}
