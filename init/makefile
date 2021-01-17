INIT = init.bin
# INIT = shell_0.bin shell_1.bin shell_2.bin 

# you can add your elf in $(BIN) latter
BIN = $(INIT)
TAR_NAME = app.tar
HD = ../b.img

# INSTALL_INIT / INSTALL_TAR / INSTALL_FAT
INSTALL_TYPE = INSTALL_TAR
# INSTALL_TYPE = INSTALL_INIT
# INSTALL_TYPE = INSTALL_FAT

# oranges文件系统在硬盘上的起始扇区
# PART_START_SECTOR = 92049
PART_START_SECTOR = 6144	# modified by mingxuan 2020-10-12

# 写入硬盘的起始位置
# INSTALL_PHY_SECTOR = PART_START_SECTOR + 951 # Why is 951 ?
INSTALL_PHY_SECTOR = 7095	# modified by mingxuan 2020-10-12
# assert(INSTALL_PHY_SECTOR > PART_START_SECTOR)

# 写入硬盘的文件大小
INSTALL_NR_SECTORS = 1000

INSTALL_START_SECTOR = $(shell echo $$(($(INSTALL_PHY_SECTOR)-$(PART_START_SECTOR))))
SUPER_BLOCK_ADDR = $(shell echo $$((($(PART_START_SECTOR)+1)*512)))

ifeq ($(INSTALL_TYPE),INSTALL_INIT)
	INSTALL_FILENAME:=$(INIT)
else ifeq ($(INSTALL_TYPE),INSTALL_TAR)
	INSTALL_FILENAME:=$(TAR_NAME)
else ifeq ($(INSTALL_TYPE),INSTALL_FAT)
	INSTALL_FILENAME:=$(INIT)
endif

default:
	@#将super block->magic 置0
	dd if=/dev/zero of=$(HD) bs=1 conv=notrunc seek=$(SUPER_BLOCK_ADDR) count=1
ifeq ($(INSTALL_TYPE),INSTALL_INIT)
else ifeq ($(INSTALL_TYPE),INSTALL_FAT)
else ifeq ($(INSTALL_TYPE),INSTALL_TAR)
	tar vcf $(TAR_NAME) $(BIN)
else
	@echo Error: unknown INSTALL_TYPE $(INSTALL_TYPE) && exit 1
endif
	@echo INSTALL_FILENAME=$(INSTALL_FILENAME) 
ifeq ($(INSTALL_TYPE),INSTALL_FAT)
	sudo losetup -P /dev/loop0 $(HD)
	sudo mkfs.vfat -F 32 /dev/loop0p6

	sudo mount /dev/loop0p6 ../iso
	sudo cp $(INSTALL_FILENAME) ../iso/
	sudo umount ../iso

	sudo losetup -d /dev/loop0
else
	dd if=$(INSTALL_FILENAME) of=$(HD) bs=512 conv=notrunc seek=$(INSTALL_PHY_SECTOR) count=$(INSTALL_NR_SECTORS) 
endif

clean_init:
	rm -rf $(TAR_NAME) $(BIN) *.map *.bin
