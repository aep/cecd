
cecd: main.cpp
	$(CXX) $^ -o $@ -Ilibcec/include -ldl ~/proj/ubus/lib/libubus.a
