#include <kernel/types.h>
#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>
#include <stddef.h>

#define INTERN_BUF_SIZE 64
#define SH_BUF_SIZE 128
typedef struct sbuf{
	char sbuffer[SH_BUF_SIZE];
	size_t slen;
} buf_shell;

buf_shell shbuf = {.slen = 0};
extern i64 contAlarm;

void proc_comando(buf_shell shbuf){
	if (shbuf.sbuffer[0] == '\0') return;
	char *com = shbuf.sbuffer;
	char *arg;
	for (size_t i = 0; i < shbuf.slen; i++){
		if (shbuf.sbuffer[i] == ' '){
			shbuf.sbuffer[i] = '\0';
			arg = &shbuf.sbuffer[i + 1];
			break;
		}
	}
	if (!strcmp(com, "uptime")){
		char strtmp[12];
		u64 tm_segundos = get_up_secs();
		snprintf(strtmp, sizeof(strtmp), "%ds\n", tm_segundos);
		print(strtmp);
	}
	if (!strcmp(com, "alarm")){
		u64 secs;
		if (arg != NULL){
			secs = strtou64(arg, 10);
			contAlarm = (i64)secs;
		}
	}
	if (!strcmp(com, "echo")){
		print(arg);
		print("\n");
	}
}

extern int _hartid[];
void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	char rx_temp[INTERN_BUF_SIZE];
	serial_puts("\n");
	serial_puts("> ");
	while(1){
		size_t tam_lido = serial_read(rx_temp);
		for (size_t i = 0; i < tam_lido; i++){
			char c = rx_temp[i];
			if (c == '\r'){
				serial_puts("\r\n");
				shbuf.sbuffer[shbuf.slen] = '\0';
				proc_comando(shbuf);
				serial_puts("> ");
				shbuf.slen = 0;
			}
			else if (c == 127){
				if (shbuf.slen > 0){
					shbuf.slen--;
					serial_puts("\b \b");
				}
			}
			else {
				if (shbuf.slen < SH_BUF_SIZE - 1){
					serial_putc(c);
					shbuf.sbuffer[shbuf.slen] = c;
					shbuf.slen++;
				}
			}
		}
	}
}

