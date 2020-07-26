include makefile.conf

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

