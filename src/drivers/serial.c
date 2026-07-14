#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/io.h>
#include <kernel/types.h>
#include <arch/spinlock.h>
#include <kernel/printf.h>
#include <arch/csr.h>

#define INTERN_BUF_SIZE 64
typedef struct buf{
	char buffer[INTERN_BUF_SIZE];
	size_t len;
} buf_interno;

buf_interno nbuf = {.len = 0};
struct spinlock serial_lock;

void serial_init()
{
	iowrite8(0x0, (SERIAL_BASE + SERIAL_IER));
	iowrite8(0x3, (SERIAL_BASE + SERIAL_LCR));
	iowrite8((SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR | SERIAL_FCR_FIFO_ENABLE), (SERIAL_BASE + SERIAL_FCR));
	iowrite8(0x8, (SERIAL_BASE + SERIAL_MCR));
	iowrite8((SERIAL_IER_ERBFI | SERIAL_IER_ELSI), (SERIAL_BASE + SERIAL_IER));
	spin_init(&serial_lock);
}

void lida_com_LSR(u8 val){
	if (val & SERIAL_LSR_OE){
		trace("Serial: OE - char não lido pela CPU foi sobrescrito\n");
	}
	if (val & SERIAL_LSR_PE){
		trace("Serial: PE - erro de paridade\n");
	}
	if (val & SERIAL_LSR_FE){
		trace("Serial: FE - bit de parada inválido\n");
	}
	if (val & SERIAL_LSR_BI){
		trace("Serial: BI - dados segurados em spacing por mais tempo do que deveriam\n");
	}
	if (val & SERIAL_LSR_RXERR){
		trace("Serial: ocorreu algum(ns) destes erros, PE, FE e BI\n");
	}
}

void serial_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	u8 iir_val = ioread8((SERIAL_BASE + SERIAL_IIR));
	if (!(iir_val & SERIAL_IIR_PENDING)) {
		u8 id = (iir_val & SERIAL_IIR_ID_MASK) >> SERIAL_IIR_ID_SHIFT;
		switch (id){
			case 0b000:
				ioread8((SERIAL_BASE + SERIAL_MSR));
				break;

			case 0b001:
				break; //o iir já foi lido

			case 0b010:
				spin_lock(&serial_lock);
				while(ioread8((SERIAL_BASE + SERIAL_LSR)) & SERIAL_LSR_DTR){
					char c = ioread8((SERIAL_BASE + SERIAL_RBR));
					if (nbuf.len >= INTERN_BUF_SIZE - 1){
						continue;
					} else {
						nbuf.buffer[nbuf.len] = c;
						nbuf.len++;
					}
				}
				spin_unlock(&serial_lock);
				break;

			case 0b011:
				u8 lsr_val = ioread8((SERIAL_BASE + SERIAL_LSR));
				lida_com_LSR(lsr_val);
				break;

			case 0b110:
				ioread8((SERIAL_BASE + SERIAL_RBR));
				break;
		}
	}
	return;
}

size_t serial_read(char *buf)
{
	u64 flags = spin_lock_irqsave(&serial_lock);
	size_t tam = nbuf.len;
	for (size_t i = 0; i < tam; i++){
		buf[i] = nbuf.buffer[i];
	}
	nbuf.len = 0;
	spin_unlock_irqrestore(&serial_lock, flags);
	return tam;
}

void serial_puts(char *str)
{
	while(*str){
		serial_putc(*str);
		str++;
	}
}

void serial_putc(char c)
{
	while (!(ioread8(SERIAL_BASE + SERIAL_LSR) & SERIAL_LSR_THRE)){
		
	}
	iowrite8((u8)c, (SERIAL_BASE + SERIAL_THR));
}
