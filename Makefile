DEFAULT_TARGET: test

OUT:=.out
LFLAGS+=-lncursesw

test: all
	$(OUT)/fade

all: $(OUT)/fade

$(OUT)/fade: fade.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

clean:
	rm -rf $(OUT)
