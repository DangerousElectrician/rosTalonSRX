CXX = g++
DEBUG = 
CXXFLAGS = -Wall -c $(DEBUG)

test : CtreCanNode.o CanTalonSRX.o CANSessionMux.o
	$(CXX) $(CXXFLAGS) -o test

CanTalonSRX : HAL/CanTalonSRX.h HAL/CanTalonSRX.cpp
	$(CXX) $(CXXFLAGS) -c HAL/CanTalonSRX.cpp
