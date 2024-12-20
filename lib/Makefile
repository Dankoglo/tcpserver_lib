CXX=g++
CXXFLAGS=-c
OUT_DIR=objects

SERVER_MODULES=server/server.cpp pool/thread_pool.cpp utils/utils.cpp
CLIENT_MODULES=client/client.cpp session/session.cpp utils/utils.cpp

BUILD_SERVER_OBJECT=for module in $(SERVER_MODULES); do \
						$(CXX) $(CXXFLAGS) $$module; \
			  		done; \
					ld -relocatable *.o -o $(OUT_DIR)/server.o; \
					rm *.o

BUILD_CLIENT_OBJECT=for module in $(CLIENT_MODULES); do \
						$(CXX) $(CXXFLAGS) $$module; \
			  		done; \
			  		ld -relocatable *.o -o $(OUT_DIR)/client.o; \
					rm *.o

BUILD_OBJECT=for module in $(CLIENT_MODULES) $(SERVER_MODULES); do \
			 	$(CXX) $(CXXFLAGS) $$module; \
			 done; \
			 ld -relocatable *.o -o $(OUT_DIR)/lib.o; \
			 rm *.o

UPDATE=if [ -d $(OUT_DIR) ]; then \
			rm -r $(OUT_DIR); \
	   fi;

build:
	@$(UPDATE)
	@mkdir $(OUT_DIR)
	@$(BUILD_OBJECT)

