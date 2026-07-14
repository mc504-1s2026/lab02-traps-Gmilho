#include <kernel/serial.h>
#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>

u64 kuptime = 0;
i64 contAlarm = -1;

void contagem_alarm(){
	if (contAlarm > 0){
		contAlarm--;
	}
	if (contAlarm == 0){
		serial_puts("\r\n");
		serial_puts("alarm");
		serial_puts("\n\r> ");
		contAlarm = -1;
	}
}

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	timer_set_alarm(1);
	csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
	u64 al = now + (TIMER_FREQ * secs);
	csr_write(CSR_STIMECMP, al);
}

void timer_irq()
{
	timer_set_alarm(1);
	kuptime++;
	contagem_alarm();
}

u64 get_up_secs(){
	return kuptime;
}