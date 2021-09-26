void kernel_main() {
	char *screen_buffer = (void*)0xB8000;
	char *msg = "Hello world!";
	unsigned int i = 24 * 80;
	while (*msg) {
		screen_buffer[i * 2] = *msg;
		msg++;
		i++;
	}
}