include makefile.conf

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) -g -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

