
all:
#	make -C dummy_dir -f Makefile
#	make -C macos8addons -f Makefile
	make -C SDL-main -f Makefile
	cp -rf */*.a SDL-main/include .
	
clean:
#	make -C __debugbreak -f Makefile clean
#	make -C dummy_atomic -f Makefile clean
#	make -C dummy_dir -f Makefile clean
#	make -C dummy_image -f Makefile clean
#	make -C dummy_libstdcpp -f Makefile clean
#	make -C dummy_memory -f Makefile clean
#	make -C dummy_pthreads -f Makefile clean
#	make -C dummy_time -f Makefile clean
#	make -C mytests -f Makefile clean
	make -C SDL-main -f Makefile clean
	rm -rf */*.o */*.a *.a include