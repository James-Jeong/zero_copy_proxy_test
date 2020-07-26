include makefile.conf

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ -lpthread

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

