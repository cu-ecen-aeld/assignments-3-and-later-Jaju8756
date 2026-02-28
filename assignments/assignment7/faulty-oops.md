# Kernel Oops Analysis  
## Command executed  
 ```
 echo “hello_world” > /dev/faulty
 ```
This command attempts to write data to the character device /dev/faulty. The file does not store data. Instead, writing to it calls the driver’s `faulty_write()` function.  
	 
```
struct file_operations faulty_fops = {  
	.read =  faulty_read,  
	.write = faulty_write,  
	.owner = THIS_MODULE  
};
```
The above `faulty_fops` is invoked during `faulty_init(void)`   

## Faulty Driver code  
```
ssize_t faulty_write(struct file *filp, const char __user *buf,  
				 size_t count, loff_t *pos)  
{  
	*(int *)0 = 0;  
	return 0;  
}
```

This intentionally dereferences a NULL pointer which means the driver tries to write to memory address 0x0  

## The oops message includes:

### Exception type (Data Abort):<br/>
>`EC = 0x25: DABT (current EL), IL = 32 bits`<br/>  	☠️ _Data Abort occurred in current Exception Level(kernel mode)_

### Faulting address (0x0000000000000000):<br/>
>`[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000`<br/>  	☠️ _CPU page tables indicating NULL pointer dereference._
### Program counter (faulty_write):<br/>
>`pc : faulty_write+0x10/0x20 [faulty]`<br/>	☠️ _Program Counter at 'faulty_write()+offset' confirms the crash occurred inside `faulty_write()`_
### Call trace: <br/>
>faulty_write+0x10/0x20 [faulty] <tab/>  ☠️ _faulty_write_  
 ksys_write+0x74/0x110		⏫ _Kernel write handler_  
 __arm64_sys_write+0x1c/0x30	⏫ _ARM64 system call implementation_  
 invoke_syscall+0x54/0x130	⏫ _Generic syscall dispatcher_  
 el0_svc_common.constprop.0+0x44/0xf0  
### Write Exception (WnR = 1):
>`CM = 0, WnR = 1`<br/>	☠️ _Exception was caused by a Write operation_  
	
## Kernel Oops
```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000045
  EC = 0x25: DABT (current EL), IL = 32 bits	
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1	
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000041b9e000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000	
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 121 Comm: sh Tainted: G           O       6.1.44 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x10/0x20 [faulty]	
lr : vfs_write+0xc8/0x390
sp : ffffffc008ddbd20
x29: ffffffc008ddbd80 x28: ffffff8001b36a00 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000000000012 x22: 0000000000000012 x21: ffffffc008ddbdc0
x20: 00000055796f3a60 x19: ffffff8001bba300 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc000787000 x3 : ffffffc008ddbdc0
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
Call trace:	
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110		
 __arm64_sys_write+0x1c/0x30	
 invoke_syscall+0x54/0x130	
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 0000000000000000 ]---
```


