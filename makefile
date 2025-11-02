# CXX ?= g++

# DEBUG ?= 1
# ifeq ($(DEBUG), 1)
#     CXXFLAGS += -g
# else
#     CXXFLAGS += -O2

# endif

# server: main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  webserver.cpp config.cpp
# 	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient

# clean:
# 	rm  -r server



CXX := g++
CXXFLAGS := -Wall $(if $(DEBUG),-g,-O2)
LIBS := -lpthread -lmysqlclient

SRC := main.cpp \
       timer/lst_timer.cpp \
       http/http_conn.cpp \
       log/log.cpp \
       CGImysql/sql_connection_pool.cpp \
       webserver.cpp

TARGET := server

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)
