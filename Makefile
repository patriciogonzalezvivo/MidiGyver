default:
	@mkdir build
	@cd build && cmake .. && make && cd ..

clean:
	@rm -rf build

install:
	@cd build
	@sudo make install
	@cd ..