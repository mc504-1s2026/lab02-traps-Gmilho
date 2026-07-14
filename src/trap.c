#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/types.h>
#include <arch/timer.h>
#include <kernel/serial.h>
#include <arch/plic.h>
#include <kernel/printf.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
	u64 sca = csr_read(CSR_SCAUSE);	
	if ((sca & TRAP_TIMER_IRQ) == TRAP_TIMER_IRQ){
			timer_irq();
		}
		else if ((sca & TRAP_EXTERNAL_IRQ) == TRAP_EXTERNAL_IRQ) {
			u32 irq_addr = plic_hart_claim_irq(0);
			if (irq_addr == IRQ_SERIAL){
				serial_irq();
			}
			if (irq_addr){
				plic_hart_complete_irq(0, irq_addr);
			}
			return;
		}
}

void handle_exception()
{
	u64 sca = csr_read(CSR_SCAUSE);
	u64 ex_cause = sca & 0xfffUL;
	switch (ex_cause){
		case EXCEPTION_INST_ACCESS_FAULT:
			error("Exception: instrução tentou acessar um endereço inválido\n");
			break;

		case EXCEPTION_LOAD_ACCESS_FAULT:
			error("Exception: carregamento de um endereço inválido\n");
			break;

		case EXCEPTION_STORE_ACCESS_FAULT:
			error("Exception: store em um endereço inválido\n");
			break;

		case EXCEPTION_INST_PAGE_FAULT:
			error("Exception: instrução tentou acessar um espaço inválido na ram\n");
			break;

		case EXCEPTION_LOAD_PAGE_FAULT:
			error("Exception: carregamento de um espaço inválido da ram\n");
			break;

		case EXCEPTION_STORE_PAGE_FAULT:
			error("Exception: store em um espaço inválido na ram\n");
			break;

		default:
			BUG();
			break;
	}
	return;
}

void trap_setup()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	csr_write(CSR_STVEC, trap_entry);
	plic_irq_set_priority(IRQ_SERIAL, 2);
	plic_hart_set_threshold(0, 0);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void handle_trap()
{
	u64 sca = csr_read(CSR_SCAUSE);
	u64 type = sca & TRAP_IRQ_BIT;
	if (type == 0){
		handle_exception();
	} else {
		handle_irq();
	}
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 ssval = csr_read(CSR_SSTATUS);
	u64 state = ssval & CSR_SSTATUS_SIE;
	hart_irq_disable();
	return state;
}

void hart_irq_restore(u64 flags)
{
	if (flags){
		hart_irq_enable();
	}
	else {
		hart_irq_disable();
	}
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
