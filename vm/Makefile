evalbot: hda initramfs.cpio.gz bzImage
	 
initramfs.cpio.gz:
	./write_ver
	( cd root; find . | cpio -o -H newc; ) | gzip -9 > initramfs.cpio.gz

hda: initramfs.cpio.gz bzImage
	./qemu-img create -f qcow2 hda.tmp 1M
	./savestate hda.tmp root
	./savestate hda.tmp root2
	./savestate hda.tmp root3
	./savestate hda.tmp rootk
	./savestate hda.tmp roots
	mv hda.tmp hda

clean: 
	rm -f initramfs.cpio.gz hda hda.tmp fifo *~

